#include "netpacket.h"
#include "proto_defs.h"
#include "../server/server.h"
#include "../version.h"

#ifdef Q_OS_WIN
    #include "../AndorSDK/atmcd32d.h"
#endif

#ifdef Q_OS_LINUX
    #include "../AndorSDK/atmcdLXd.h"
#endif


#include <QCoreApplication>
#include <QTcpSocket>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QRegExp>
#include <iostream>

#include <chrono>
#include <thread>

#define CLIENT_DEFAULT_SERVER_IP "127.0.0.1"

// client errors

enum ClientError {CLIENT_ERROR_OK, CLIENT_ERROR_INVALID_OPTION = 10, CLIENT_ERROR_INVALID_FITS_FILENAME,
                  CLIENT_ERROR_INVALID_HDR_FILENAME, CLIENT_ERROR_CONNECTION,
                  CLIENT_ERROR_UNKNOWN_SERVER, CLIENT_ERROR_INVALID_SERVER_STATUS, CLIENT_ERROR_INVALID_TEMP_RESPONSE};

// send command macro

#define SEND_COMMAND {\
    ok = command_packet.Send(&socket,NETPROTOCOL_TIMEOUT); \
    if ( !ok ) { \
        socket.disconnectFromHost(); \
        return CLIENT_ERROR_CONNECTION; \
    } \
}

// get server status macro

