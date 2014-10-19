#ifndef NETPACKETHANDLER_H
#define NETPACKETHANDLER_H


#include "netpacket.h"
#include "proto_defs.h"

#include <QObject>
#include <QtNetwork/QTcpSocket>
#include <QList>
#include <QString>

class NetPacketHandler : public QObject
{
    Q_OBJECT
public:
    enum NetPacketHandlerError {PACKET_ERROR_OK, PACKET_ERROR_NETWORK,
                                PACKET_ERROR_UNKNOWN_PROTOCOL, PACKET_ERROR_CONTENT_LEN,
                                PACKET_ERROR_BAD_NUMERIC, PACKET_ERROR_WAIT,
                                PACKET_ERROR_NO_SOCKET, PACKET_ERROR_EMPTY_QUEUE};

    explicit NetPacketHandler(QObject *parent = 0);
    NetPacketHandler(QTcpSocket *socket, QObject *parent = 0);

    ~NetPacketHandler();

    void SetSocket(QTcpSocket *socket);

    bool SendPacket(NetPacket *packet, int timeout = NETPROTOCOL_TIMEOUT);

    NetPacket* GetPacket(); // extract first received packet from a queue and return it to caller

    NetPacketHandlerError GetLastError() const;

signals:
    void PacketIsReceived();
    void PacketIsSent();
    void Error();

public slots:
    void ReadDataStream();

private:
    QTcpSocket *current_socket;

    QList<NetPacket*> packet_queue;

    NetPacketHandlerError lastError;

    // current recieved packet fields
    NetPacket *packet;

    bool newPacket;

    NetPacket::NetPacketID ID;
    long Content_LEN;
    QString Content;
    QByteArray Packet;

    void SplitContent(QString &left, QString &right);
};

#endif // NETPACKETHANDLER_H
