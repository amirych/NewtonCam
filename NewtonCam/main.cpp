#include "server.h"
#include "servergui.h"
#include "proto_defs.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QSettings>
#include <QStringList>
#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QList>
#include <QDialog>
#include <QDateTime>
#include <iostream>


#define CAMERA_DEFAULT_LOG_FILENAME "NewtonCam.log"

                /****************************************
                *                                       *
                *   Main NewtonCam server application   *
                *                                       *
                ****************************************/


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

            /*  commandline options definition  */

    QCommandLineParser cmdline_parser;

    cmdline_parser.addHelpOption();

    QCommandLineOption noguiOption("nogui","Disable server GUI","");

    cmdline_parser.addOption(noguiOption);

    cmdline_parser.addPositionalArgument("Config-filename", "NewtonCam server configuration filename");

    cmdline_parser.process(app);

    QStringList pos_arg = cmdline_parser.positionalArguments();

                /*   read config file    */

    // default configuration parameters
    quint16 server_port = NETPROTOCOL_DEFAULT_PORT;
    QList<QHostAddress> allowed_hosts = QList<QHostAddress>();

    int fontsize = SERVERGUI_DEFAULT_FONTSIZE;
    int statusFontsize = SERVERGUI_DEFAULT_FONTSIZE-2;
    int logFontsize = SERVERGUI_DEFAULT_FONTSIZE;

    QString cameraLogFilename = CAMERA_DEFAULT_LOG_FILENAME;

    bool ok;

    if ( !pos_arg.isEmpty() && QFile::exists(pos_arg[0]) ) {
        QFile file(pos_arg[0]);
        bool isreadable = (file.permissions() & QFile::ReadOwner) || (file.permissions() & QFile::ReadUser);
        if ( isreadable ) {
            QSettings config(pos_arg[0],QSettings::IniFormat);

            fontsize = config.value("servergui/fontsize",SERVERGUI_DEFAULT_FONTSIZE).toInt(&ok);
            if ( !ok ) {
                std::cerr << "Bad value of font size! Use of default value!\n";
                fontsize = SERVERGUI_DEFAULT_FONTSIZE;
            }

            statusFontsize = config.value("servergui/statusfontsize",fontsize-2).toInt(&ok);
            if ( !ok ) {
                std::cerr << "Bad value of status bar font size! Use of default value!\n";
                statusFontsize = fontsize-2;
            }

            logFontsize = config.value("servergui/logfontsize",fontsize).toInt(&ok);
            if ( !ok ) {
                std::cerr << "Bad value of server log window font size! Use of default value!\n";
                logFontsize = fontsize;
            }

            server_port = config.value("network/port",NETPROTOCOL_DEFAULT_PORT).toUInt(&ok);
            if ( !ok ) {
                std::cerr << "Bad value of server port! Use of default value!\n";
                server_port = NETPROTOCOL_DEFAULT_PORT;
            }

            QString hosts = config.value("network/allowed_hosts",SERVER_DEFAULT_ALLOWED_HOST).toString();

            QStringList list = hosts.split(",",QString::SkipEmptyParts);
            foreach (QString str, list) {
                allowed_hosts << QHostAddress(str);
            }

            cameraLogFilename = config.value("camera/log_file",CAMERA_DEFAULT_LOG_FILENAME).toString();

        } else { // config file is given but not readable or cannot be found
            std::cerr << "Config file could not be found or user has no permissions for reading! Use default values!\n";
        }
    }

    Server CamServer(allowed_hosts,server_port,&app);

        /*  create GUI  */

    ServerGUI *serverGUI;
    QWidget root_widget;

    if ( !cmdline_parser.isSet(noguiOption) ) {
        serverGUI = new ServerGUI(&root_widget);
        serverGUI->SetFonts(fontsize,statusFontsize,logFontsize);
        serverGUI->show();

        QString str = QDateTime::currentDateTime().toString(" dd-MM-yyyy hh:mm:ss");
        serverGUI->LogMessage("<b>" + str + "</b>" + " Starting NewtonCam server ...");

        QObject::connect(&CamServer,SIGNAL(HelloIsReceived(QString)),serverGUI,SLOT(LogMessage(QString)));
    }


//    if ( CamServer.getLastServerError() != Server::SERVER_ERROR_OK ) {
//        std::cerr << "Camera server failed to start!\n";
//        exit(CamServer.getLastServerError());
//    }


    return app.exec();
}
