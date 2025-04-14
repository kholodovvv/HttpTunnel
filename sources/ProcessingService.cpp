#include "include/ProcessingService.hpp"
#include "include/SettingsLoader.hpp"

#include <iostream>
#include <string>
#include <memory>
#include <QString>
#include <QStringList>

ProcessingService::ProcessingService(QObject *parent): QObject(parent)
{
    _settingsLoader = SettingsLoader::getInstace();
    _httpTranseiver = std::make_shared<HttpTransceiver>();

    connect(_httpTranseiver.get(), &HttpTransceiver::signalRunning, this, &ProcessingService::slotHttpServiceRunning);
    connect(_httpTranseiver.get(), &HttpTransceiver::signalStopped, this, &ProcessingService::slotHttpServiceStopped);
    connect(_httpTranseiver.get(), &HttpTransceiver::signalTestingProxyFinished, this, &ProcessingService::slotFinishedTestingProxy);
    connect(_httpTranseiver.get(), &HttpTransceiver::signalMessage, this, &ProcessingService::slotShowUserMessage);
    connect(_httpTranseiver.get(), &HttpTransceiver::signalError, this, &ProcessingService::slotError);

    _threadLoopProcessCommand = std::make_unique<QThread>();
    _loopProcessCommand = std::make_unique<LoopProcessCommand>();

    if(_threadLoopProcessCommand && _loopProcessCommand){
        _loopProcessCommand->moveToThread(_threadLoopProcessCommand.get());
        connect(_threadLoopProcessCommand.get(), &QThread::started, _loopProcessCommand.get(), &LoopProcessCommand::slotLoopWaitCommand, Qt::AutoConnection);
        connect(_loopProcessCommand.get(), &LoopProcessCommand::runService, this, &ProcessingService::slotRunService, Qt::AutoConnection);
        connect(_loopProcessCommand.get(), &LoopProcessCommand::stopService, this, &ProcessingService::slotStopService, Qt::AutoConnection);
        connect(_loopProcessCommand.get(), &LoopProcessCommand::restartService, this, &ProcessingService::slotRestartService, Qt::AutoConnection);
        connect(_loopProcessCommand.get(), &LoopProcessCommand::exitService, this, &ProcessingService::slotExitService, Qt::AutoConnection);
    }
}

void ProcessingService::slotRunService()
{
    QPair<QStringList, QStringList> proxyServersList;

    if (_httpTranseiver && _httpTranseiver->getState() == HttpTransceiverState::notRunning) {

        if (!_settingsLoader->isLoadingSettings()) {
            slotShowUserMessage(QString("Couldn't load list proxy servers"));
            qInfo() << "Couldn't load list proxy servers";

        }
        else {
            slotShowUserMessage(QString("The list of proxy servers has been loaded successfully ..."));
            qInfo() << "The list of proxy servers has been loaded successfully";

        }

        proxyServersList = _settingsLoader->getProxyServersList();

        _httpTranseiver->setTestingProxy(false);
        _httpTranseiver->setSettings(proxyServersList, 7777, 60, 300);

        slotShowUserMessage(QString("The http service is being started, please wait ..."));
        qInfo() << "Startup http service ...";
        _httpTranseiver->run();
        
    }
    else {
        slotShowUserMessage(QString("The service is already running or has not been stopped yet!"));
    }
}

void ProcessingService::slotStopService()
{
    if (_httpTranseiver) {
        if (_httpTranseiver->getState() != HttpTransceiverState::notRunning)
            _httpTranseiver->stop();
    }
    else {
        slotShowUserMessage(QString("The http service has already been stopped ..."));
    }
}

void ProcessingService::slotRestartService()
{
    if (_httpTranseiver) {
        if (_httpTranseiver->getState() != HttpTransceiverState::notRunning)
            _httpTranseiver->stop();
    }

    if (_httpTranseiver->getState() == HttpTransceiverState::notRunning) {
        slotRunService();
    }
    else {
        slotShowUserMessage(QString("The service could not stop in time and therefore was not started. "
                                    "Try starting the service a little later with the 'start' command."));
        qWarning() << "The service could not stop in time and therefore was not started.";
    }

}

void ProcessingService::slotExitService()
{
    if (_httpTranseiver) {
        if (_httpTranseiver->getState() != HttpTransceiverState::notRunning) {

            _httpTranseiver->stop();
        }
    }

    if (_threadLoopProcessCommand)
        if (_threadLoopProcessCommand->isRunning())
            _threadLoopProcessCommand->quit();

    exit(0);
}

void ProcessingService::slotFinishedTestingProxy()
{
    slotShowUserMessage(QString("Testing of proxy servers is completed!"));
    qInfo() << "Testing of proxy servers is completed!";
}

ProcessingService::~ProcessingService()
{

}

void LoopProcessCommand::slotLoopWaitCommand()
{
    std::string command;

    while (command != "exit") {

        std::cout << std::endl;
        std::cout << "> ";
        std::cin >> command;

        if (command == "start") {
            emit runService();
 
        }
        else if (command == "restart") {
            emit restartService();

        }
        else if (command == "stop") {
            emit stopService();
            
        }
        else if (command == "help") {
            std::cout << std::endl;
            std::cout << "== COMMANDS PROGRAM ==" << std::endl;
            std::cout << "start - run services;" << std::endl;
            std::cout << "restart - restarts services;" << std::endl;
            std::cout << "stop - stops services;" << std::endl;
            std::cout << "exit - exit the program." << std::endl;
        }

        QThread::sleep(5);
    }

    emit exitService();
}

void ProcessingService::slotHttpServiceRunning(const int &port)
{
    slotShowUserMessage(QString("The Http service is running on the port %1").arg(port));
    qInfo() << "The Http service is running on the port " << port;

    if(_threadLoopProcessCommand){
        if(!_threadLoopProcessCommand->isRunning())
            _threadLoopProcessCommand->start();
    }
}

void ProcessingService::slotHttpServiceStopped()
{
    slotShowUserMessage(QString("Http service stopped ..."));
    qInfo() << "Http service stopped ...";
}

void ProcessingService::slotShowUserMessage(const QString &message)
{
    if (!message.isEmpty()) {
        std::cout << message.toStdString() << std::endl;
    }
}

void ProcessingService::slotError(const QString &message)
{
    slotShowUserMessage(message);
    slotExitService();
}

LoopProcessCommand::LoopProcessCommand(){ }

LoopProcessCommand::~LoopProcessCommand(){ }
