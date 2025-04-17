#include "include/SettingsLoader.hpp"

#include <QFileInfo>
#include <QFile>
#include <QStringList>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QSettings>

SettingsLoader *SettingsLoader::ptr = NULL;

std::shared_ptr<SettingsLoader> SettingsLoader::getInstace()
{
    if(ptr == NULL)
        ptr = new SettingsLoader();

    return std::shared_ptr<SettingsLoader>(ptr);
}

bool SettingsLoader::isLoadingProxyList()
{
    QFileInfo checkTextFile(QString("./%1.txt").arg(_nameFileProxyList));
    QFileInfo checkJsonFile(QString("./%1.json").arg(_nameFileProxyList));

    if (!_vecProxySettings.isEmpty())
        _vecProxySettings.clear();
    
    if(checkTextFile.exists() && checkTextFile.isFile()){
        
        if(parseTextFile(checkTextFile.fileName()))
            return true;

    }else if(checkJsonFile.exists() && checkJsonFile.isFile()){

        if(parseJsonFile(checkJsonFile.fileName()))
            return true;
    }

    return false;
}

bool SettingsLoader::isLoadingProgramSettings()
{
    QFileInfo checkSettingsFile(QString("./%1.ini").arg(_nameFileSettings));

    if (checkSettingsFile.exists() && checkSettingsFile.isFile()) {
        
        if(parseSettingsFile(checkSettingsFile.fileName()))
            return true;

    }

    return false;
}

QVector<std::shared_ptr<ProxySettings>> SettingsLoader::getProxyServersList()
{
    if (isLoadingProxyList())
        return _vecProxySettings;

    return QVector<std::shared_ptr<ProxySettings>>();
}

std::shared_ptr<Settings> SettingsLoader::getProgramSettings()
{
    if (isLoadingProgramSettings())
        return _settings;

    return std::shared_ptr<Settings>();
}

SettingsLoader::SettingsLoader()
{

}

bool SettingsLoader::parseTextFile(const QString& path)
{
    QString contentFile = readTextFile(path);

    if(!contentFile.isEmpty()){
        QStringList contentFileList = contentFile.split("\n");
        QStringList tmpContentList;

        for (QString content : contentFileList) {
            if (!content.isEmpty()) {
                tmpContentList.append(content.section("http://", 1));
            }
        }

        for (QString tmpString : tmpContentList) {
            if (!tmpString.isEmpty()) {
                _proxySettings = std::make_shared<ProxySettings>();
                _proxySettings->host = tmpString.section(":", 0, 0);
                _proxySettings->port = tmpString.section(":", 1).toInt();

                _vecProxySettings.append(_proxySettings);
            }
        }

        return true;
    }

    return false;
}

bool SettingsLoader::parseJsonFile(const QString& path)
{
    QJsonParseError jsonError;

    QJsonDocument jsonDocument = QJsonDocument::fromJson(readTextFile(path).toUtf8(), &jsonError);

    if (jsonError.error == QJsonParseError::NoError) {
        QJsonObject jsonRootObject = jsonDocument.object();
        QJsonArray jsonArray = jsonRootObject["HttpProxyServers"].toArray();

        for (auto jsonItem : jsonArray) {
            if (jsonItem.isObject()) {
                QJsonObject jsonObject = jsonItem.toObject();

                _proxySettings = std::make_shared<ProxySettings>();

                _proxySettings->host = jsonObject["address"].toString();
                _proxySettings->port = jsonObject["port"].toInt();
                _proxySettings->user = jsonObject["user"].toString();
                _proxySettings->password = jsonObject["password"].toString();

                _vecProxySettings.append(_proxySettings);
            }
        }

        return true;
    }
        
    return false;
}

bool SettingsLoader::parseSettingsFile(const QString &path)
{
    if (!_settings) {
        _settings = std::make_shared<Settings>();
    }
    else {
        _settings->reset();
    }

    QSettings settingsFile(path, QSettings::IniFormat);

    if (settingsFile.status() == QSettings::Status::NoError) {
        settingsFile.beginGroup("Main");
        _settings->portListen = settingsFile.value("PortListen", 7777).toInt();
        _settings->testingProxyServers = settingsFile.value("TestingProxyServers", "no").toString() == "yes" ? true : false;
        _settings->proxyVerificationInterval = settingsFile.value("ProxyVerificationInterval", 0).toInt();
        _settings->maxTimeWaitReply = settingsFile.value("MaxTimeWaitReply", 60).toInt();

        return true;
    }

    return false;
}

QString SettingsLoader::readTextFile(const QString &path)
{
    QFile file(path);
    QString content;

    if(file.open(QIODevice::Text|QIODevice::ReadOnly)){
        content = file.readAll();
        file.close();

        return content;
    }

    return content;
}

SettingsLoader::~SettingsLoader()
{

}
