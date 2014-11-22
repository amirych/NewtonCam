#ifndef NETPACKETHANDLER_H
#define NETPACKETHANDLER_H


#include "netpacket.h"
#include "proto_defs.h"

#include <QObject>
#include <QtNetwork/QTcpSocket>
#include <QList>
#include <QString>
#include <QThread>

class NET_PROTOCOLSHARED_EXPORT NetPacketHandler : public QObject
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

//    bool SendPacket(NetPacket *packet, int timeout = NETPROTOCOL_TIMEOUT);
//    bool SendPacket(QList<QTcpSocket*> *queue, NetPacket* packet, int timeout = NETPROTOCOL_TIMEOUT);

    void AddSocket(QTcpSocket *socket); // add socket to send queue

    NetPacket* GetPacket(); // extract first received packet from a queue and return it to caller

    NetPacketHandlerError GetLastError() const;

    void SetNetworkTimeout(const int timeout);

    void Reset();

signals:
    void PacketIsReceived();
    void PacketIsSent();
    void Error();

public slots:
    void ReadDataStream();
    void SendPacket(NetPacket *packet);

private slots:
    void SocketDisconnected();

private:
    QTcpSocket *current_socket;

//    QList<NetPacket*> packet_queue;

    QList<QTcpSocket*> send_socket_queue;
    QList<NetPacket*> receive_queue;

    NetPacketHandlerError lastError;

    int networkTimeout;

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
