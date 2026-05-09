#include "fileworker.h"
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QThread>
#include <QtEndian>

const int CHUNK_SIZE = 65536;
const int SIZE_OF_BYTE = 8;
const int SLEEP_MS = 10;

FileWorker::FileWorker(const AppSettings &appSettings, const QString &path):
    settings(appSettings),
    inputPath(path) {
    setAutoDelete(true);
}

void FileWorker::run() {
    doProcess();
}

void FileWorker::doProcess()
{
    QFile inputFile(inputPath);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        emit error("Ошибка открытия файла");
        return;
    }

    QFileInfo fi(inputFile);
    QString outPath = settings.outputDir + "/" + fi.fileName();
    if (QFile::exists(outPath) && !settings.overwrite) {
        QString base = fi.baseName();
        QString ext = fi.completeSuffix();
        int counter = 1;
        do {
            outPath = settings.outputDir + "/" + base + "_" + QString::number(counter) + "." + ext;
            ++counter;
        } while (QFile::exists(outPath));
    }

    QFile outFile(outPath);
    if (!outFile.open(QIODevice::WriteOnly)) {
        emit error("Ошибка создания файла");
        return;
    }

    char keyBytes[SIZE_OF_BYTE];
    qToBigEndian(settings.xorKey, reinterpret_cast<uchar*>(keyBytes));

    while (!inputFile.atEnd()) {
        QByteArray chunk = inputFile.read(CHUNK_SIZE); // 64KB chunk
        for (int i = 0; i < chunk.size(); ++i) {
            chunk[i] ^= keyBytes[i % SIZE_OF_BYTE];
        }
        outFile.write(chunk);
        if (stop) {
            return;
        }
        QThread::msleep(SLEEP_MS); // Yield для UI
    }

    inputFile.close();
    outFile.close();

    if (settings.deleteSource && QFile::exists(inputPath)) {
        QFile::remove(inputPath);
    }

    emit finished();
}

void FileWorker::cancel(){
    stop = true;
}