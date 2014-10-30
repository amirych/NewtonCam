#include "netpacket.h"
#include "proto_defs.h"
#include "../server/server.h"

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

enum ClientError {CLIENT_ERROR_OK, CLIENT_ERROR_INVALID_OPTION = 10, CLIENT_ERROR_CONNECTION,
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
        return CLIENT_ERROR_CONNECTION; \
    } \
    status = server_status_packet.GetStatus(); \
    qDebug() << "<> SERVER RESPONDS: " << status; \
    if ( status != Server::SERVER_ERROR_OK ) {\
        return status; \
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


    HelloNetPacket hello;
    StatusNetPacket server_status_packet;
    status_t status;

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

                /*  Send HELLO message  */

    hello.SetSenderType(NETPROTOCOL_SENDER_TYPE_CLIENT,"");
    ok = hello.Send(&socket,NETPROTOCOL_TIMEOUT);
    if ( !ok ) {
        return CLIENT_ERROR_CONNECTION;
    }
    SERVER_STATUS;

                /*  Send commands to server  */

    CmdNetPacket command_packet;

    // --stop

    if ( cmdline_parser.isSet(stopOption) ) {
        command_packet.SetCommand(NETPROTOCOL_COMMAND_STOP,"");
        SEND_COMMAND;
        SERVER_STATUS;
    }


    // --init
    if ( cmdline_parser.isSet(initOption) ) {
        command_packet.SetCommand(NETPROTOCOL_COMMAND_INIT,"");
        SEND_COMMAND;
        SERVER_STATUS;
    }

    // --gettemp, it is a special command! The program receives a CCD temperature and exit!

    if ( cmdline_parser.isSet(gettempOption)) {
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
        std::cout << ccd_temp << " " << cooling_status;

        socket.disconnectFromHost();
        return status;
    }



    // --bin (-b)

    if ( cmdline_parser.value(binOption) != "" ) {
        command_packet.SetCommand(NETPROTOCOL_COMMAND_BINNING,bin_vals);
        SEND_COMMAND;
        SERVER_STATUS;
    }

    // --rate (-r)

    if ( cmdline_parser.value(rateOption) != "" ) {
        command_packet.SetCommand(NETPROTOCOL_COMMAND_RATE,cmdline_parser.value(rateOption));
        SEND_COMMAND;
        SERVER_STATUS;
    }


    // --gain (-g)

    if ( cmdline_parser.value(gainOption) != "" ) {
        command_packet.SetCommand(NETPROTOCOL_COMMAND_GAIN,cmdline_parser.value(gainOption));
        SEND_COMMAND;
        SERVER_STATUS;
    }


    // --frame (-f)

    if ( cmdline_parser.value(roiOption) != "" ) {
        command_packet.SetCommand(NETPROTOCOL_COMMAND_ROI,roi_vals);
        SEND_COMMAND;
        SERVER_STATUS;
    }


    // --exp (-e)

    if ( cmdline_parser.value(expOption) != "" ) {
        command_packet.SetCommand(NETPROTOCOL_COMMAND_EXPTIME,exp_time);
        SEND_COMMAND;
        SERVER_STATUS;
    }


    // --shutter (-s)

    command_packet.SetCommand(NETPROTOCOL_COMMAND_SHUTTER, shutter_state == 0 ? 0.0 : 1.0);
    SEND_COMMAND;
    SERVER_STATUS;


    // --settemp

    if ( cmdline_parser.value(tempOption) != "" ) {
        command_packet.SetCommand(NETPROTOCOL_COMMAND_SETTEMP,ccd_temp);
        SEND_COMMAND;
        SERVER_STATUS;
    }


    // positional arguments (FITS and optional header files) if present

    QStringList pos_args = cmdline_parser.positionalArguments();

    if ( !pos_args.isEmpty() ) {
        // result FITS-file name
        command_packet.SetCommand(NETPROTOCOL_COMMAND_FITSFILE,pos_args[0]);
        SEND_COMMAND;
        SERVER_STATUS;

        // user FITS-header file
        if ( pos_args.length() > 1 ) {
            command_packet.SetCommand(NETPROTOCOL_COMMAND_HEADFILE,pos_args[1]);
            SEND_COMMAND;
            SERVER_STATUS;
        }
    }

#ifdef QT_DEBUG
    qDebug() << "Disconnect from server";
#endif


    InfoNetPacket ii("command-line from client");
    ii.Send(&socket);
    server_status_packet.Receive(&socket);

    socket.disconnect(); // disconnect all slots

    socket.disconnectFromHost();

    return CLIENT_ERROR_OK;
}
