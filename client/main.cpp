#include "netpacket.h"

#include <QCoreApplication>
#include <QTcpSocket>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <iostream>

#include <chrono>
#include <thread>

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

    QCommandLineOption binOption(QStringList() << "b" << "bin","Set binning",
                                 "binning string");

    QCommandLineOption geometryOption(QStringList() << "f" << "frame", "Set readout region",
                                      "geometry array");

    QCommandLineOption rateOption(QStringList() << "r" << "rate", "Set readout rate", "rate string");

    QCommandLineOption tempOption(QStringList() << "t" << "temp", "Set CCD chip temperature","degrees");

    QCommandLineOption gettempOption(QStringList() << "g" << "gettemp","Get CCD chip temperature");


    cmdline_parser.addPositionalArgument("FITS-file","Output FITS filename");
    cmdline_parser.addPositionalArgument("HEADER-file","Additional FITS-header filename (optional)","[HEADER-file]");

    cmdline_parser.addOption(initOption);
    cmdline_parser.addOption(stopOption);

    cmdline_parser.addOption(expOption);
    cmdline_parser.addOption(binOption);
    cmdline_parser.addOption(geometryOption);
    cmdline_parser.addOption(rateOption);
    cmdline_parser.addOption(tempOption);
    cmdline_parser.addOption(gettempOption);


    cmdline_parser.process(a);

    QString bb = cmdline_parser.value(binOption);
    QString gg = cmdline_parser.value(geometryOption);

    QStringList aa = cmdline_parser.positionalArguments();

    qDebug() << "   BIN: " << bb << "\n";
    qDebug() << "   ROI: " << gg << "\n";
    qDebug() << "   ARGS: " << aa;

    CmdNetPacket pk("EXP",2.57);
    InfoNetPacket ipk("This is the test client!!!");

    QString addr = "127.0.0.1";
    QTcpSocket s;

    QObject::connect(&s,SIGNAL(disconnected()),&a,SLOT(quit()));

    s.connectToHost(addr,7777);
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
