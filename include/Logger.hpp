#pragma once

#include <qloggingcategory.h>
#include <QDir>

class Logger
{

public:
	explicit Logger();
	Logger(Logger const&) = delete;
	Logger& operator=(const Logger&) = delete;
	~Logger();

	bool writeToFile(QtMsgType type, const QMessageLogContext &context, const QString &message);

private:
	const QString pathToDir = "./Logs/";
};