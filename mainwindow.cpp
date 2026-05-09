#include "mainwindow.h"
#include <memory>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QDebug>
#include <QStatusBar>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QDirIterator>
#include <QMessageBox>
#include "fileworker.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    watcher(new QFileSystemWatcher(this)),
    timer(new QTimer(this)),
    progressDialog(nullptr)
{
    settings.loadSettings();
    setupUI();
    pool = QThreadPool::globalInstance();
    pool->setMaxThreadCount(100);
    connect(progressDialog.get(), &QProgressDialog::canceled, this, &MainWindow::stopAllWorkers);
}

MainWindow::~MainWindow()
{
    settings.saveSettings();
    settings.sync();
    pool->clear();
    pool->waitForDone();
}

void MainWindow::stopAllWorkers() {
    for (const auto thread: pool->children()) {
        if (FileWorker* worker = dynamic_cast<FileWorker*>(thread)) {
            worker->cancel();
        }
    }
    pool->clear();
    pool->waitForDone();
    onStatusUpdate();
}

void MainWindow::setupUI()
{
    QWidget *central = new QWidget;
    setCentralWidget(central);
    QGridLayout *layout = new QGridLayout(central);

    // a) Маска
    {
        QLabel *lblMask = new QLabel("Маска файлов:");
        const auto mask = settings.get().mask;
        QLineEdit *editMask = new QLineEdit(mask.isEmpty() ? "*.txt" : mask);
        QRegularExpression re("^\\*\\.[a-zA-Z0-9]+$|^\\*\\.\\*$|^[a-zA-Z0-9]+\\.[a-zA-Z0-9]+$");
        QRegularExpressionValidator *validator = new QRegularExpressionValidator(re, this);
        editMask->setValidator(validator);
        layout->addWidget(lblMask, 0, 0);
        layout->addWidget(editMask, 0, 1);
        connect(editMask, &QLineEdit::textChanged, [&](const QString &t){ settings.setMask(t); });
    }

    // b) Удалять?
    QCheckBox *chkDelete = new QCheckBox("Удалять входные файлы");
    chkDelete->setChecked(settings.get().deleteSource);
    layout->addWidget(chkDelete, 1, 0, 1, 2);
    connect(chkDelete, &QCheckBox::toggled, [&](bool needDeleteSource){ settings.setDeleteSource(needDeleteSource); });

    // Путь поиска входных файлов
    QPushButton *btnInDir = new QPushButton("Выбрать папку вввода");
    const auto inDir = settings.get().inDir;
    QLineEdit *editInDir = new QLineEdit(QDir(inDir).exists() ? inDir : QDir::currentPath());
    connect(btnInDir, &QPushButton::clicked, [this, editInDir](){
        QString dir = QFileDialog::getExistingDirectory(this, "Папка вввода");
        if (!dir.isEmpty()) {
            editInDir->setText(dir);
            settings.setInputDir(dir);
        }
    });
    layout->addWidget(btnInDir, 2, 0);
    layout->addWidget(editInDir, 2, 1);
    connect(editInDir, &QLineEdit::textChanged, [&](const QString &t){ settings.setInputDir(t); });

    // c) Путь вывода
    QPushButton *btnOutDir = new QPushButton("Выбрать папку вывода");
    const auto outputDir = settings.get().outputDir;
    QLineEdit *editOutDir = new QLineEdit(QDir(outputDir).exists() ? outputDir : QDir::currentPath());
    connect(btnOutDir, &QPushButton::clicked, [this, editOutDir](){
        QString dir = QFileDialog::getExistingDirectory(this, "Папка вывода");
        if (!dir.isEmpty()) {
            editOutDir->setText(dir);
            settings.setOutputDir(dir);
        }
    });
    layout->addWidget(btnOutDir, 3, 0);
    layout->addWidget(editOutDir, 3, 1);
    connect(editOutDir, &QLineEdit::textChanged, [&](const QString &t){ settings.setOutputDir(t); });

    // d) При дубликате
    QLabel *lblAction = new QLabel("При дубликате:");
    QComboBox *comboAction = new QComboBox;
    comboAction->addItems({"Перезапись", "Добавить счетчик"});
    layout->addWidget(lblAction, 4, 0);
    layout->addWidget(comboAction, 4, 1);
    comboAction->setCurrentIndex(settings.get().overwrite ? 0 : 1);
    connect(comboAction, &QComboBox::currentIndexChanged, [&](int index){ settings.setNeedOverride(index == 0); });

    // e,f) Таймер
    {
        QCheckBox *chkTimer = new QCheckBox("По таймеру (сек):");
        chkTimer->setChecked(settings.get().timerMode);
        QLineEdit *editInterval = new QLineEdit(QString::number(settings.get().pollIntervalMs / 1000));
        QIntValidator *validator = new QIntValidator(0, 1000, this);
        editInterval->setValidator(validator);
        layout->addWidget(chkTimer, 5, 0);
        layout->addWidget(editInterval, 5, 1);
        connect(chkTimer, &QCheckBox::toggled, [&](bool onTimer){ settings.setOnTimer(onTimer); });
        connect(editInterval, &QLineEdit::textChanged, [&](const QString &t){ settings.setPollIntervalMs(t.toInt() * 1000); });
    }

    // g) Ключ 8 байт
    {
        QLabel *lblKey = new QLabel("Ключ (hex, 16 символов):");
        const auto strXorKey = QString("%1")
                                   .arg(settings.get().xorKey, 16, 16, QLatin1Char('0'))
                                   .toUpper();
        QLineEdit *editKey = new QLineEdit(strXorKey);
        QRegularExpression re("^[0-9A-Fa-f]{0,16}$");  // 0-16 hex цифр
        QRegularExpressionValidator *validator = new QRegularExpressionValidator(re, this);
        editKey->setValidator(validator);
        editKey->setInputMask("HHHHHHHHHHHHHHHH");
        layout->addWidget(lblKey, 6, 0);
        layout->addWidget(editKey, 6, 1);
        connect(editKey, &QLineEdit::textChanged, [&](const QString &t){
            settings.setXorKey(t.toULongLong(nullptr, 16));
        });
    }

    // Старт
    buttonStart = new QPushButton("Старт обработки");
    layout->addWidget(buttonStart, 7, 0, 1, 2);
    connect(buttonStart, &QPushButton::clicked, this, &MainWindow::onStartClicked);

    // Стоп
    buttonStop = new QPushButton("Остановить");
    buttonStop->setEnabled(false);
    layout->addWidget(buttonStop, 8, 0, 1, 2);
    connect(buttonStop, &QPushButton::clicked, this, &MainWindow::onStopClicked);

    // Статус
    QLabel *lblStatus = new QLabel("");
    layout->addWidget(lblStatus, 9, 0, 1, 2);

    resize(450, 400);
}

