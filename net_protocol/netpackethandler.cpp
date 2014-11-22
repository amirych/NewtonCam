#include "netpackethandler.h"


                /*  Constructors and destructor  */

NetPacketHandler::NetPacketHandler(QTcpSocket *socket, QObject *parent):
    QObject(parent), current_socket(socket),
    send_socket_queue(QList<QTcpSocket*>()), receive_queue(QList<NetPacket*>()),
    lastError(PACKET_ERROR_OK), networkTimeout(NETPROTOCOL_TIMEOUT), newPacket(true)
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
    packet = nullptr;

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


void NetPacketHandler::AddSocket(QTcpSocket *socket)
{
    send_socket_queue << socket;
    connect(socket,SIGNAL(disconnected()),this,SLOT(SocketDisconnected()));
}


void NetPacketHandler::SetNetworkTimeout(const int timeout)
{
    networkTimeout = timeout;
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


//bool NetPacketHandler::SendPacket(NetPacket *packet, int timeout)
//{
//    if ( current_socket == nullptr ) {
//        lastError = NetPacketHandler::PACKET_ERROR_NO_SOCKET;
//        return false;
//    }

//    return packet->Send(current_socket, timeout);
//}


//bool NetPacketHandler::SendPacket(QList<QTcpSocket *> *queue, NetPacket *packet, int timeout)
//{
//    if ( queue == nullptr ) return true;
//    if ( queue->isEmpty() ) return true;

//    foreach (QTcpSocket* socket, *queue) {
//        bool ok = packet->Send(socket,timeout);
//        if ( !ok ) return ok;
//    }

//    return true;
//}


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
    case GuiNetPacket::PACKET_ID_GUI: {
        GuiNetPacket *pk = new GuiNetPacket(*packet);
        receive_queue << pk;
        break;
    }
    default:
        break;
    }

#ifdef QT_DEBUG
    qDebug() << "PACKETHANDLER: received " << packet->GetByteView();
#endif

    packet = new NetPacket();
    newPacket = true;

    emit PacketIsReceived();


    // read remained data from socket
    if ( current_socket->bytesAvailable() ) ReadDataStream();
}


void NetPacketHandler::SendPacket(NetPacket *packet)
{
    if ( send_socket_queue.isEmpty() ) return;

#ifdef QT_DEBUG
    qDebug() << "PACKETHANDLER: sending: " << packet->GetByteView();
#endif

    foreach (QTcpSocket* socket, send_socket_queue) {
        packet->Send(socket,networkTimeout);
//        bool ok = packet->Send(socket,networkTimeout);
//        if ( !ok ) return;
    }
}

                /*  Private slots   */

void NetPacketHandler::SocketDisconnected()
{
    QObject* sender = QObject::sender();

    if ( sender != nullptr ) {
        int i = 0;
        foreach (QTcpSocket* sk, send_socket_queue) {
            if ( sk == sender ) {
                sk->disconnect();
#ifdef QT_DEBUG
                qDebug() << "PACKETHANDLER: socket " << sk << " disconnected";
#endif
                send_socket_queue.removeAt(i);
                return;
            }
            ++i;
        }
    }
}
                /*  Private methods  */