#define SERVER_STATUS {\
    server_status_packet.Receive(&socket,NETPROTOCOL_TIMEOUT); \
    if ( !server_status_packet.isPacketValid() ) { \
        qDebug() << "PACKET ERROR: " << server_status_packet.GetPacketError();\
        qDebug() << "BAD SERVER STATUS PACKET!"; \
        exit(CLIENT_ERROR_CONNECTION); \
    } \
    status = server_status_packet.GetStatus(); \
    qDebug() << "<> SERVER RESPONDS: " << status; \
    if ( status != Server::SERVER_ERROR_OK ) {\
        exit(status); \
    } \
}


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QCommandLineParser cmdline_parser;

    cmdline_parser.addHelpOption();

    // create commandline options

    QCommandLineOption initOption("init", "Initialize camera");

    QCommandLineOption stopOption("stop", "Stop current acquisition process");

    QCommandLineOption expOption(QStringList() << "e" << "exp", "Set exposure time",
                                 "time in secs");

    QCommandLineOption binOption(QStringList() << "b" << "bin", "Set binning",
                                 "binning string");

    QCommandLineOption gainOption(QStringList() << "g" << "gain", "Set gain",
                                  "gain string");

    QCommandLineOption roiOption(QStringList() << "f" << "frame", "Set readout region",
                                      "geometry array");

    QCommandLineOption rateOption(QStringList() << "r" << "rate", "Set readout rate", "rate string");

    QCommandLineOption shutterOption(QStringList() << "s" << "shutter", "Shutter control", "boolean value", "1");

    QCommandLineOption coolerOption(QStringList() << "c" << "cooler", "Cooler control", "ON/OFF string");

    QCommandLineOption fanOption("fan", "Set fan state", "Fan state string (FULL, LOW, OFF)");

    QCommandLineOption tempOption("settemp", "Set CCD chip temperature", "degrees");

    QCommandLineOption gettempOption("gettemp", "Get CCD chip temperature");

    QCommandLineOption server_ipOption("ip", "Server IP address", "address", CLIENT_DEFAULT_SERVER_IP);

    QString server_port_str;
    server_port_str.setNum(NETPROTOCOL_DEFAULT_PORT);
    QCommandLineOption server_portOption("port", "Server port", "port", server_port_str);

    cmdline_parser.addPositionalArgument("FITS-file","Output FITS filename");
    cmdline_parser.addPositionalArgument("HEADER-file","Additional FITS-header filename (optional)","[HEADER-file]");

    cmdline_parser.addOption(initOption);
    cmdline_parser.addOption(stopOption);

    cmdline_parser.addOption(expOption);
    cmdline_parser.addOption(binOption);
    cmdline_parser.addOption(gainOption);
    cmdline_parser.addOption(roiOption);
    cmdline_parser.addOption(rateOption);
    cmdline_parser.addOption(shutterOption);
    cmdline_parser.addOption(coolerOption);
    cmdline_parser.addOption(fanOption);
    cmdline_parser.addOption(tempOption);
    cmdline_parser.addOption(gettempOption);

    cmdline_parser.addOption(server_ipOption);
    cmdline_parser.addOption(server_portOption);

    cmdline_parser.process(a);


    // is client was run without arguments and options
    if ( a.arguments().length() == 1 ) {
        qDebug() << "There were no arguments and options!";
        return 0;
    }

    /*  Parsing some command-line options  */

    bool ok;

    // binning
    QString bin_str = cmdline_parser.value(binOption);
    QVector<double> bin_vals;
    if ( !bin_str.isEmpty() ) {
        QRegExp rx("\\s*\\d+x\\d+\\s*"); // binning string must be in format XBINxYBIN
        ok = rx.exactMatch(bin_str);
        if ( !ok ) {
            qDebug() << "BAD BIN!";
            return CLIENT_ERROR_INVALID_OPTION;
        }
        QStringList v = bin_str.split("x",QString::SkipEmptyParts);
        bin_vals << v[0].toDouble() << v[1].toDouble();
    }

    // read-out region
    QString roi_str = cmdline_parser.value(roiOption);
    QVector<double> roi_vals;
    if ( !roi_str.isEmpty() ) {
        QRegExp rx("\\s*\\d+\\s+\\d+\\s+\\d+\\s+\\d+\\s*");
        ok = rx.exactMatch(roi_str);
        if ( !ok ) {
            qDebug() << "BAD ROI!";
            return CLIENT_ERROR_INVALID_OPTION;
        }
        QStringList v = roi_str.split(" ",QString::SkipEmptyParts);
        for ( int i = 0; i < 4; ++i ) roi_vals << v[i].toDouble();
    }


    // exposure time
    QString exp_str = cmdline_parser.value(expOption);
    double exp_time;
    if ( !exp_str.isEmpty() ) {
        exp_time = exp_str.toDouble(&ok);
        if (!ok || (exp_time < 0.0) ){
            qDebug() << "BAD EXP TIME!";
            return CLIENT_ERROR_INVALID_OPTION;
        }
    }


    // shutter state
    QString shutter_str = cmdline_parser.value(shutterOption);
    int shutter_state = shutter_str.toInt(&ok); // shutterOtion has default value, so do not check for empty string
    if ( !ok ) {
        qDebug() << "BAD SHUTTER STATE!";
        return CLIENT_ERROR_INVALID_OPTION;
    }

    // cooler operation
    QString cooler_str = cmdline_parser.value(coolerOption).toUpper().trimmed();
    if ( !cooler_str.isEmpty() ) {
        if ( (cooler_str != "ON") && (cooler_str != "OFF") ) {
#ifdef QT_DEBUG
            qDebug() << "BAD COOLER STATE VALUE! (" << cooler_str << ")";
#endif
            return CLIENT_ERROR_INVALID_OPTION;
        }
    }


    // fan operation
    QString fan_str = cmdline_parser.value(fanOption).toUpper().trimmed();
    if ( !fan_str.isEmpty() ) {
        if ( (fan_str != "FULL") && (fan_str != "LOW") && (fan_str != "OFF") ) {
#ifdef QT_DEBUG
            qDebug() << "BAD FAN STATE VALUE!";
#endif
            return CLIENT_ERROR_INVALID_OPTION;
        }
    }

    // CCD temperature
    QString temp_str = cmdline_parser.value(tempOption);
    double ccd_temp;
    if ( !temp_str.isEmpty() ) {
        ccd_temp = temp_str.toDouble(&ok);
        if ( !ok ) {
            qDebug() << "BAD TEMPERATURE!";
            return CLIENT_ERROR_INVALID_OPTION;
        }
    }

    // server port
    QString port_str = cmdline_parser.value(server_portOption);
    QRegExp rx("\\s*\\d{1,}\\s*");
    if ( !rx.exactMatch(port_str) ) {
        qDebug() << "BAD PORT!";
        return CLIENT_ERROR_INVALID_OPTION;
    }
    quint16 server_port = port_str.toULong();

    // check positional arguments (FITS and optional header files) if present

    QStringList pos_args = cmdline_parser.positionalArguments();

    bool send_shutter = false;

    if ( !pos_args.isEmpty() ) {
        QString str = pos_args[0].trimmed();
        if ( str.isEmpty() ) {
            return CLIENT_ERROR_INVALID_FITS_FILENAME;
        }
        send_shutter = true;

        if ( pos_args.length() > 1 ) {
            str = pos_args[1].trimmed();
            if ( str.isEmpty() ) {
                return CLIENT_ERROR_INVALID_HDR_FILENAME;
            }
        }
    }

                /*  Try to connect to server  */

    QString server_ip = cmdline_parser.value(server_ipOption);
    QTcpSocket socket;

    socket.connectToHost(server_ip,server_port);
    if ( !socket.waitForConnected(NETPROTOCOL_TIMEOUT) ) {
        std::cerr << "Can not connect to server at " << server_ip.toUtf8().data()
                  << ":" << server_port << std::endl;
        return CLIENT_ERROR_CONNECTION;
    }

    QObject::connect(&socket,&QTcpSocket::disconnected,[=](){exit(CLIENT_ERROR_CONNECTION);});

                /*   Hand-shaking    */

    HelloNetPacket hello;
    StatusNetPacket server_status_packet;
    status_t status;
    QString clientVersion, str;

                /*  Send HELLO message  */

    clientVersion.setNum(NEWTONCAM_PACKAGE_VERSION_MAJOR);
    str.setNum(NEWTONCAM_PACKAGE_VERSION_MINOR);
    clientVersion += "." + str;

    hello.SetSenderType(NETPROTOCOL_SENDER_TYPE_CLIENT,clientVersion);
    ok = hello.Send(&socket,NETPROTOCOL_TIMEOUT);
    if ( !ok ) {
        return CLIENT_ERROR_CONNECTION;
    }

                /*   Receive HELLO message from server   */

    hello.Receive(&socket,NETPROTOCOL_TIMEOUT);
    if ( !hello.isPacketValid() ) {
        socket.disconnectFromHost();
        return CLIENT_ERROR_CONNECTION;
    }

    if ( hello.GetSenderType() != NETPROTOCOL_SENDER_TYPE_SERVER ) {
        socket.disconnectFromHost();
        return CLIENT_ERROR_UNKNOWN_SERVER;
    }
