//#include <QCoreApplication>


#include "netpacket.h"
#include "camera.h"

#include<iostream>
#include<fstream>
#include<QDebug>

using namespace std;

int main(int argc, char *argv[])
{
//    QCoreApplication a(argc, argv);


    CmdNetPacket pk("EXP TIME",10.00009);

    QVector<double> aa;

    bool ok = pk.GetCmdArgs(&aa);

    cout << "NAME: " << pk.GetCmdName().toStdString().c_str() <<
            "; ARGS: " << aa[0] << endl;
    cout << "IS NUM? " << ok << endl;


    cout << "PACKET: " << pk.GetByteView().data() << endl;

    pk.SetCommand("COM","skj  s-kjl s");

    QString ss;

    pk.GetCmdArgs(&ss);

    cout << "NAME: " << pk.GetCmdName().toStdString().c_str() <<
            "; ARGS: " << ss.toStdString().c_str() << endl;

    ok = pk.GetCmdArgs(&aa);
    cout << "IS NUM? " << ok << endl;

    aa.push_back(10);
    aa.push_back(10);
    aa.push_back(110);
    aa.push_back(210);

    pk.SetCommand("   roi   ",aa);

    cout << "PACKET: " << pk.GetByteView().data() << endl;


    StatusNetPacket stpk(-10,"Server time-out!");
    cout << "PACKET: " << stpk.GetByteView().data() << endl;

    ofstream ff("ss");
    Camera cc(ff);
    ff.close();

    cout << "Camera last error: " << cc.GetLastError() << endl;

//    return a.exec();
    return 0;
}
