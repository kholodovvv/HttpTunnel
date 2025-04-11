#include "include/SettingsLoader.hpp"

#include <QFileInfo>
#include <QFile>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

SettingsLoader *SettingsLoader::ptr = NULL;

std::shared_ptr<SettingsLoader> SettingsLoader::getInstace()
{
    if(ptr == NULL)
        ptr = new SettingsLoader();

    return std::shared_ptr<SettingsLoader>(ptr);
}

bool SettingsLoader::isLoadingSettings()
{
    QFileInfo checkTextFile(QString("./%1.txt").arg(_nameFileSettings));
    QFileInfo checkJsonFile(QString("./%1.json").arg(_nameFileSettings));

    if(checkTextFile.exists() && checkTextFile.isFile()){
        _proxyServersList = parseTextFile(checkTextFile.fileName());

        if (!_proxyServersList.first.isEmpty())
            return true;

    }else if(checkJsonFile.exists() && checkJsonFile.isFile()){

        _proxyServersList = parseJsonFile(checkJsonFile.fileName());

        if (!_proxyServersList.first.isEmpty())
            return true;
    }

    return false;
}

QPair<QStringList, QStringList> SettingsLoader::getProxyServersList()
{
    return _proxyServersList;
}

SettingsLoader::SettingsLoader()
{

}

QPair<QStringList, QStringList> SettingsLoader::parseTextFile(const QString &path)
{
    QString contentFile = readTextFile(path);
    QPair<QStringList, QStringList> pairProxyList;

    if(!contentFile.isEmpty()){
        QStringList contentFileList = contentFile.split("\n");
        QStringList serverNameList;
        QStringList serverPortList;
        QStringList tmpContentList;

        for (QString content : contentFileList) {
            if (!content.isEmpty()) {
                tmpContentList.append(content.section("http://", 1));
            }
        }

        for (QString tmpString : tmpContentList) {
            if (!tmpString.isEmpty()) {
                serverNameList.append(tmpString.section(":", 0, 0));
                serverPortList.append(tmpString.section(":", 1));
            }
        }

        pairProxyList.first.append(serverNameList);
        pairProxyList.second.append(serverPortList);
    }


    return pairProxyList;
}

QPair<QStringList, QStringList> SettingsLoader::parseJsonFile(const QString &path)
{
    QPair<QStringList, QStringList> pairProxyList;
    QStringList serverNameList;
    QStringList serverPortList;
    QJsonParseError jsonError;

    QJsonDocument jsonDocument = QJsonDocument::fromJson(readTextFile(path).toUtf8(), &jsonError);

    if (jsonError.error == QJsonParseError::NoError) {
        QJsonObject jsonRootObject = jsonDocument.object();
        QJsonArray jsonArray = jsonRootObject["HttpProxyServers"].toArray();

        for (auto jsonItem : jsonArray) {
            if (jsonItem.isObject()) {
                QJsonObject jsonObject = jsonItem.toObject();

                serverNameList.append(jsonObject["address"].toString());
                serverPortList.append(jsonObject["port"].toString());
            }
        }
    }
        
    if (!serverNameList.isEmpty() && !serverPortList.isEmpty()) {
        pairProxyList.first = serverNameList;
        pairProxyList.second = serverPortList;
    }

    return pairProxyList;
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