#ifdef QT_DEBUG
    qDebug() << "Received HELLO from server: " << hello.GetSenderType();
#endif

    // receive server answer ... It may be BUSY or OK
    SERVER_STATUS;

                /*  Send commands to server  */

    CmdNetPacket command_packet;
    QString info_str;
    QTextStream info_stream(&info_str);

    // --stop

    if ( cmdline_parser.isSet(stopOption) ) {
        info_stream << NETPROTOCOL_COMMAND_STOP << " ";
        command_packet.SetCommand(NETPROTOCOL_COMMAND_STOP,"");
        SEND_COMMAND;
        SERVER_STATUS;
    }


    // --init
    if ( cmdline_parser.isSet(initOption) ) {
        info_stream << NETPROTOCOL_COMMAND_INIT << " ";
        command_packet.SetCommand(NETPROTOCOL_COMMAND_INIT,"");
        SEND_COMMAND;
        SERVER_STATUS;
    }

    // --gettemp, it is a special command! The program receives a CCD temperature and cooler status, then exit!

    if ( cmdline_parser.isSet(gettempOption)) {
        info_stream << NETPROTOCOL_COMMAND_GETTEMP << " ";
        command_packet.SetCommand(NETPROTOCOL_COMMAND_GETTEMP,"");
        SEND_COMMAND;
        TempNetPacket pk;
        pk.Receive(&socket,NETPROTOCOL_TIMEOUT);
        if ( !pk.isPacketValid() ) {
            socket.disconnectFromHost();
            return CLIENT_ERROR_INVALID_TEMP_RESPONSE;
        }
        SERVER_STATUS;

        unsigned int cooling_status;
        ccd_temp = pk.GetTemp(&cooling_status);

        QString cooling_status_str = "[]";

        switch (cooling_status) {
        case DRV_ACQUIRING: {
            cooling_status_str.insert(1,"ACQUIRING");
            break;
        }
        case DRV_TEMP_OFF: {
            cooling_status_str.insert(1,"OFF");
            break;
        }
        case DRV_TEMP_STABILIZED: {
            cooling_status_str.insert(1,"STABILIZED");
            break;
        }
        case DRV_TEMP_NOT_REACHED: {
            cooling_status_str.insert(1,"NOT REACHED");
            break;
        }
        case DRV_TEMP_DRIFT: {
            cooling_status_str.insert(1,"DRIFT");
            break;
        }
        case DRV_TEMP_NOT_STABILIZED: {
            cooling_status_str.insert(1,"NOT STABILIZED");
            break;
        }
        default:
            break;
        }

        std::cout << ccd_temp << " " << cooling_status_str.toUtf8().data() << std::endl;

//        socket.disconnect();
//        socket.disconnectFromHost();
//        return status;
    }



    // --bin (-b)

    if ( cmdline_parser.value(binOption) != "" ) {
        command_packet.SetCommand(NETPROTOCOL_COMMAND_BINNING,bin_vals);
        info_stream << command_packet.GetPacketContent() << " ";
        SEND_COMMAND;
        SERVER_STATUS;
    }

    // --rate (-r)

    if ( cmdline_parser.value(rateOption) != "" ) {
        command_packet.SetCommand(NETPROTOCOL_COMMAND_RATE,cmdline_parser.value(rateOption));
        info_stream << command_packet.GetPacketContent() << " ";
        SEND_COMMAND;
        SERVER_STATUS;
    }


    // --gain (-g)

    if ( cmdline_parser.value(gainOption) != "" ) {
        command_packet.SetCommand(NETPROTOCOL_COMMAND_GAIN,cmdline_parser.value(gainOption));
        info_stream << command_packet.GetPacketContent() << " ";
        SEND_COMMAND;
        SERVER_STATUS;
    }


    // --frame (-f)

    if ( cmdline_parser.value(roiOption) != "" ) {
        command_packet.SetCommand(NETPROTOCOL_COMMAND_ROI,roi_vals);
        info_stream << command_packet.GetPacketContent() << " ";
        SEND_COMMAND;
        SERVER_STATUS;
    }


    // --exp (-e)

    if ( cmdline_parser.value(expOption) != "" ) {
        command_packet.SetCommand(NETPROTOCOL_COMMAND_EXPTIME,exp_time);
        info_stream << command_packet.GetPacketContent() << " ";
        SEND_COMMAND;
        SERVER_STATUS;
    }


    // --shutter (-s)

    if ( send_shutter ) { // if FITS-file is given then send SHUTTER-command
        command_packet.SetCommand(NETPROTOCOL_COMMAND_SHUTTER, shutter_state == 0 ? 0.0 : 1.0);
        info_stream << command_packet.GetPacketContent() << " ";
        SEND_COMMAND;
        SERVER_STATUS;
    }


    // --cooler (-c)

    if ( !cooler_str.isEmpty() ) {
        command_packet.SetCommand(NETPROTOCOL_COMMAND_COOLER,cooler_str);
        info_stream << command_packet.GetPacketContent() << " ";
        SEND_COMMAND;
        SERVER_STATUS;
    }


    // --fan

    if ( !fan_str.isEmpty() ) {
        command_packet.SetCommand(NETPROTOCOL_COMMAND_FAN,fan_str);
//        qDebug() << "FAN: " << command_packet.GetByteView();
        info_stream << command_packet.GetPacketContent() << " ";
        SEND_COMMAND;
        SERVER_STATUS;
    }


    // --settemp

    if ( cmdline_parser.value(tempOption) != "" ) {
        command_packet.SetCommand(NETPROTOCOL_COMMAND_SETTEMP,ccd_temp);
        info_stream << command_packet.GetPacketContent() << " ";
        SEND_COMMAND;
        SERVER_STATUS;
    }


    // positional arguments (FITS and optional header files) if present

//    QStringList pos_args = cmdline_parser.positionalArguments();

    if ( !pos_args.isEmpty() ) {
        // result FITS-file name
        command_packet.SetCommand(NETPROTOCOL_COMMAND_FITSFILE,pos_args[0]);
        info_stream << command_packet.GetPacketContent() << " ";
        SEND_COMMAND;
        SERVER_STATUS;

        // user FITS-header file
        if ( pos_args.length() > 1 ) {
            command_packet.SetCommand(NETPROTOCOL_COMMAND_HEADFILE,pos_args[1]);
            info_stream << command_packet.GetPacketContent() << " ";
            SEND_COMMAND;
            SERVER_STATUS;
        }
    }


    info_stream.flush();

    InfoNetPacket info_pk(info_str);
    ok = info_pk.Send(&socket);
    if ( !ok ) {
        return CLIENT_ERROR_CONNECTION;
    }

    SERVER_STATUS;

    socket.disconnect(); // disconnect all slots

#ifdef QT_DEBUG
    qDebug() << "Disconnect from server";
#endif

//    socket.disconnectFromHost();

    return CLIENT_ERROR_OK;
}
