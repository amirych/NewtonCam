#include "server.h"
#include <QCoreApplication>
#include <QTimer>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Server s;

    QTimer::singleShot(20000,&a,SLOT(quit()));
    return a.exec();
}
