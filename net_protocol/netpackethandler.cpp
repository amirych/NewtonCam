#include "netpackethandler.h"


                /*  Constructors and destructor  */

NetPacketHandler::NetPacketHandler(QObject *parent):
    QObject(parent), packet_queue(QList<NetPacket*>()), current_socket(nullptr),
    newPacket(true)
{
}


NetPacketHandler::NetPacketHandler(QTcpSocket *socket, QObject *parent):
    QObject(parent), current_socket(socket), packet_queue(QList<NetPacket*>()),
    newPacket(true)
{

}


NetPacketHandler::~NetPacketHandler()
{
    if ( !packet_queue.empty() ) {
        foreach (NetPacket *pk, packet_queue) {
           delete pk;
        }
    }
}


                /*  Public methods  */

void NetPacketHandler::SetSocket(QTcpSocket *socket)
{
    current_socket = socket;
}


NetPacket* NetPacketHandler::GetPacket()
{
    if ( packet_queue.isEmpty() ) return nullptr;

    return packet_queue.takeFirst();
}


bool NetPacketHandler::SendPacket(NetPacket *packet, int timeout)
{
    if ( current_socket == nullptr ) return false;

    qint64 n = current_socket->write(packet->GetByteView());

    if ( n == -1 ) return false;

    return current_socket->waitForBytesWritten(timeout);
}



                /*  Public slots  */

void NetPacketHandler::ReadDataStream()
{
    if ( current_socket == nullptr ) return;

    if ( newPacket ) { // read header (ID and LEN fields)

    }
}