void MainWindow::getMatchFilesInDir(const QString &path) {
    QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        const auto fi = it.fileInfo();
        if (fi.isFile() && matchesMask(fi.absoluteFilePath())) {
            filesToProcess[fi.absoluteFilePath()] = fi.lastModified();
        }
        it.next();
    }
    const auto fi = it.fileInfo();
    if (fi.isFile() && matchesMask(fi.absoluteFilePath())) {
        filesToProcess[fi.absoluteFilePath()] = fi.lastModified();
    }
}

void MainWindow::updateMatchFilesInDir(const QString &path) {
    QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        const auto fi = it.fileInfo();
        if (fi.isFile() && matchesMask(fi.absoluteFilePath())) {
            // new file - added since last process
            if (!processedFiles.contains(fi.absoluteFilePath())) {
                if (!filesToProcess.contains(fi.absoluteFilePath())) {
                    filesToProcess[fi.absoluteFilePath()] = fi.lastModified();
                }
            } else {
                // file was changed - need to process again
                if (processedFiles[fi.absoluteFilePath()] != fi.lastModified()){
                    filesToProcess[fi.absoluteFilePath()] = fi.lastModified();
                }
            }
        }
        it.next();
    }
}

void MainWindow::processMatchFiles() {
    if (filesToProcess.size() == 0) {
        {
           QMessageBox::information(this, "Информация", "Не найдено подходящих файлов в директории!");
           isProcessing = false;
           onStatusUpdate();
           return;
        }
    }
    initProgress();
    for (const auto file: filesToProcess.keys()) {
        const auto worker = new FileWorker(settings.get(), file);
        pool->start(worker);
        connect(worker, &FileWorker::finished, this, &MainWindow::onProgress, Qt::QueuedConnection);
        connect(worker, &FileWorker::error, this, &MainWindow::onError, Qt::QueuedConnection);
    }
    pool->waitForDone();
}

