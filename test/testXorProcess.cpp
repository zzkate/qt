#include "testXorProcess.h"
#include <memory>
#include <QFile>
#include <QRandomGenerator>
#include <QByteArray>
#include <QDebug>

void TestXorProcess::createRandomBinaryFile(const QString &fileName, qint64 sizeInBytes) {
    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not open file for writing:" << file.errorString();
        return;
    }

    QByteArray buffer;
    buffer.resize(sizeInBytes);

    // Fill the buffer with random 32-bit values (efficiently fills bytes)
    // Note: sizeInBytes should ideally be a multiple of 4 for fillRange
    quint32 *dataPtr = reinterpret_cast<quint32*>(buffer.data());
    QRandomGenerator::global()->fillRange(dataPtr, sizeInBytes / sizeof(quint32));

    file.write(buffer);
    file.close();

    qDebug() << "Successfully created" << fileName << "with" << sizeInBytes << "random bytes.";
}

void TestXorProcess::fillDir(QTemporaryDirPtr dir)
{
    int countFiles = QRandomGenerator::global()->bounded(10, 100);
    for (auto i = 0; i < countFiles; ++i) {
        const auto fName = dir->path() + QDir::separator() + QString::number(i) + ".bin";
        createRandomBinaryFile(fName, QRandomGenerator::global()->bounded(250, 1800));
    }
}

QStringList TestXorProcess::dirEntry(QTemporaryDirPtr dir, bool abs) {
    const auto dirEntry = QDir(dir->path()).entryList(QDir::Files | QDir::NoDotAndDotDot);
    if (abs) {
        QStringList absPathsList;
        for (const auto file: dirEntry) {
            absPathsList.append(dir->path() + QDir::separator() + file);
        }
        return absPathsList;
    }
    return dirEntry;
}

void TestXorProcess::checkXOR(const QString& inputPath, const QString& outputPath) {
    QFile input(inputPath);
    QVERIFY(input.open(QIODevice::ReadOnly));
    QByteArray original = input.readAll();

    QFile output(outputPath);
    QVERIFY(output.open(QIODevice::ReadOnly));
    QByteArray result = output.readAll();
    QCOMPARE(result.size(), original.size());

    char keyBytes[8];
    qToBigEndian(mockSettings.xorKey, reinterpret_cast<uchar*>(keyBytes));
    QByteArray expected = original;
    for (int i = 0; i < expected.size(); ++i) {
        expected[i] ^= keyBytes[i % 8];
    }
    QCOMPARE(result, expected);
}

void TestXorProcess::onCompleted() {
    if (mockSettings.deleteSource) {
        // check if in inDir there are no one file from OutDir
        for (const auto file : dirEntry(outDir)) {
            QVERIFY(!dirEntry(inDir).contains(file));
        }
    }
    else {
        if (mockSettings.overwrite) {
            // check 1 file XOR
            const auto outFile = dirEntry(outDir)[0];
            bool findOrigin = false;
            for (const auto& inFile : dirEntry(inDir)) {
                if (QFileInfo(inFile).fileName() == QFileInfo(outFile).fileName()) {
                    checkXOR(inFile, outFile);
                    findOrigin = true;
                }
            }
            QVERIFY(findOrigin == true);
        }
        else {
            // check if we have files with counters
            QVERIFY(QFile::exists(outDir->path() + "/0_1.bin"));
            QVERIFY(QFile::exists(outDir->path() + "/1_1.bin"));
            QVERIFY(!QFile::exists(outDir->path() + "/1_2.bin"));
        }
    }
}

void TestXorProcess::mockProcessFiles(const QStringList filesToProcess) {
    for (const auto file: filesToProcess) {
        // run sync in test
        std::unique_ptr<FileWorker> worker(new FileWorker(mockSettings, file));
        worker->doProcess();
    }
    onCompleted();
}

void TestXorProcess::initTestCase()
{
    inDir = std::make_shared<QTemporaryDir>();
    QVERIFY(inDir->isValid());
    fillDir(inDir);
    QVERIFY(dirEntry(inDir).size() >= 10);
    outDir = std::make_shared<QTemporaryDir>();
    QVERIFY(outDir->isValid());

    mockSettings.inDir = inDir->path();
    mockSettings.outputDir = outDir->path();
    mockSettings.mask = "*.bin";
    mockSettings.xorKey = 0xf00000000010;
}

void TestXorProcess::cleanupTestCase()
{
}

void TestXorProcess::testXorSimple()
{
    mockProcessFiles(dirEntry(inDir));
}

void TestXorProcess::testOverwriteFalse()
{
    // use duplicate files counter
    mockSettings.overwrite = false;
    // cp files from in to out
    for (const auto file: dirEntry(inDir, false)) {
        QString srcFile = inDir->path() + "/" + file;
        QString dstFile = outDir->path() + "/" + file;

        // QFile::copy fails if the destination file already exists
        if (QFile::exists(dstFile)) {
            QFile::remove(dstFile);
        }
        QVERIFY(QFile::copy(srcFile, dstFile));
    }
    mockProcessFiles(dirEntry(inDir));
}