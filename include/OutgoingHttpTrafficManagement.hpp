#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QList>
#include <QPair>
#include <QNetworkProxy>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStringList>
#include <QElapsedTimer>
#include <QVector>
#include <memory>

class OutgoingHttpTrafficHandler : public QObject
{
    Q_OBJECT

public:
    explicit OutgoingHttpTrafficHandler(QObject* parent = nullptr);
    ~OutgoingHttpTrafficHandler();

    QPair<bool, quint64> isConnected(const QString &hostName, const /*QString*/uint &port);
    void setProxy(const QString &address, const uint &port, const QString &userName, const QString &password);
    void setMaxTimeWaitReply(const uint &time);
    uint& getResponsesCounter();
    uint& getRequestsCounter();
    quint64& getResponseTime();
    void run();
    void stop();

public slots:
    void slotNewConnection(QTcpSocket *socket, const QByteArray &package); //Слот для обработки входящих пакетов
    void slotReadData(QNetworkReply *reply);
    void slotDestroyConnection(QTcpSocket *socket);

signals:
    void readingData(QTcpSocket *socket, const QByteArray &package);
    void signalError(const QString &error);
    void signalServiceMessage(const QString &message);
    void signalStarted();
    void signalStopped();

private:
    void sendData(QTcpSocket *socket, const QByteArray &package);
    void destroyReply(QNetworkReply *replyPtr);
    void destroyAllConnections();
    QPair<QPair<QString, QNetworkRequest>, QByteArray> parseHeaderPackage(const QByteArray &package);
    void calculatingResponseTime();

private:
    std::shared_ptr<QNetworkProxy> _proxy;
    uint _maxTimeWaitReply;
    QList<QPair<QTcpSocket*, std::shared_ptr<QNetworkAccessManager>>> _listConnections;
    QList<QPair<QTcpSocket*, QNetworkReply*>> _listReply;
    uint _counterResponses;
    uint _counterRequests;
    std::shared_ptr<QElapsedTimer> _timerWaitingResponse;
    quint64 _responseTime = 0;
    QVector<quint64> _vecResponseTime;
    bool _isStartedTimer = false;
};
