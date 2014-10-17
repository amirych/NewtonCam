#ifndef NETPACKETHANDLER_H
#define NETPACKETHANDLER_H


#include "netpacket.h"
#include "proto_defs.h"

#include <QObject>
#include <QTcpSocket>
#include <QList>
#include <QString>

class NetPacketHandler : public QObject
{
    Q_OBJECT
public:
    explicit NetPacketHandler(QObject *parent = 0);
    NetPacketHandler(QTcpSocket *socket, QObject *parent = 0);

    ~NetPacketHandler();

    void SetSocket(QTcpSocket *socket);

    bool SendPacket(NetPacket *packet, int timeout = NETPROTOCOL_TIMEOUT);

    NetPacket* GetPacket(); // extract first received packet from a queue and return it to caller

signals:
    void PacketIsReceived();
    void PacketIsSent();
    void Error();

public slots:
    void ReadDataStream();

private:
    QTcpSocket *current_socket;

    QList<NetPacket*> packet_queue;

    bool newPacket;

    NetPacket::NetPacketID ID;

};

#endif // NETPACKETHANDLER_H
