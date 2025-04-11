#include <QCoreApplication>
#include "include/ProcessingService.hpp"
#include "include/Logger.hpp"
#include <qlogging.h>
#include <iostream>
#include <memory>

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    std::shared_ptr<Logger> logger = std::make_shared<Logger>();

    if (logger) {
        if (!logger->writeToFile(type, context, msg))
            std::cout << "Logs cannot be recorded!" << std::endl;
    }
}

int main(int argc, char *argv[])
{
    //Install message handler
    qInstallMessageHandler(messageHandler);

    //Restore handler
    //qInstallMessageHandler(0);

    QCoreApplication a(argc, argv);

    qInfo() << "Launching the program!";

    ProcessingService procService;
    procService.runService();
    
    return a.exec();
}
