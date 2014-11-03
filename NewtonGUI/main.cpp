#include "newtongui.h"
#include "servergui.h"
#include "../net_protocol/proto_defs.h"


#include <QApplication>
#include <QHostAddress>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QString>
#include <QStringList>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCommandLineParser cmdline_parser;

    cmdline_parser.addHelpOption();

    // create commandline options

//    QCommandLineOption serverOption(QStringList() << "h" << "host", "NewtonCam server address", "host name or IP", "127.0.0.1");

    QString str;
    str.setNum(NETPROTOCOL_DEFAULT_PORT);

    QCommandLineOption portOption(QStringList() << "p" << "port", "NewtonCam server port", "numeric value", str);

    str.setNum(SERVERGUI_DEFAULT_FONTSIZE);
    QCommandLineOption fontOption(QStringList() << "f" << "fontsize", "GUI font size", "numeric value", str);

    cmdline_parser.addOption(portOption);
    cmdline_parser.addOption(fontOption);

    cmdline_parser.addPositionalArgument("IP-address", "NewtonCam server address (default 127.0.0.1)");

    cmdline_parser.process(a);

    quint16 port;
    bool ok;

    port = cmdline_parser.value(portOption).toUInt(&ok);
    if ( !ok ) {
        QMessageBox::StandardButton bt = QMessageBox::critical(0,"Error","Invalid port value!");
        exit(NEWTONGUI_ERROR_INVALID_PORT);
    }


    int fontsize = cmdline_parser.value(fontOption).toInt(&ok);
    if ( !ok ) {
        QMessageBox::StandardButton bt = QMessageBox::critical(0,"Error","Invalid port value!");
        exit(NEWTONGUI_ERROR_INVALID_FONTSIZE);
    }

    QStringList pos_arg = cmdline_parser.positionalArguments();
    if ( pos_arg.length() == 0 ) {
        str = "127.0.0.1";
    }

    QHostAddress addr(str);

    NewtonGui ng(fontsize);
//    QObject::connect(&ng,&NewtonGui::Error,[=](int err){exit(err);});

    a.processEvents(); // to draw the main window and show start message

    ng.Connect(addr,port);

    return a.exec();
}
