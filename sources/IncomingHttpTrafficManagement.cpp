#include "include/IncomingHttpTrafficManagement.hpp"

#include <QByteArray>
#include <QStringList>
#include <QThread>
#include <QDebug>

IncomingHttpTrafficHandler::IncomingHttpTrafficHandler(QObject *parent): QObject(parent)
{
	
}

IncomingHttpTrafficHandler::~IncomingHttpTrafficHandler()
{
}

void IncomingHttpTrafficHandler::setPortListen(const uint &port)
{
    _port = port;
}

uint& IncomingHttpTrafficHandler::getPortListen()
{
    return _port;
}

void IncomingHttpTrafficHandler::run()
{
	_tcpServer = std::make_shared<QTcpServer>();

	if (_tcpServer) {
        if (_tcpServer->listen(QHostAddress::AnyIPv4, _port)) {
			connect(_tcpServer.get(), &QTcpServer::newConnection, this, &IncomingHttpTrafficHandler::slotNewConnection);

			qInfo() << "IncomingHttpTrafficHandler started";
            emit signalStarted();

		}
		else {
            qCritical() << _tcpServer->errorString();
            emit signalError(_tcpServer->errorString());
		}
	}
	
}

void IncomingHttpTrafficHandler::stop()
{
	qInfo() << "IncomingHttpTrafficHandler stopped";
	desroyAllConnections();
}

void IncomingHttpTrafficHandler::sendPackages(QTcpSocket* socket, const QByteArray &package)
{
    if (socket) {
        if (socket->isOpen() && socket->state() == QTcpSocket::ConnectedState) {
            if(socket->write(package, package.size()) == -1){
                qWarning() << "The data may not have reached the client!";
            }
		}
	}

}

void IncomingHttpTrafficHandler::destroyConnection(QTcpSocket* socket)
{
    int idxSocket = _clientsList.indexOf(socket);

	if (idxSocket > -1) {
		_clientsList.removeAt(idxSocket);
	}

    socket->deleteLater();
}

void IncomingHttpTrafficHandler::desroyAllConnections()
{
	if (_tcpServer) {
		_tcpServer->close();
	}
	
    for (QTcpSocket* client : _clientsList) {
        if (client->state() == QTcpSocket::ConnectedState) {
			client->abort();

            QThread::sleep(1);

			if (client->state() == QTcpSocket::ClosingState) {
				client->deleteLater();
			}
		}
	}

	if(!_clientsList.empty())
		_clientsList.clear();

    emit signalStopped();
}

bool IncomingHttpTrafficHandler::checkHeaderPackage(const QByteArray &package)
{
	QString header = package;
	QStringList headerList = header.split("\n");
	QString typeRequest;

	QString content = headerList.at(0);
	typeRequest = content.section(" ", 0, 0);
	
	if (typeRequest.length() <= 4)
		return true;

	return false;
}

void IncomingHttpTrafficHandler::slotReadData()
{
    QTcpSocket* socket = reinterpret_cast<QTcpSocket*>(sender());

	if (socket) {
        if (socket->isOpen()) {
            QByteArray data = socket->readAll();

            if (!data.isEmpty()){
                if (!checkHeaderPackage(data)) { //Если не GET или POST запрос, то пакет отклоняется
                    destroyConnection(socket);
                }
                else {
                    emit sendingPackage(socket, data);
                }
            }
		}
			
	}
}

void IncomingHttpTrafficHandler::slotDestroyConnection()
{
	QTcpSocket* socket = reinterpret_cast<QTcpSocket*>(sender());

	emit signalSocketDestroy(socket);

    int idxSocket = _clientsList.indexOf(socket);

	if (idxSocket > -1) {
		_clientsList.removeAt(idxSocket);
	}

	socket->deleteLater();
}

void IncomingHttpTrafficHandler::slotReceptionData(QTcpSocket* socket, const QByteArray &package)
{
    if (!package.isEmpty()) {			//Прием данных от менеджера исходящего трафика и отравка их обратно клиенту
        sendPackages(socket, package);
        socket->close();
	}
}

void IncomingHttpTrafficHandler::addToSocketList(QTcpSocket* socket)
{
	_clientsList.append(socket);

    connect(socket, &QTcpSocket::readyRead, this, &IncomingHttpTrafficHandler::slotReadData);
    connect(socket, &QTcpSocket::disconnected, this, &IncomingHttpTrafficHandler::slotDestroyConnection);
}

void IncomingHttpTrafficHandler::slotNewConnection()
{
	if (_tcpServer) {
		while (_tcpServer->hasPendingConnections()) {
            QTcpSocket *socket = _tcpServer->nextPendingConnection();

            addToSocketList(socket);
		}
	}
	
}
