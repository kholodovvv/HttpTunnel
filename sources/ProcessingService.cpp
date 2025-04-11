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
    connect(_httpTranseiver.get(), &HttpTransceiver::signalMessage, this, &ProcessingService::slotShowUserMessage);
    connect(_httpTranseiver.get(), &HttpTransceiver::signalError, this, &ProcessingService::slotError);

    thread = std::make_shared<QThread>();
    mainLoop = std::make_shared<LoopProcessCommand>();

    if(thread){
        mainLoop->moveToThread(thread.get());
        connect(thread.get(), &QThread::started, mainLoop.get(), &LoopProcessCommand::slotLoopWaitCommand);
        connect(mainLoop.get(), &LoopProcessCommand::runService, this, &ProcessingService::runService);
        connect(mainLoop.get(), &LoopProcessCommand::stopService, this, &ProcessingService::stopService);
        connect(mainLoop.get(), &LoopProcessCommand::restartService, this, &ProcessingService::restartService);
        connect(mainLoop.get(), &LoopProcessCommand::exitService, this, &ProcessingService::exitService);
    }
}

void ProcessingService::runService()
{
    QPair<QStringList, QStringList> proxyServersList;

    if (!_settingsLoader->isLoadingSettings()) {
        slotShowUserMessage(QString("Couldn't load list proxy servers"));
        qWarning() << "Couldn't load list proxy servers";

    }
    else {
        slotShowUserMessage(QString("The list of proxy servers has been loaded successfully ..."));
        qInfo() << "The list of proxy servers has been loaded successfully";

    }

    if (_httpTranseiver) {
        proxyServersList = _settingsLoader->getProxyServersList();

        _httpTranseiver->setTestingProxy(false);
        _httpTranseiver->setSettings(proxyServersList, 7777, 50000, 300);

        slotShowUserMessage(QString("The http service is being started, please wait ..."));
        qInfo() << "Run http service";
        _httpTranseiver->run();
    }
}

void ProcessingService::stopService()
{
    if (_httpTranseiver) {
        if (_httpTranseiver->getState() != HttpTransceiverState::notRunning)
            _httpTranseiver->stop();
    }
}

void ProcessingService::restartService()
{
    if (_httpTranseiver) {
        if (_httpTranseiver->getState() != HttpTransceiverState::notRunning)
            _httpTranseiver->stop();

        while (_httpTranseiver->getState() != HttpTransceiverState::notRunning)
        {
            QThread::sleep(5);
        }
    }

    runService();
}

void ProcessingService::exitService()
{
    if (_httpTranseiver) {
        if (_httpTranseiver->getState() != HttpTransceiverState::notRunning) {

            _httpTranseiver->stop();

            while (_httpTranseiver->getState() != HttpTransceiverState::notRunning)
            {
                QThread::sleep(5);
            }

        }
        else {

            if (thread->isRunning()) {
                thread->quit();
            }
        }
    }

    exit(0);
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

    if(thread){
        if(!thread->isRunning())
            thread->start();
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
    exitService();
}


LoopProcessCommand::LoopProcessCommand(QObject* parent) : QObject(parent)
{

}

LoopProcessCommand::~LoopProcessCommand()
{
}
