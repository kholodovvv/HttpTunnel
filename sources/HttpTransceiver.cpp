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

    if (!_vecProxySettings.isEmpty() && _timeIntervalChecksConnection > 0)
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

void HttpTransceiver::setSettings(const QVector<std::shared_ptr<ProxySettings>> &vecProxySettings, std::shared_ptr<Settings> &settings)
{
    _vecProxySettings = vecProxySettings;
    _inHttpTrafficHandler->setPortListen(settings->portListen);
    _outHttpTrafficHandler->setMaxTimeWaitReply(settings->maxTimeWaitReply * millisecond);
    _maxTimeWaitReply = settings->maxTimeWaitReply * millisecond;
    _timeIntervalChecksConnection = settings->proxyVerificationInterval * millisecond;
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

        if (percentSuccessfulCon < 60 || responseTime > _maxTimeWaitReply) {
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
    QVector<std::shared_ptr<ProxySettings>> vecProxySettings;

    if (!_vecProxySettings.isEmpty()) {

        for (int i = 0; i < _vecProxySettings.length(); ++i) {
            if(_vecProxySettings.at(i))
                result = _outHttpTrafficHandler->isConnected(_vecProxySettings.at(i)->host, _vecProxySettings.at(i)->port);

            if (result.first) {
                vecProxySettings.append(_vecProxySettings.at(i));
                tableTimeResponse.push_back(QPair(i, result.second));
            }
        }

        qSort(tableTimeResponse.begin(), tableTimeResponse.end(), [](const QPair<int, quint64>& p1, const QPair<int, quint64>& p2) {
            return p1.second < p2.second;
            });

        _vecProxySettings.clear();

        for (int i = 0; i < tableTimeResponse.length(); ++i) {
            for (int j = 0; j < _vecProxySettings.length(); ++j) {
                if (tableTimeResponse.at(i).first == j) {
                    if(vecProxySettings.at(j))
                        _vecProxySettings.append(vecProxySettings.at(j));
                }
            }
        }
    }

}

void HttpTransceiver::setProxy()
{
    if (!_vecProxySettings.isEmpty()) {
        if ((_idxCurrentProxy + 1) <= _vecProxySettings.length()) {
            _idxCurrentProxy += 1;

            if(_vecProxySettings.at(_idxCurrentProxy))
                _outHttpTrafficHandler->setProxy(_vecProxySettings.at(_idxCurrentProxy)->host, _vecProxySettings.at(_idxCurrentProxy)->port, _vecProxySettings.at(_idxCurrentProxy)->user, _vecProxySettings.at(_idxCurrentProxy)->password);
        }
    }
    else {
        _outHttpTrafficHandler->setProxy("", 0, "", "");
    }
}

void HttpTransceiver::changeProxy()
{
    if ((_idxCurrentProxy + 1) <= _vecProxySettings.length()) {
        
        if (_vecProxySettings.at(_idxCurrentProxy + 1))
            _outHttpTrafficHandler->setProxy(_vecProxySettings.at(_idxCurrentProxy + 1)->host, _vecProxySettings.at(_idxCurrentProxy + 1)->port, _vecProxySettings.at(_idxCurrentProxy + 1)->user, _vecProxySettings.at(_idxCurrentProxy + 1)->password);
        
        _idxCurrentProxy += 1;

    }
    else {
        _idxCurrentProxy = 0;

        if(_vecProxySettings.at(_idxCurrentProxy))
            _outHttpTrafficHandler->setProxy(_vecProxySettings.at(_idxCurrentProxy)->host, _vecProxySettings.at(_idxCurrentProxy)->port, _vecProxySettings.at(_idxCurrentProxy)->user, _vecProxySettings.at(_idxCurrentProxy)->password);
    }
        
}
