#include "netpacket.h"
#include "proto_defs.h"

#include <QCoreApplication>
#include <QTcpSocket>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QTimer>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QRegExp>
#include <iostream>

#include <chrono>
#include <thread>

#define CLIENT_DEFAULT_SERVER_IP "127.0.0.1"

// client errors
#define CLIENT_ERROR_INVALID_OPTION 10
#define CLIENT_ERROR_CONNECTION 11

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
    cmdline_parser.addOption(tempOption);
    cmdline_parser.addOption(gettempOption);

    cmdline_parser.addOption(server_ipOption);
    cmdline_parser.addOption(server_portOption);

    cmdline_parser.process(a);

    QString bb = cmdline_parser.value(binOption);
    QString gg = cmdline_parser.value(roiOption);

    QStringList aa = cmdline_parser.positionalArguments();

    qDebug() << "   BIN: " << bb << "\n";
    qDebug() << "   ROI: " << gg << "\n";
    qDebug() << "   ARGS: " << aa;

    // is client was run without arguments and options
    if ( a.arguments().length() == 1 ) {
        qDebug() << "There were no arguments and options!";
        return 0;
    }

    /*  Parsing some command-line options  */

    // binning
    QString bin_str = cmdline_parser.value(binOption);
    QVector<double> bin_vals;
    if ( !bin_str.isEmpty() ) {
        QRegExp rx("\\s*\\d+x\\d+\\s*"); // binning string must be in format XBINxYBIN
        bool ok = rx.exactMatch(bin_str);
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
        bool ok = rx.exactMatch(roi_str);
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
        bool ok;
        exp_time = exp_str.toDouble(&ok);
        if (!ok || (exp_time < 0.0) ){
            qDebug() << "BAD EXP TIME!";
            return CLIENT_ERROR_INVALID_OPTION;
        }
    }

    // CCD temperature
    QString temp_str = cmdline_parser.value(tempOption);
    double ccd_temp;
    if ( !temp_str.isEmpty() ) {
        bool ok;
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

                /*  Send commands to server  */

    CmdNetPacket command_packet;
    StatusNetPacket server_status_packet;
    status_t status;

    // --stop

    if ( cmdline_parser.isSet(stopOption) ) {
        command_packet.SetCommand(NETPROTOCOL_COMMAND_STOP,"");
        command_packet.Send(&socket,NETPROTOCOL_TIMEOUT);
        server_status_packet.Receive(&socket,NETPROTOCOL_TIMEOUT);
        if ( !server_status_packet.isPacketValid() ) {
            qDebug() << "BAD SERVER STATUS PACKET!";
            return CLIENT_ERROR_CONNECTION;
        }
        status = server_status_packet.GetStatus();
    }


    CmdNetPacket pk("EXP",2.57);
    InfoNetPacket ipk("This is the test client!!!");

    QString addr = "127.0.0.1";
    QTcpSocket s;

    QObject::connect(&s,SIGNAL(disconnected()),&a,SLOT(quit()));

//    s.connectToHost(addr,7777);
    s.connectToHost(server_ip,server_port);
    if ( !s.waitForConnected() ) {
        std::cerr << "Cannot connect to server!\n";
        exit(10);
    }

    StatusNetPacket st;
    NetPacket pp;
    bool ok;
    for ( long i = 0; i < 10; ++i ) {
        std::cerr << "------------(" << i << ")-----------------\n";
        ok = pk.Send(&s);
        if ( !ok ) {
            std::cerr << "Cannot send CMD-packet!\n";
            break;
        } else std::cerr << "Sent CMD-packet\n";
//        st.Receive(&s,10000);
//        while (st.GetPacketError() == NetPacket::PACKET_ERROR_WAIT ) {
//            if ( !s.waitForReadyRead(10000) ) break;
//            st.Receive(&s,0);
//        }
        pp.Receive(&s,10000);
        st = pp;
        if ( !st.isPacketValid() ) {
            std::cerr << "Bad server response!\n";
            break;
        } else {
            std::cerr << "CLIENT: server response: [" << st.GetByteView().data() <<
                         "] (generic: " << pp.GetByteView().data() << ")\n";
        }
        ok = ipk.Send(&s);
        if ( !ok ) {
            std::cerr << "Cannot send INFO-packet!\n";
            break;
        } else std::cerr << "Sent INFO-packet\n";
    }

//    QTimer::singleShot(3000,&s,SLOT(disconnectFromHostImplementation()));
//    QTimer::singleShot(3000,&s,"");

    std::this_thread::sleep_for(std::chrono::seconds(5));

    s.disconnectFromHost();

//    QTimer::singleShot(3000,&s,SLOT(disconnectFromHostImplementation())); // No such slot in QT 5.3!!!

//    QTimer::singleShot(3000,&a,SLOT(quit()));

    //    s.disconnectFromHost();

//    return 0;
    return a.exec();
}
