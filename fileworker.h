#ifndef FILEWORKER_H
#define FILEWORKER_H

#include <QObject>
#include <QRunnable>
#include <QMutex>
#include <QString>
#include "settings.h"

class FileWorker: public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit FileWorker(const AppSettings& appSettings, const QString &path);
    [[nodiscard]] const QString& filePath() const { return inputPath; }

public slots:
    void cancel();
    void run() override;

signals:
    void finished();
    void error(const QString &msg);

private slots:
    void doProcess();

private:
    AppSettings settings;
    const QString inputPath;
    bool stop = false;

 friend class TestXorProcess;
};

#endif // FILEWORKER_H