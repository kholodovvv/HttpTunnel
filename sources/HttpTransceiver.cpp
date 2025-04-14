#include "include/HttpTransceiver.hpp"

#define millisecond 1000

HttpTransceiver::HttpTransceiver(QObject* parent) : QObject(parent)
{
    _inHttpTrafficHandler = std::make_shared<IncomingHttpTrafficHandler>();
    _outHttpTrafficHandler = std::make_shared<OutgoingHttpTrafficHandler>();

    connect(_inHttpTrafficHandler.get(), &IncomingHttpTrafficHandler::sendingPackage, _outHttpTrafficHandler.get(), &OutgoingHttpTrafficHandler::slotNewConnection);
    connect(_outHttpTrafficHandler.get(), &OutgoingHttpTrafficHandler::readingData, _inHttpTrafficHandler.get(), &IncomingHttpTrafficHandler::slotReceptionData);
    connect(_inHttpTrafficHandler.get(), &IncomingHttpTrafficHandler::signalSocketDestroy, _outHttpTrafficHandler.get(), &OutgoingHttpTrafficHandler::slotDestroyConnection);
    
    connect(_outHttpTrafficHandler.get(), &OutgoingHttpTrafficHandler::signalError, this, &HttpTransceiver::slotError);
    connect(_inHttpTrafficHandler.get(), &IncomingHttpTrafficHandler::signalError, this, &HttpTransceiver::slotError);

    connect(_inHttpTrafficHandler.get(), &IncomingHttpTrafficHandler::signalStarted, this, &HttpTransceiver::slotRun);
    connect(_inHttpTrafficHandler.get(), &IncomingHttpTrafficHandler::signalStopped, this, &HttpTransceiver::slotStop);

    connect(_outHttpTrafficHandler.get(), &OutgoingHttpTrafficHandler::signalStarted, this, &HttpTransceiver::slotRun);
    connect(_outHttpTrafficHandler.get(), &OutgoingHttpTrafficHandler::signalStopped, this, &HttpTransceiver::slotStop);

    connect(_inHttpTrafficHandler.get(), &IncomingHttpTrafficHandler::signalServiceMessage, this, &HttpTransceiver::slotProcessServiceMessages);
    connect(_outHttpTrafficHandler.get(), &OutgoingHttpTrafficHandler::signalServiceMessage, this, &HttpTransceiver::slotProcessServiceMessages);

}

HttpTransceiver::~HttpTransceiver()
{
}

void HttpTransceiver::run()
{
    if (_testingProxy) {
        testingProxy();
        setProxy();
        emit signalTestingProxyFinished();

    }else{
        setProxy();
    }

    if (!_timerCheckNetwork) {
        _timerCheckNetwork = std::make_shared<QTimer>();
        connect(_timerCheckNetwork.get(), &QTimer::timeout, this, &HttpTransceiver::slotNetworkDataCollection);
    }

    if (!_listProxyServers.first.isEmpty() && _timeIntervalChecksConnection > 0)
        _timerCheckNetwork->start(_timeIntervalChecksConnection);

    _outHttpTrafficHandler->run();
    _inHttpTrafficHandler->run();
}

void HttpTransceiver::stop()
{
    _outHttpTrafficHandler->stop();
    _inHttpTrafficHandler->stop();

    if(_timerCheckNetwork && _timerCheckNetwork->isActive())
        _timerCheckNetwork->stop();
}

uint HttpTransceiver::getState()
{
    return _state;
}

void HttpTransceiver::setTestingProxy(const bool &value)
{
    _testingProxy = value;
}

void HttpTransceiver::setSettings(const QPair<QStringList, QStringList> &listProxy, const uint &portListen, const uint &maxTimeWaitConnectingProxyServer, const uint &timeIntervalChecksConnection)
{
    _listProxyServers = listProxy;
    _inHttpTrafficHandler->setPortListen(portListen);
    _outHttpTrafficHandler->setMaxTimeWaitConnectingProxyServer(maxTimeWaitConnectingProxyServer);
    _maxTimeWaitConnectingProxyServer = maxTimeWaitConnectingProxyServer * millisecond;
    _timeIntervalChecksConnection = timeIntervalChecksConnection * millisecond;
}

