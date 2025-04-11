#include "include/Logger.hpp"
#include <QFile>
#include <QFileInfo>
#include <QDateTime>

Logger::Logger()
{

}

Logger::~Logger()
{
}

bool Logger::writeToFile(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    Q_UNUSED(context)

	static const char* typeStr[] = { "Debug", "Warning", "Critical", "Fatal", "Info" };
	QString nameFile = QString("%1.log").arg(QDateTime::currentDateTime().toString("dd_MM_yyyy"));
	QFile logFile(QString("%1%2").arg(pathToDir, nameFile));

	if (!QDir(pathToDir).exists())
		QDir().mkdir(pathToDir);

	if (!QFileInfo(QString("%1%2").arg(pathToDir, nameFile)).exists()) {

		if (logFile.open(QIODevice::WriteOnly | QFile::Text)) {

			logFile.write(QString("[%1]%2: %3\n").arg(QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm:ss"), typeStr[type], message).toUtf8());
			logFile.close();

			return true;
		}
	}
	else {
		if (logFile.open(QFile::Append | QFile::Text)) {
			logFile.write(QString("[%1]%2: %3%4").arg(QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm:ss"), typeStr[type], message, "\n").toUtf8());
			logFile.close();

			return true;
		}
	}

	return false;
}
