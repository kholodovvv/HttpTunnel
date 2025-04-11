/*
 * ProcessingService отвечает за взаимодействие пользователя с программой.
 * LoopProcessCommand цикл ожидающий команд пользователя.
 *
 */

#pragma once

#include <QObject>
#include <QPair>
#include <QThread>
#include <memory>

#include "include/HttpTransceiver.hpp"

class QString;
class QStringList;
class SettingsLoader;
class LoopProcessCommand;

class ProcessingService: public QObject
{
    Q_OBJECT

public:
    explicit ProcessingService(QObject *parent = nullptr);
    ProcessingService(ProcessingService const&) = delete;
    ProcessingService& operator=(const ProcessingService&) = delete;
    ~ProcessingService();

private slots:
    void slotHttpServiceRunning(const int &port);
    void slotHttpServiceStopped();
    void slotShowUserMessage(const QString &message);
    void slotError(const QString &message);

public slots:
    void runService();
    void stopService();
    void restartService();
    void exitService();

private:
    std::shared_ptr<SettingsLoader> _settingsLoader;
    QPair<QStringList, QStringList> _listProxyServers;
    std::shared_ptr<HttpTransceiver> _httpTranseiver;
    std::shared_ptr<QThread> thread;
    std::shared_ptr<LoopProcessCommand> mainLoop;
};

class LoopProcessCommand : public QObject
{
    Q_OBJECT

public:
    explicit LoopProcessCommand(QObject* parent = nullptr);
    ~LoopProcessCommand();

signals:
    void runService();
    void stopService();
    void restartService();
    void exitService();

public slots:
    void slotLoopWaitCommand();
};
