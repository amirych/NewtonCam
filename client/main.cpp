#include "netpacket.h"

#include <QCoreApplication>
#include <QTcpSocket>
#include <QString>
#include <QTimer>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    CmdNetPacket pk("EXP",2.57);

    QString addr = "127.0.0.1";
    QTcpSocket s;

    QObject::connect(&s,SIGNAL(disconnected()),&a,SLOT(quit()));

    s.connectToHost(addr,7777);
    if ( !s.waitForConnected() ) {
        exit(10);
    }

    pk.Send(&s);

    QTimer::singleShot(10000,&s,SLOT(disconnectFromHostImplementation()));

//    s.disconnectFromHost();

//    return 0;
    return a.exec();
}
