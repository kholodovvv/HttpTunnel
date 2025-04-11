/*
 * IncomingHttpTrafficHandler отвечает за приём Tcp пакетов от клиента
 * и возвращает ответ на запрос от сервера
 *
 */

#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <memory>
#include <QList>

class IncomingHttpTrafficHandler : public QObject
{
	Q_OBJECT

public:
	explicit IncomingHttpTrafficHandler(QObject* parent = nullptr);
	~IncomingHttpTrafficHandler();

    void run();
    void setPortListen(const uint &port);
    uint& getPortListen();
    void stop();

private slots:
	void slotNewConnection();
	void slotReadData();
	void slotDestroyConnection();

public slots:
    void slotReceptionData(QTcpSocket* socket, const QByteArray &package);

signals:
    void signalStarted();
    void signalStopped();
    void signalError(const QString &message);
    void sendingPackage(QTcpSocket *socket, const QByteArray &package);
    void signalSocketDestroy(QTcpSocket *socket);
    void signalServiceMessage(const QString &message);

private:
    void addToSocketList(QTcpSocket* socket);
    void sendPackages(QTcpSocket* socket, const QByteArray &package);
    void destroyConnection(QTcpSocket* socket);
	void desroyAllConnections();
	bool checkHeaderPackage(const QByteArray& package);

private:
	std::shared_ptr<QTcpServer> _tcpServer;
    uint _port;
    QList<QTcpSocket*> _clientsList;
};
