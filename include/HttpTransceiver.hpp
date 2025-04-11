/*
 * HttpTransceiver отвечает за взаимодействие модуля обрабатывающего
 * входящий (IncomingHttpTrafficHandler) и исходящий (OutgoingHttpTrafficManagement) трафик.
 *
 */

#pragma once

#include <QObject>
#include <QStringList>
#include <QThread>
#include <QPair>
#include <QStringList>
#include <QTimer>
#include <memory>

#include "include/IncomingHttpTrafficManagement.hpp"
#include "include/OutgoingHttpTrafficManagement.hpp"

namespace  HttpTransceiverState {
    enum servicesState{ inProgress = 0, notRunning = 1 };
}

class HttpTransceiver: public QObject
{
	Q_OBJECT

public:
	explicit HttpTransceiver(QObject *parent = nullptr);
	~HttpTransceiver();

    void run();
    void stop();
    uint getState();
    void setTestingProxy(const bool &value);
    void setSettings(const QPair<QStringList, QStringList> &listProxy, const uint &portListen, const uint &maxTimeWaitConnectingProxyServer, const uint &timeIntervalChecksConnection);

signals:
    void signalMessage(const QString &message);
    void signalStopped();
    void signalRunning(const int &port);
    void signalError(const QString &error);
    void signalTestingProxyFinished();

private slots:
    void slotProcessServiceMessages(const QString &message);
    void slotError(const QString &message);
    void slotRun();
    void slotStop();
    void slotNetworkDataCollection();

private:
    void testingProxy();
    void setProxy();
    void changeProxy();

private:
    std::shared_ptr<IncomingHttpTrafficHandler> _inHttpTrafficHandler;
    std::shared_ptr<OutgoingHttpTrafficHandler> _outHttpTrafficHandler;
    uint _state = HttpTransceiverState::notRunning;
    uint _signalCounter = 0;
    bool _testingProxy = true;
    QPair<QStringList, QStringList> _listProxyServers;
    int _idxCurrentProxy = -1;
    std::shared_ptr<QTimer> _timerCheckNetwork;
    uint _maxTimeWaitConnectingProxyServer = 5000;
    uint _timeIntervalChecksConnection = 0;
};
