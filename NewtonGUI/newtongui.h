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


#define NEWTONGUI_ERROR_INVALID_PORT 10
#define NEWTONGUI_ERROR_CANNOT_CONNECT 11
#define NEWTONGUI_ERROR_NETWORK 12
#define NEWTONGUI_ERROR_INVALID_SERVER 13
#define NEWTONGUI_ERROR_INVALID_FONTSIZE 14


class NewtonGui: public QWidget
{
    Q_OBJECT

public:
    NewtonGui(int fontsize = SERVERGUI_DEFAULT_FONTSIZE);

    void Connect(QHostAddress &server_addr, quint16 port = NETPROTOCOL_DEFAULT_PORT);

    ~NewtonGui();

signals:
    void Error(int err_code);

private slots:
    void ServerDisconnected();
    void ServerMsgIsReceived();

private:
    QTcpSocket* socket;
    NetPacketHandler* pk_handler;

    ServerGUI* serverGUI;
};

#endif // NEWTONGUI_H
