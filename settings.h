#ifndef SETTINGS_H
#define SETTINGS_H

#include <QString>
#include <QSettings>

struct AppSettings {
    QString mask = ".txt";
    bool deleteSource = false;
    QString outputDir = "/tmp";
    QString inDir = "/tmp";
    bool overwrite = true;
    bool timerMode = false;
    int pollIntervalMs = 5000;
    quint64 xorKey = 0;

    bool operator==(const AppSettings& other) const {
        return mask == other.mask &&
                deleteSource == other.deleteSource &&
                outputDir == other.outputDir &&
                inDir == other.inDir &&
                overwrite == other.overwrite &&
                timerMode == other.timerMode &&
                pollIntervalMs == other.pollIntervalMs &&
                xorKey == other.xorKey;
    }

    bool operator!=(const AppSettings& other) const {
        return !(*this == other);
    }
};

class Settings
{
    public:
        Settings();
        void saveSettings();
        void loadSettings();
        void sync();

        const AppSettings& get(){ return s; }

        void setMask(const QString& mask);
        void setDeleteSource(bool flag);
        void setOutputDir(const QString& dir);
        void setInputDir(const QString& dir);
        void setNeedOverride(bool needOverride);
        void setOnTimer(bool onTimer);
        void setPollIntervalMs(int interval);
        void setXorKey(quint64 xorKey);

    private:
        AppSettings s;
        QSettings st = QSettings("MyCompany", "XorTool");
};

#endif // SETTINGS_H
