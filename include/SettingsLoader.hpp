/*
 * SettingsLoader отвечает за загрузку списков прокси серверов
 * из txt или json файла
 *
 */

#pragma once

#include <memory>
#include <QString>
#include <QPair>
#include <QStringList>

class SettingsLoader
{

public:
    static std::shared_ptr<SettingsLoader> getInstace();
    SettingsLoader(SettingsLoader const&) = delete;
    SettingsLoader& operator=(const SettingsLoader&) = delete;
    bool isLoadingSettings();
    QPair<QStringList, QStringList> getProxyServersList();
    ~SettingsLoader();

private:
    SettingsLoader();
    QPair<QStringList, QStringList> parseTextFile(const QString&);
    QPair<QStringList, QStringList> parseJsonFile(const QString&);
    QString readTextFile(const QString&);

private:
    static SettingsLoader *ptr;
    const QString _nameFileSettings = "proxylist";
    QPair<QStringList, QStringList> _proxyServersList;
};