void HttpTransceiver::slotProcessServiceMessages(const QString &message)
{
    Q_UNUSED(message)
}

void HttpTransceiver::slotError(const QString &message)
{
    Q_UNUSED(message)
}

void HttpTransceiver::slotRun() 
{
    _signalCounter += 1;

    if(_signalCounter == 2){
        emit signalRunning(_inHttpTrafficHandler->getPortListen());
        _signalCounter = 0;
        _state = HttpTransceiverState::inProgress;
    }

}

void HttpTransceiver::slotStop()
{
    _signalCounter += 1;

    if(_signalCounter == 2){
        emit signalStopped();
        _signalCounter = 0;
        _state = HttpTransceiverState::notRunning;
    }
}

void HttpTransceiver::slotNetworkDataCollection()
{
    quint64 responseTime = _outHttpTrafficHandler->getResponseTime();
    int percentSuccessfulCon = 0;

    uint numberRequests = _outHttpTrafficHandler->getRequestsCounter();
    uint numberResponses = _outHttpTrafficHandler->getResponsesCounter();

    if (responseTime > 0 && numberRequests > 0 && numberResponses > 0) {

        percentSuccessfulCon = (numberResponses / numberRequests) * 100;

        if (percentSuccessfulCon < 60 || responseTime > _maxTimeWaitConnectingProxyServer) {
            changeProxy();
        }
    }
    else if(numberRequests > 0 && numberResponses == 0){
        changeProxy();
    }
}

void HttpTransceiver::testingProxy()
{
    QPair<bool, quint64> result;
    QVector<QPair<int, quint64>> tableTimeResponse;
    QPair<QStringList, QStringList> proxyList;

    if (!_listProxyServers.first.isEmpty() && !_listProxyServers.second.isEmpty()) {

        for (int i = 0; i < _listProxyServers.first.length(); ++i) {
            result = _outHttpTrafficHandler->isConnected(_listProxyServers.first.at(i), _listProxyServers.second.at(i));

            if (result.first) {
                proxyList.first.append(_listProxyServers.first.at(i));
                proxyList.second.append(_listProxyServers.second.at(i));
                tableTimeResponse.push_back(QPair(i, result.second));
            }
        }

        qSort(tableTimeResponse.begin(), tableTimeResponse.end(), [](const QPair<int, quint64>& p1, const QPair<int, quint64>& p2) {
            return p1.second < p2.second;
            });

        _listProxyServers.first.clear();
        _listProxyServers.second.clear();

        for (int i = 0; i < tableTimeResponse.length(); ++i) {
            for (int j = 0; j < proxyList.first.length(); ++j) {
                if (tableTimeResponse.at(i).first == j) {
                    _listProxyServers.first.append(proxyList.first.at(j));
                    _listProxyServers.second.append(proxyList.second.at(j));
                }
            }
        }
    }

}

void HttpTransceiver::setProxy()
{
    if (!_listProxyServers.first.isEmpty()) {
        if ((_idxCurrentProxy + 1) <= _listProxyServers.first.length()) {
            _idxCurrentProxy += 1;
            _outHttpTrafficHandler->setProxy(_listProxyServers.first.at(_idxCurrentProxy), _listProxyServers.second.at(_idxCurrentProxy).toInt(), "", "");
        }
    }
    else {
        _outHttpTrafficHandler->setProxy("", 0, "", "");
    }
}

void HttpTransceiver::changeProxy()
{
    if ((_idxCurrentProxy + 1) <= _listProxyServers.first.length()) {
        _outHttpTrafficHandler->setProxy(_listProxyServers.first.at(_idxCurrentProxy + 1), _listProxyServers.second.at(_idxCurrentProxy + 1).toInt(), "", "");
        _idxCurrentProxy += 1;

    }
    else {
        _idxCurrentProxy = 0;
        _outHttpTrafficHandler->setProxy(_listProxyServers.first.at(_idxCurrentProxy), _listProxyServers.second.at(_idxCurrentProxy).toInt(), "", "");
    }
        
}
