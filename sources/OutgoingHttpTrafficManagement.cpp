#include "include/OutgoingHttpTrafficManagement.hpp"

#include <QTimer>
#include <QEventLoop>
#include <QNetworkRequest>
#include <QDebug>

OutgoingHttpTrafficHandler::OutgoingHttpTrafficHandler(QObject *parent):
    QObject(parent)
{

}

OutgoingHttpTrafficHandler::~OutgoingHttpTrafficHandler()
{

}

QPair<bool, quint64> OutgoingHttpTrafficHandler::isConnected(const QString &hostName, const QString &port)
{
    QElapsedTimer elapsedTimer;
    QTimer timer;
    QString elapsedTime;
    QPair<bool, quint64> answer;

    QNetworkProxy proxy;
    proxy.setType(QNetworkProxy::HttpProxy);
    proxy.setHostName(hostName);
    proxy.setPort(port.toInt());

    timer.setInterval(_maxTimeWaitConnectingProxyServer);
    timer.setSingleShot(true);

    std::shared_ptr<QNetworkAccessManager> accessManagerPtr;
    QNetworkRequest request;

    request.setUrl(QUrl("http://ruliauto.ru"));

    if(!accessManagerPtr)
        accessManagerPtr = std::make_shared<QNetworkAccessManager>(new QNetworkAccessManager(this));

    accessManagerPtr->setProxy(proxy);

    QNetworkReply *reply = accessManagerPtr->get(request);

    QEventLoop loop;

    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timer, &QTimer::timeout, reply, &QNetworkReply::abort);

    timer.start();
    elapsedTimer.start();
    loop.exec();

    if (reply->isFinished() && reply->error() == QNetworkReply::NoError) {
        if(reply->bytesAvailable() > 0){
            answer.first = true;
            answer.second = elapsedTimer.elapsed();
        }
    }
    else {
        answer.first = false;
        answer.second = elapsedTimer.elapsed();
    }

    if(reply)
        reply->deleteLater();

    return answer;
}

void OutgoingHttpTrafficHandler::setMaxTimeWaitConnectingProxyServer(const uint &time)
{
    _maxTimeWaitConnectingProxyServer = time;
}

uint& OutgoingHttpTrafficHandler::getResponsesCounter()
{
    return _counterResponses;
}

uint& OutgoingHttpTrafficHandler::getRequestsCounter()
{
    return _counterRequests;
}

quint64& OutgoingHttpTrafficHandler::getResponseTime()
{
    return _responseTime;
}

void OutgoingHttpTrafficHandler::run()
{
    qInfo() << "OutgoingHttpTrafficHandler started";
    emit signalStarted();
}

void OutgoingHttpTrafficHandler::stop()
{
    destroyAllConnections();
    qInfo() << "OutgoingHttpTrafficHandler stopped";

    emit signalStopped();
}

void OutgoingHttpTrafficHandler::setProxy(const QString &address, const uint &port, const QString &userName, const QString &password)
{
    if (!_proxy) {
        _proxy = std::make_shared<QNetworkProxy>();
        _proxy->setType(QNetworkProxy::HttpProxy);
    }

    if(address.isEmpty() && port == 0){
        _proxy->setType(QNetworkProxy::NoProxy);

    }else{
        _proxy->setHostName(address);
        _proxy->setPort(port);

        if(!userName.isEmpty())
            _proxy->setUser(userName);

        if(!password.isEmpty())
            _proxy->setPassword(password);
    }

    _counterResponses = 0;
    _counterRequests = 0;
}

void OutgoingHttpTrafficHandler::slotNewConnection(QTcpSocket* socket, const QByteArray &package)
{
    std::shared_ptr<QNetworkAccessManager> accessManagerPtr = std::make_shared<QNetworkAccessManager>(new QNetworkAccessManager(this));

    if (socket && !package.isEmpty()) {
        for (auto item : _listConnections) {
            if (item.first == socket) {
                accessManagerPtr = item.second;
            }
        }

        accessManagerPtr->setProxy(*_proxy);
        _listConnections.append(QPair(socket, accessManagerPtr));

        _counterRequests += 1;
        
        if (!_isStartedTimer) {
            if (!_timerWaitingResponse) {
                _timerWaitingResponse = std::make_shared<QElapsedTimer>();
            }

            _timerWaitingResponse->start();

            _isStartedTimer = true;
        }

        sendData(socket, package);
    }

}

void OutgoingHttpTrafficHandler::slotReadData(QNetworkReply* reply)
{
    QByteArray data;
    QTcpSocket *socket = nullptr;

    if(reply){
        for (auto item : _listReply) {
            if (item.second == reply)
                socket = item.first;
        }

        while(reply->bytesAvailable() > 0){
            data.append(reply->readAll());
        }

        if (!data.isEmpty()) {
            _counterResponses += 1;

            readingData(socket, data);

            if (_timerWaitingResponse) {
                if (_vecResponseTime.length() < 10) {
                    _vecResponseTime.append(_timerWaitingResponse->elapsed());
                    _isStartedTimer = false;
                }
                else {
                    calculatingResponseTime();
                }
            }

        }else if(reply->error() == QNetworkReply::NetworkError()){
   
            qWarning() << reply->errorString();
        }

        destroyReply(reply);
    }
}

