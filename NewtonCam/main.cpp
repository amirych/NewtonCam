#include "server.h"
#include "servergui.h"
#include "proto_defs.h"
#include "../version.h"

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
#include <QRegExp>
#include <QObject>
#include <iostream>
#include <fstream>
#include <csignal>


#define CAMERA_DEFAULT_LOG_FILENAME "NewtonCam.log"
#define CAMERA_DEFAULT_INIT_PATH ""

                /****************************************
                *                                       *
                *   Main NewtonCam server application   *
                *                                       *
                ****************************************/

static QApplication *myApp;

// POSIX OS signal handler
#ifdef Q_OS_UNIX
void signal_handler_int(int signal) // Ctrl-C signal handler (cross-platform)
{
//    std::cerr << "I received signal " << signal << '\n';
    myApp->quit();
}

void signal_handler_term(int signal)
{
//    std::cerr << "I received signal " << signal << '\n';
    myApp->quit();
}
#endif

// Windows OS signal handler (from MDSN)
#ifdef Q_OS_WIN
#include <Windows.h>

BOOL CtrlHandler( DWORD fdwCtrlType )
{
  switch( fdwCtrlType )
  {
    // Handle the CTRL-C signal.
    case CTRL_C_EVENT:
      myApp->quit();
      return TRUE ;

    // CTRL-CLOSE: confirm that the user wants to exit.
    case CTRL_CLOSE_EVENT:
      myApp->quit();
      return TRUE;

    // Pass other signals to the next handler.
    case CTRL_BREAK_EVENT:
      myApp->quit();
      return TRUE;

    case CTRL_LOGOFF_EVENT:
      myApp->quit();
      return FALSE;

    case CTRL_SHUTDOWN_EVENT:
      myApp->quit();
      return FALSE;

    default:
      return FALSE;
  }
}
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    myApp = &app;

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
    unsigned long temp_poll_int = CAMERA_DEFAULT_TEMP_POLLING_INT;

    QString cameraInitPath = CAMERA_DEFAULT_INIT_PATH;

    bool ok;

    if ( !pos_arg.isEmpty() && QFile::exists(pos_arg[0]) ) {
        QFile file(pos_arg[0]);
        bool isreadable = (file.permissions() & QFile::ReadOwner) || (file.permissions() & QFile::ReadUser);
        if ( isreadable ) {
            QSettings config(pos_arg[0],QSettings::IniFormat);

#ifdef QT_DEBUG
            qDebug() << "Reading config from " << pos_arg[0];
#endif

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

            // DNS lookup (WARNING: it is blocks current thread!)
            QRegExp rx("\\s*\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\s*");
            QStringList list = hosts.split(",",QString::SkipEmptyParts);
            foreach (const QString &str, list) {
                ok = rx.exactMatch(str);
                if ( !ok ) { // skip IP-addresses and lookup only hostnames
                    QHostInfo info = QHostInfo::fromName(str.trimmed());
                    allowed_hosts.append(info.addresses());
                } else allowed_hosts << QHostAddress(str);
            }
#ifdef QT_DEBUG
            qDebug() << "Allowed hosts from config: " << allowed_hosts;
#endif
            cameraInitPath = config.value("camera/init_path",CAMERA_DEFAULT_INIT_PATH).toString();

            cameraLogFilename = config.value("camera/log_file",CAMERA_DEFAULT_LOG_FILENAME).toString();

            temp_poll_int = config.value("camera/temp_poll_interval",CAMERA_DEFAULT_TEMP_POLLING_INT).toUInt(&ok);
            if ( !ok ) {
                std::cerr << "Bad value of CCD chip temperature polling! Use of default value!\n";
                temp_poll_int = CAMERA_DEFAULT_TEMP_POLLING_INT;
            }

        } else { // config file is given but not readable or cannot be found
            std::cerr << "Config file could not be found or user has no permissions for reading! Use default values!\n";
        }
    }

#ifdef QT_DEBUG
    qDebug() << "Use log-file: " << cameraLogFilename;
