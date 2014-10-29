#ifndef NEWTONGUI_H
#define NEWTONGUI_H

#include "proto_defs.h"
#include "netpacket.h"
#include "netpackethandler.h"
#include "servergui.h"

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QMessageBox>


class NewtonGui: QObject
{
    Q_OBJECT

public:
    NewtonGui(QObject* parent, QHostAddress &server_addr, quint16 port = NETPROTOCOL_DEFAULT_PORT);

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
