#include <QCoreApplication>


#include "netpacket.h"

#include<iostream>
#include<QDebug>

using namespace std;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);


    CmdNetPacket pk("EXP TIME",10.00009);

    QVector<double> aa;

    bool ok = pk.GetCmdArgs(&aa);

    cout << "NAME: " << pk.GetCmdName().toStdString().c_str() <<
            "; ARGS: " << aa[0] << endl;
    cout << "IS NUM? " << ok << endl;


    cout << "PACKET: " << pk.GetByteView().data() << endl;

    return a.exec();
}
