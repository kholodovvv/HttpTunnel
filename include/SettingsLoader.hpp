#pragma once

#include <memory>
#include <QString>
#include <QVector>

struct ProxySettings {
    QString host = "";
    uint port = 0;
    QString user = "";
    QString password = "";
};

struct Settings {
    uint portListen = 0;
    bool testingProxyServers = false;
    uint proxyVerificationInterval = 0;
    uint maxTimeWaitReply = 0;

    void reset() {
        portListen = 0;
        testingProxyServers = false;
        proxyVerificationInterval = 0;
        maxTimeWaitReply = 0;
    }
};

class SettingsLoader
{

public:
    static std::shared_ptr<SettingsLoader> getInstace();
    SettingsLoader(SettingsLoader const&) = delete;
    SettingsLoader& operator=(const SettingsLoader&) = delete;
    QVector<std::shared_ptr<ProxySettings>> getProxyServersList();
    std::shared_ptr<Settings> getProgramSettings();
    ~SettingsLoader();

private:
    SettingsLoader();
    bool isLoadingProxyList();
    bool isLoadingProgramSettings();
    bool parseTextFile(const QString&);
    bool parseJsonFile(const QString&);
    bool parseSettingsFile(const QString&);
    QString readTextFile(const QString&);

private:
    static SettingsLoader *ptr;
    const QString _nameFileProxyList = "proxylist";
    const QString _nameFileSettings = "settings";
    std::shared_ptr<ProxySettings> _proxySettings;
    std::shared_ptr<Settings> _settings;
    QVector<std::shared_ptr<ProxySettings>> _vecProxySettings;
};
