#include "newtongui.h"
#include "servergui.h"
#include "../net_protocol/proto_defs.h"


#include <QApplication>
#include <QHostAddress>
#include <QHostInfo>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QString>
#include <QStringList>
#include <QRegExp>

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

    str.setNum(SERVERGUI_DEFAULT_FONTSIZE-2);
    QCommandLineOption statusfontOption(QStringList() << "s" << "statusfontsize", "GUI status font size", "numeric value", str);

    str.setNum(SERVERGUI_DEFAULT_FONTSIZE);
    QCommandLineOption logfontOption(QStringList() << "l" << "logfontsize", "GUI log window font size", "numeric value", str);

    cmdline_parser.addOption(portOption);
    cmdline_parser.addOption(fontOption);
    cmdline_parser.addOption(statusfontOption);
    cmdline_parser.addOption(logfontOption);

    cmdline_parser.addPositionalArgument("IP-address", "NewtonCam server address (default 127.0.0.1)");

    cmdline_parser.process(a);

    quint16 port;
    bool ok;

    port = cmdline_parser.value(portOption).toUInt(&ok);
    if ( !ok ) {
//        QMessageBox::StandardButton bt =
        QMessageBox::critical(0,"Error","Invalid port value!");
        exit(NEWTONGUI_ERROR_INVALID_PORT);
    }


    int fontsize = cmdline_parser.value(fontOption).toInt(&ok);
    if ( !ok ) {
//        QMessageBox::StandardButton bt =
        QMessageBox::critical(0,"Error","Invalid GUI fontsize value!");
        exit(NEWTONGUI_ERROR_INVALID_FONTSIZE);
    }

    int statusFontsize = cmdline_parser.value(statusfontOption).toInt(&ok);
    if ( !ok ) {
        QMessageBox::critical(0,"Error","Invalid GUI status fontsize value!");
        exit(NEWTONGUI_ERROR_INVALID_FONTSIZE);
    }

    int logFontsize = cmdline_parser.value(logfontOption).toInt(&ok);
    if ( !ok ) {
        QMessageBox::critical(0,"Error","Invalid GUI log window fontsize value!");
        exit(NEWTONGUI_ERROR_INVALID_FONTSIZE);
    }

    QHostAddress addr;
    QStringList pos_arg = cmdline_parser.positionalArguments();

    if ( pos_arg.length() == 0 ) {
        str = "127.0.0.1";
        addr.setAddress(str);
    } else {
        str = pos_arg[0];
        QRegExp rx("\\s*\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\s*");
        if ( !rx.exactMatch(str) ) { // do DNS look-up
            QHostInfo info = QHostInfo::fromName(str.trimmed());
            if ( info.addresses().isEmpty() ) {
                QMessageBox::critical(0,"Error","Invalid server hotname!");
                exit(NEWTONGUI_ERROR_INVALID_HOSTNAME);
            }
            addr = info.addresses().first();
        }
    }

    NewtonGui ng(fontsize);
//    QObject::connect(&ng,&NewtonGui::Error,[=](int err){exit(err);});

    a.processEvents(); // to draw the main window and show start message

    ng.Connect(addr,port);

    return a.exec();
}
