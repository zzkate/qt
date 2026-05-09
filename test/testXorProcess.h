#ifndef TEST_XOR_PROCESS_H
#define TEST_XOR_PROCESS_H

#include <QtTest>
#include "../fileworker.h"
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QTest>

typedef std::shared_ptr<QTemporaryDir> QTemporaryDirPtr;

class TestXorProcess : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();
    void testXorSimple();
    void testOverwriteFalse();
    void onCompleted();

    QStringList dirEntry(QTemporaryDirPtr dir, bool abs = true);
    void checkXOR(const QString& outputPath, const QString& inputPath);

private:
    QTemporaryDirPtr inDir;
    QTemporaryDirPtr outDir;
    AppSettings mockSettings;

    void fillDir(QTemporaryDirPtr dir);
    void createRandomBinaryFile(const QString &fileName, qint64 sizeInBytes);
    void mockProcessFiles(const QStringList filesToProcess);
};

#endif // TEST_XOR_PROCESS_H