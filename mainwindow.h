#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QProgressDialog>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QGridLayout>
#include <QFileDialog>
#include <QThreadPool>
#include "settings.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class FileWorker;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onStartClicked();
    void onStopClicked();
    void processInputDir();
    void onTimerTimeout();
    void onProgress();
    void onFinished(bool success, const QString &status);
    void onError(const QString &error);
    void onStatusUpdate();

private:
    void setupUI();
    bool matchesMask(const QString &fileName);
    void getMatchFilesInDir(const QString &path);
    void updateMatchFilesInDir(const QString &path);
    void processMatchFiles();
    void stopAllWorkers();
    void initProgress();

    QPushButton *buttonStart = nullptr;
    QPushButton *buttonStop = nullptr;

    Settings settings;
    AppSettings workCopySettings;
    QFileSystemWatcher *watcher;
    QTimer *timer;
    QThreadPool *pool;
    std::unique_ptr<QProgressDialog> progressDialog;
    QMap<QString, QDateTime> processedFiles;
    QMap<QString, QDateTime> filesToProcess;
    QString lastInputPath;
    bool isProcessing = false;
};

#endif // MAINWINDOW_H