#include "netpackethandler.h"


                /*  Constructors and destructor  */

NetPacketHandler::NetPacketHandler(QTcpSocket *socket, QObject *parent):
    QObject(parent), current_socket(socket), receive_queue(QList<NetPacket*>()),
    newPacket(true), lastError(PACKET_ERROR_OK)
{
    packet = new NetPacket();
}

NetPacketHandler::NetPacketHandler(QObject *parent): NetPacketHandler(nullptr,parent)
{

}

NetPacketHandler::~NetPacketHandler()
{
//    qDebug() << "DISTR: " << packet << ", " << receive_queue;
    delete packet;

    if ( !receive_queue.empty() ) {
        foreach (NetPacket *pk, receive_queue) {
           delete pk;
        }
    }
}


                /*  Public methods  */

void NetPacketHandler::SetSocket(QTcpSocket *socket)
{
    current_socket = socket;
    lastError = NetPacketHandler::PACKET_ERROR_OK;
}


NetPacket* NetPacketHandler::GetPacket()
{
    if ( receive_queue.isEmpty() ) {
        lastError = NetPacketHandler::PACKET_ERROR_EMPTY_QUEUE;
        return nullptr;
    }

    lastError = NetPacketHandler::PACKET_ERROR_OK;
    return receive_queue.takeFirst();
}


bool NetPacketHandler::SendPacket(NetPacket *packet, int timeout)
{
    if ( current_socket == nullptr ) {
        lastError = NetPacketHandler::PACKET_ERROR_NO_SOCKET;
        return false;
    }

    return packet->Send(current_socket, timeout);
}


bool NetPacketHandler::SendPacket(QList<QTcpSocket *> *queue, NetPacket *packet, int timeout)
{
    if ( queue == nullptr ) return true;
    if ( queue->isEmpty() ) return true;

    foreach (QTcpSocket* socket, *queue) {
        bool ok = packet->Send(socket,timeout);
        if ( !ok ) return ok;
    }

    return true;
}


NetPacketHandler::NetPacketHandlerError NetPacketHandler::GetLastError() const
{
    return lastError;
}


void NetPacketHandler::Reset()
{
    packet->SetContent(NetPacket::PACKET_ID_UNKNOWN,""); // discard all previous received data in packet (e.g. an error occured on socket)
}

                /*  Public slots  */

void NetPacketHandler::ReadDataStream()
{
    if ( current_socket == nullptr ) {
        lastError = NetPacketHandler::PACKET_ERROR_NO_SOCKET;
        return;
    }

    packet->Receive(current_socket,10);
    if ( packet->GetPacketError() == NetPacket::PACKET_ERROR_WAIT ) return; // wait for new data

    if ( packet->GetPacketError() != NetPacket::PACKET_ERROR_OK ) {
        return;
    }

    switch (packet->GetPacketID()) {
    case NetPacket::PACKET_ID_INFO: {
        receive_queue << packet;
        break;
    }
    case NetPacket::PACKET_ID_CMD: {
        CmdNetPacket *pk = new CmdNetPacket(*packet);
        receive_queue << pk;
        break;
    }
    case NetPacket::PACKET_ID_STATUS: {
        StatusNetPacket *pk = new StatusNetPacket(*packet);
        receive_queue << pk;
        break;
    }
    case NetPacket::PACKET_ID_TEMP: {
        TempNetPacket *pk = new TempNetPacket(*packet);
        receive_queue << pk;
        break;
    }
    case NetPacket::PACKET_ID_HELLO: {
        HelloNetPacket *pk = new HelloNetPacket(*packet);
        receive_queue << pk;
        break;
    }
    default:
        break;
    }

    packet = new NetPacket();
    newPacket = true;

    emit PacketIsReceived();


    // read remained data from socket
    if ( current_socket->bytesAvailable() ) ReadDataStream();
}



                /*  Private methods  */

void NetPacketHandler::SplitContent(QString &left, QString &right)
{
    if ( Content.isEmpty() || Content.isNull() ) {
        left.clear();
        right.clear();
    }

    int idx = Content.indexOf(NETPROTOCOL_CONTENT_DELIMETER, Qt::CaseInsensitive);

    if ( idx == -1 ) {
        left = Content;
        right = "";
    } else {
        left = Content.mid(0,idx);
        right = Content.mid(idx);
    }
}
