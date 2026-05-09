#include "settings.h"
#include <QSettings>
#include <QDir>

enum SETTINGS {
    MASK,
    DEL_SRC,
    OUT_DIR,
    IN_DIR,
    OVERWRITE,
    TIMER,
    INTERVAL,
    XOR_KEY,
    COUNT
};

const QString SettingsKeys[SETTINGS::COUNT] = {
    "mask",
    "deleteSource",
    "outputDir",
    "inDir",
    "overwrite",
    "timerMode",
    "pollIntervalMs",
    "xorKey"};

Settings::Settings() {}

void Settings::saveSettings() {
    st.setValue(SettingsKeys[MASK], s.mask);
    st.setValue(SettingsKeys[DEL_SRC], s.deleteSource);
    st.setValue(SettingsKeys[OUT_DIR], s.outputDir);
    st.setValue(SettingsKeys[IN_DIR], s.inDir);
    st.setValue(SettingsKeys[OVERWRITE], s.overwrite);
    st.setValue(SettingsKeys[TIMER], s.timerMode);
    st.setValue(SettingsKeys[INTERVAL], s.pollIntervalMs);
    st.setValue(SettingsKeys[XOR_KEY], QVariant::fromValue<qulonglong>(s.xorKey));
    st.sync();
}

void Settings::loadSettings() {
    s.mask = st.value(SettingsKeys[MASK], "*.bin").toString();
    s.deleteSource = st.value(SettingsKeys[DEL_SRC], false).toBool();
    s.inDir = st.value(SettingsKeys[IN_DIR], QDir::homePath()).toString();
    s.outputDir = st.value(SettingsKeys[OUT_DIR], QDir::homePath()).toString();
    s.overwrite = st.value(SettingsKeys[OVERWRITE], true).toBool();
    s.timerMode = st.value(SettingsKeys[TIMER], false).toBool();
    s.pollIntervalMs = st.value(SettingsKeys[INTERVAL], 5000).toInt();
    s.xorKey = st.value(SettingsKeys[XOR_KEY], QVariant::fromValue<qulonglong>(0)).toULongLong();
}

void Settings::sync(){
    st.sync();
}

void Settings::setMask(const QString& mask){
    s.mask = mask;
}

void Settings::setDeleteSource(bool flag){
    s.deleteSource = flag;
}

void Settings::setOutputDir(const QString& dir){
    s.outputDir = dir;
}

void Settings::setInputDir(const QString& dir){
    s.inDir = dir;
}

void Settings::setNeedOverride(bool needOverride){
    s.overwrite = needOverride;
}

void Settings::setOnTimer(bool onTimer){
    s.timerMode = onTimer;
}

void Settings::setPollIntervalMs(int interval){
    s.pollIntervalMs = interval;
}

void Settings::setXorKey(quint64 xorKey){
    s.xorKey = xorKey;
}