#endif
    // open log-file

    std::ostream *LogFile;
    std::ofstream log_file;


    if ( cameraLogFilename.isEmpty() || cameraLogFilename.isNull() ||
         (!cameraLogFilename.compare("STD::CERR",Qt::CaseInsensitive) ) ) {
        LogFile = &std::cerr;
    } else if ( !cameraLogFilename.compare("STD::COUT",Qt::CaseInsensitive) ) {
        LogFile = &std::cout;
    } else if ( !cameraLogFilename.compare("NULL",Qt::CaseInsensitive) ) { // no log messages
        LogFile = nullptr;
    } else {
        log_file.open(cameraLogFilename.toUtf8().data(),std::ios_base::app);
        if ( !log_file.is_open() ) {
            qDebug() << "Can not open Log-file!!! Use of standard error output!";
            LogFile = &std::cerr;
        } else {
            LogFile = &log_file;
        }
    }

    // log header
    if ( LogFile != nullptr ) {
        char *sp = "            ";
        QString str1,str2;
        *LogFile << "\n\n\n";
        *LogFile << sp  << "***************************************************\n";
        *LogFile << sp  << "*                                                 *\n";
        *LogFile << sp  << "* NewtonCam: Andor Newton camera control software *\n";
        *LogFile << sp  << "*                                                 *\n";
        str1.setNum(NEWTONCAM_PACKAGE_VERSION_MAJOR);
        str2.setNum(NEWTONCAM_PACKAGE_VERSION_MINOR);
        str1 += "." + str2;
        str2 = "* Version:                                        *\n";
        str2.replace(11,str1.length(),str1);
        *LogFile << sp  << str2.toUtf8().data();

        char sdk_ver[100];
        GetVersionInfo(AT_SDKVersion,sdk_ver,100);
        str1 = sdk_ver;
        str2 = "* Andor SDK version:                              *\n";
        str2.replace(21,str1.length(),str1);
        *LogFile << sp  << str2.toUtf8().data();

        *LogFile << sp  << "*                                                 *\n";
        *LogFile << sp  << "***************************************************\n";

        *LogFile << "\n" << std::flush;
    }

    Server CamServer(*LogFile,allowed_hosts,server_port);
//    Server CamServer(std::cerr,allowed_hosts,server_port);

#ifdef QT_DEBUG
    qDebug() << "NETWORK SERVER START STATUS: " << CamServer.getLastSocketError();
#endif

        /*  create GUI  */

    ServerGUI *serverGUI;
    QWidget root_widget;

    if ( !cmdline_parser.isSet(noguiOption) ) {
        serverGUI = new ServerGUI(&root_widget);
        serverGUI->SetFonts(fontsize,statusFontsize,logFontsize);
        serverGUI->show();

        QString str = QDateTime::currentDateTime().toString(" dd-MM-yyyy hh:mm:ss:");
        serverGUI->LogMessage("<b>" + str + "</b>" + " Starting NewtonCam server ...");

        QObject::connect(&CamServer,SIGNAL(CameraStatus(QString)),serverGUI,SLOT(ServerStatus(QString)));
        QObject::connect(&CamServer,SIGNAL(TemperatureChanged(double)),serverGUI,SLOT(TempChanged(double)));
        QObject::connect(&CamServer,SIGNAL(CoolerStatusChanged(unsigned int)),serverGUI,SLOT(CoolerStatusChanged(uint)));
        QObject::connect(&CamServer,SIGNAL(HelloIsReceived(QString)),serverGUI,SLOT(LogMessage(QString)));
        QObject::connect(&CamServer,SIGNAL(InfoIsReceived(QString)),serverGUI,SLOT(LogMessage(QString)));
        QObject::connect(&CamServer,SIGNAL(CameraError(unsigned int)),serverGUI,SLOT(ServerError(uint)));
        QObject::connect(&CamServer,SIGNAL(ServerSocketError(QAbstractSocket::SocketError)),
                         serverGUI,SLOT(NetworkError(QAbstractSocket::SocketError)));
        QObject::connect(&CamServer,SIGNAL(ExposureClock(double)),serverGUI,SLOT(ExposureProgress(double)));
        app.processEvents();
    }

    // init path handling
    if ( cameraInitPath.isEmpty() || cameraInitPath.isNull() ) {
        cameraInitPath = QCoreApplication::applicationDirPath();
    }

    CamServer.SetPollingIntervals(temp_poll_int,CAMERA_DEFAULT_STATUS_POLLING_INT);
    CamServer.InitCamera(cameraInitPath,0);

//    if ( CamServer.getLastServerError() != Server::SERVER_ERROR_OK ) {
//        std::cerr << "Camera server failed to start!\n";
//        exit(CamServer.getLastServerError());
//    }

#ifdef Q_OS_UNIX
    std::signal(SIGINT, signal_handler_int);
    std::signal(SIGTERM, signal_handler_term);
#endif

#ifdef Q_OS_WIN
    if( !SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE ) ) {
#ifdef QT_DEBUG
        qDebug() << "Can not install OS signal handler!";
#endif
    }
#endif

    return app.exec();
}
