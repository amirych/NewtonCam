#include "newtongui.h"
#include "servergui.h"


#include <QApplication>
#include <QHostAddress>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//    MainWindow w;
//    w.show();

//    ServerGUI sg;

//    sg.show();

    QHostAddress addr("127.0.0.1");
    NewtonGui ng(0,addr);


    return a.exec();
}
