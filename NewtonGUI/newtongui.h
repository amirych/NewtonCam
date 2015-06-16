#ifndef NEWTONGUI_H
#define NEWTONGUI_H

#include "proto_defs.h"
#include "netpacket.h"
#include "netpackethandler.h"
#include "servergui.h"

#include <QObject>
#include <QWidget>
#include <QTcpSocket>
#include <QHostAddress>
#include <QMessageBox>
#include <QThread>


#define NEWTONGUI_ERROR_INVALID_PORT 10
#define NEWTONGUI_ERROR_CANNOT_CONNECT 11
#define NEWTONGUI_ERROR_NETWORK 12
#define NEWTONGUI_ERROR_INVALID_SERVER 13
#define NEWTONGUI_ERROR_INVALID_FONTSIZE 14
#define NEWTONGUI_ERROR_INVALID_HOSTNAME 15
#define NEWTONGUI_ERROR_INVALID_COORDINATE 16

//class NewtonGui: public QWidget
class NewtonGui: public ServerGUI
{
    Q_OBJECT

public:
    NewtonGui(int fontsize = SERVERGUI_DEFAULT_FONTSIZE);

    void Connect(QHostAddress &server_addr, quint16 port = NETPROTOCOL_DEFAULT_PORT);

//    void SetFonts(int fontsize, int status_fontsize, int log_fontsize);

    ~NewtonGui();

signals:
    void Error(int err_code);

private slots:
    void ServerDisconnected();
    void ServerMsgIsReceived();

private:
    QTcpSocket* socket;
    NetPacketHandler* pk_handler;
    QThread *pk_handler_thread;

//    ServerGUI* serverGUI;
};

#endif // NEWTONGUI_H
