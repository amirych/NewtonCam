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
#include <iostream>

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

                /*  create GUI  */

    ServerGUI serverGUI;
    if ( !cmdline_parser.isSet(noguiOption) ) {

    }
                /*   read config file    */

    quint16 server_port = NETPROTOCOL_DEFAULT_PORT;
    QList<QHostAddress> allowed_hosts = QList<QHostAddress>();
    int fontsize = SERVERGUI_DEFAULT_FONTSIZE;

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

            server_port = config.value("network/port",NETPROTOCOL_DEFAULT_PORT).toUInt(&ok);
            if ( !ok ) {

            }
        } else { // config file is given but not readable or cannot be found

        }
    }

    Server CamServer;


    return app.exec();
}