void OutgoingHttpTrafficHandler::destroyReply(QNetworkReply* replyPtr)
{
    int idxReply = -1;
    QTcpSocket *socket = nullptr;

    for (auto item : _listReply) {
        if (item.second == replyPtr) {
            socket = item.first;

            if (replyPtr) {
                replyPtr->abort();
                replyPtr->deleteLater();
            }

            idxReply = _listReply.indexOf(item);
        }
    }

    if (idxReply > -1)
        _listReply.removeAt(idxReply);
}

void OutgoingHttpTrafficHandler::slotDestroyConnection(QTcpSocket *socket)
{
    int idxConnections = -1;

    for (auto item : _listConnections) {
        if (item.first == socket) {

            idxConnections = _listConnections.indexOf(item);
        }
    }

    if (idxConnections > -1)
        _listConnections.removeAt(idxConnections);
}


void OutgoingHttpTrafficHandler::sendData(QTcpSocket* socket, const QByteArray &package)
{
    std::shared_ptr<QNetworkAccessManager> accessManagerPtr;

    QPair<QPair<QString, QNetworkRequest>, QByteArray> paramHeaderRequest = parseHeaderPackage(package);

    if (socket) {
        for (auto item : _listConnections) {
            if (item.first == socket) {
                accessManagerPtr = item.second;
            }
        }

        if (accessManagerPtr) {

            if(paramHeaderRequest.first.first == "GET"){
                QNetworkReply *replyPtr(accessManagerPtr->get(paramHeaderRequest.first.second));
                _listReply.append(QPair(socket, replyPtr));

            }else{

                QNetworkReply *replyPtr(accessManagerPtr->post(paramHeaderRequest.first.second, paramHeaderRequest.second));
                _listReply.append(QPair(socket, replyPtr));
            }

            connect(accessManagerPtr.get(), &QNetworkAccessManager::finished, this, &OutgoingHttpTrafficHandler::slotReadData);

        }
    }

}

void OutgoingHttpTrafficHandler::destroyAllConnections()
{
    if (!_listReply.empty()) {
        for (auto item : _listReply) {
            if(item.second)
                item.second->deleteLater();
        }

        _listReply.clear();
    }

    if (!_listConnections.empty()) 
        _listConnections.clear();
    
}

QPair<QPair<QString, QNetworkRequest>, QByteArray> OutgoingHttpTrafficHandler::parseHeaderPackage(const QByteArray &package)
{
    QString header = package;
    QStringList headerList = header.split("\r\n");
    QPair<QPair<QString, QNetworkRequest>, QByteArray> paramRequest;

    QByteArray dataArr;
    QNetworkRequest request;

    QString content = headerList.at(0);
    if(content.section(" ", 1, 1) != "" && content.section(" ", 0).length() > 0){
        paramRequest.first.first = content.section(" ", 0, 0);
        request.setUrl(QUrl(content.section(" ", 1, 1)));
    }

    if(paramRequest.first.first == "POST"){

        for (int i = 0; i < headerList.length(); ++i) {
            if(headerList.at(i).section(" ", 0, 0) == "Host:"){
                request.setRawHeader("Host", headerList.at(i).section(" ", 1, 1).toUtf8());

            }else if(headerList.at(i).section(" ", 0, 0) == "User-Agent:"){
                request.setHeader(QNetworkRequest::UserAgentHeader, headerList.at(i).section(" ", 1, 1).toUtf8());

            }else if(headerList.at(i).section(" ", 0, 0) == "Accept:"){
                request.setRawHeader("Accept", headerList.at(i).section(" ", 1, 1).toUtf8());

            }else if(headerList.at(i).section(" ", 0, 0) == "Accept-Language:"){
                request.setRawHeader("Accept-Language", headerList.at(i).section(" ", 1, 1).toUtf8());

            }else if (headerList.at(i).section(" ", 0, 0) == "Accept-Encoding:") {
                request.setRawHeader("Accept-Encoding", headerList.at(i).section(" ", 1, 1).toUtf8());

            }else if (headerList.at(i).section(" ", 0, 0) == "Content-Type:") {
                request.setHeader(QNetworkRequest::ContentTypeHeader, headerList.at(i).section(" ", 1, 1).toUtf8());

            }else if (headerList.at(i).section(" ", 0, 0) == "Content-Length:") {
                request.setHeader(QNetworkRequest::ContentLengthHeader, headerList.at(i).section(" ", 1, 1).toUtf8());

            }else if (headerList.at(i).section(" ", 0, 0) == "Connection:") {
                request.setRawHeader("Connection", headerList.at(i).section(" ", 1, 1).toUtf8());

            }
        }

        dataArr.append(headerList.back());
        paramRequest.second = dataArr;

    }else{
        for (int i = 1; i < headerList.length(); ++i) {

            if(headerList.at(i).section(" ", 0, 0) == "Host:"){
                request.setRawHeader("Host", headerList.at(i).section(" ", 1, 1).toUtf8());

            }else if(headerList.at(i).section(" ", 0, 0) == "User-Agent:"){
                request.setHeader(QNetworkRequest::UserAgentHeader, headerList.at(i).section(" ", 1, 1).toUtf8());

            }
        }

    }

    paramRequest.first.second = request;

    return paramRequest;
}

void OutgoingHttpTrafficHandler::calculatingResponseTime()
{
    quint64 summTime = 0;

    for (auto timeIter : _vecResponseTime) {
        summTime += timeIter;
    }

    _responseTime = summTime / _vecResponseTime.length();
    _vecResponseTime.clear();
}
