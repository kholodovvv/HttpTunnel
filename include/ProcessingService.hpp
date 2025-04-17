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
    void slotRunService();
    void slotStopService();
    void slotRestartService();
    void slotExitService();
    void slotFinishedTestingProxy();

private:
    std::shared_ptr<SettingsLoader> _settingsLoader;
    std::shared_ptr<Settings> _settings;
    std::shared_ptr<HttpTransceiver> _httpTranseiver;
    std::unique_ptr<QThread> _threadLoopProcessCommand;
    std::unique_ptr<LoopProcessCommand> _loopProcessCommand;
};

class LoopProcessCommand : public QObject
{
    Q_OBJECT

public:
    explicit LoopProcessCommand();
    ~LoopProcessCommand();

signals:
    void runService();
    void stopService();
    void restartService();
    void exitService();

public slots:
    void slotLoopWaitCommand();
};
