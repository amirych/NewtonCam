#include "netpacket.h"

#include <QCoreApplication>
#include <QTcpSocket>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QCommandLineOption>
#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // create commandline options

    QCommandLineOption binOption(QStringList() << "b" << "bin","Set binning",
                                 "1x1","1x1");

    CmdNetPacket pk("EXP",2.57);
    InfoNetPacket ipk("This is the test client!!!");

    QString addr = "127.0.0.1";
    QTcpSocket s;

    QObject::connect(&s,SIGNAL(disconnected()),&a,SLOT(quit()));

    s.connectToHost(addr,7777);
    if ( !s.waitForConnected() ) {
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



    QTimer::singleShot(3000,&s,SLOT(disconnectFromHostImplementation()));
//    s.disconnectFromHost();

//    return 0;
    return a.exec();
}