void MainWindow::onStopClicked() {
    if (workCopySettings.timerMode) {
        timer->stop();
        timer->disconnect();
    } else {
        if (!isProcessing)
            return;
    }
    isProcessing = false;

    onStatusUpdate();
    onFinished(true, "Отмена");
    stopAllWorkers();
}

void MainWindow::onStartClicked()
{
    if (isProcessing)
        return;

    isProcessing = true;
    onStatusUpdate();
    if (workCopySettings != AppSettings(settings.get())) {
        // clear caches
        processedFiles.clear();
    }
    workCopySettings = settings.get();

    if (workCopySettings.timerMode) {
        timer->start(workCopySettings.pollIntervalMs);
        connect(timer, &QTimer::timeout, this, &MainWindow::onTimerTimeout);
    }
    processInputDir();
}

void MainWindow::processInputDir()
{
    filesToProcess.clear();
    if (workCopySettings.inDir != lastInputPath){
        lastInputPath = workCopySettings.inDir;
        // clear caches
        processedFiles.clear();
        getMatchFilesInDir(workCopySettings.inDir);
    } else {
        updateMatchFilesInDir(workCopySettings.inDir);
    }
    processMatchFiles();
}

void MainWindow::onTimerTimeout()
{
    processInputDir();
}

bool MainWindow::matchesMask(const QString &fileName)
{
    QString mask = workCopySettings.mask;
    mask = mask.replace("*", "");
    return fileName.contains(mask, Qt::CaseInsensitive);
}

void MainWindow::initProgress() {
    progressDialog = std::make_unique<QProgressDialog>("Обработка файла...", "Отмена", 0, filesToProcess.size(), nullptr);
    progressDialog->setWindowFlags(progressDialog->windowFlags() & ~Qt::WindowCloseButtonHint);
    progressDialog->setMinimumDuration(2000);
    progressDialog->setAutoReset(false);
    progressDialog->setAutoClose(false);
    progressDialog->setMaximum(filesToProcess.size());
}

void MainWindow::onProgress()
{
    if (filesToProcess.size() < 1)
        return;
    if (sender()){
        auto worker = dynamic_cast<FileWorker*>(sender());
        if (worker) {
            processedFiles[worker->filePath()] = QFileInfo(worker->filePath()).lastModified();
        }
    }
    progressDialog->setValue(progressDialog->value() + 1);
    if (progressDialog->value() == filesToProcess.size() - 1) {
        onFinished(true, "Все файлы были обработаны");
    }
}

void MainWindow::onError(const QString &error)
{
    onFinished(false, error);
}

void MainWindow::onFinished(bool success, const QString &status)
{
    if (progressDialog) {
        progressDialog->disconnect();
        progressDialog->close();
        progressDialog = nullptr;
    }
    isProcessing = false;
    onStatusUpdate();
    update();
    if (success) {
        statusBar()->showMessage(status, 5000);
    } else {
        QMessageBox::warning(this, "Ошибка", status);
    }
}

void MainWindow::onStatusUpdate()
{
    if (progressDialog && !isProcessing)
    {
        progressDialog->close();
        progressDialog = nullptr;
    }
    const bool isWorking = isProcessing || timer->isActive();
    buttonStart->setEnabled(!isWorking);
    buttonStop->setEnabled(isWorking);
    statusBar()->showMessage(isProcessing ? "Процесс обработки запущен" : (timer->isActive() ? "Обработка окончена. Активна обработка по таймеру" : "Обработка окончена"), 3000);
}