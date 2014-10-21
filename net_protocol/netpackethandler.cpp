#include "netpackethandler.h"


                /*  Constructors and destructor  */

NetPacketHandler::NetPacketHandler(QTcpSocket *socket, QObject *parent):
    QObject(parent), current_socket(socket), packet_queue(QList<NetPacket*>()),
    newPacket(true), lastError(PACKET_ERROR_OK)
{
    packet = new NetPacket();
}

NetPacketHandler::NetPacketHandler(QObject *parent): NetPacketHandler(nullptr,parent)
{

}

NetPacketHandler::~NetPacketHandler()
{
    delete packet;

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
    lastError = NetPacketHandler::PACKET_ERROR_OK;
}


NetPacket* NetPacketHandler::GetPacket()
{
    if ( packet_queue.isEmpty() ) {
        lastError = NetPacketHandler::PACKET_ERROR_EMPTY_QUEUE;
        return nullptr;
    }

    lastError = NetPacketHandler::PACKET_ERROR_OK;
    return packet_queue.takeFirst();
}


bool NetPacketHandler::SendPacket(NetPacket *packet, int timeout)
{
    if ( current_socket == nullptr ) {
        lastError = NetPacketHandler::PACKET_ERROR_NO_SOCKET;
        return false;
    }

    qint64 n = current_socket->write(packet->GetByteView());

    if ( n == -1 ) {
        lastError = NetPacketHandler::PACKET_ERROR_NETWORK;
        return false;
    }

    bool ok = current_socket->waitForBytesWritten(timeout);
    if ( !ok ) {
        lastError = NetPacketHandler::PACKET_ERROR_NETWORK;
    } else lastError = NetPacketHandler::PACKET_ERROR_OK;

    return ok;
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
        packet_queue << packet;
        return;
    }

    switch (packet->GetPacketID()) {
    case NetPacket::PACKET_ID_INFO: {
        packet_queue << packet;
        break;
    }
    case NetPacket::PACKET_ID_CMD: {
        CmdNetPacket *pk = new CmdNetPacket(*packet);
        packet_queue << pk;
        break;
    }
    case NetPacket::PACKET_ID_STATUS: {
        StatusNetPacket *pk = new StatusNetPacket(*packet);
        packet_queue << pk;
        break;
    }
    case NetPacket::PACKET_ID_TEMP: {
        TempNetPacket *pk = new TempNetPacket(*packet);
        packet_queue << pk;
        break;
    }
    case NetPacket::PACKET_ID_HELLO: {
        HelloNetPacket *pk = new HelloNetPacket(*packet);
        packet_queue << pk;
        break;
    }
    default:
        break;
    }
//    bool ok;

//    lastError = NetPacketHandler::PACKET_ERROR_OK;

//    if ( newPacket ) { // read header (ID and LEN fields)
//        if ( current_socket->bytesAvailable() < (NETPROTOCOL_ID_FIELD_LEN + NETPROTOCOL_SIZE_FIELD_LEN) ) {
//            lastError = NetPacketHandler::PACKET_ERROR_WAIT;
//            return;
//        }

//        Packet = current_socket->read(NETPROTOCOL_ID_FIELD_LEN + NETPROTOCOL_SIZE_FIELD_LEN);
//        if ( Packet.length() < (NETPROTOCOL_ID_FIELD_LEN + NETPROTOCOL_SIZE_FIELD_LEN) ) {
//            lastError = NetPacketHandler::PACKET_ERROR_NETWORK;
//            return;
//        }


//#ifdef QT_DEBUG
//        qDebug() << "NETPACKET: read header [" << Packet << "]";
//#endif

//        long id_num = Packet.left(NETPROTOCOL_ID_FIELD_LEN).toLong(&ok);
//        if ( !ok ) {
//            lastError = NetPacketHandler::PACKET_ERROR_BAD_NUMERIC;
//            return;
//        }

//        ID = static_cast<NetPacket::NetPacketID>(id_num);
//        if ( ID >= NetPacket::PACKET_ID_UNKNOWN) {
//            lastError = NetPacketHandler::PACKET_ERROR_UNKNOWN_PROTOCOL;
//            return;
//        }

//        Content_LEN = Packet.right(NETPROTOCOL_SIZE_FIELD_LEN).toLong(&ok);
//        if ( !ok ) {
//            lastError = NetPacketHandler::PACKET_ERROR_BAD_NUMERIC;
//            return;
//        }

//        if ( Content_LEN < 0 || Content_LEN > NETPROTOCOL_MAX_CONTENT_LEN ) {
//            lastError = NetPacketHandler::PACKET_ERROR_UNKNOWN_PROTOCOL;
//            return;
//        }


//        newPacket = false;

//        if ( current_socket->bytesAvailable() < Content_LEN ) {
//            lastError = NetPacketHandler::PACKET_ERROR_WAIT;
//            return;
//        }
//    }

//    Packet = current_socket->read(Content_LEN);
//    if ( Packet.length() < Content_LEN ) {
//        lastError = NetPacketHandler::PACKET_ERROR_NETWORK;
//        newPacket = true; // restor handler state to "ready for new packet"
//        return;
//    }

//    Content = Packet.data();
//    lastError = NetPacketHandler::PACKET_ERROR_OK;

//    // parse content
//    switch (ID) {
//    case NetPacket::PACKET_ID_INFO: {
//        InfoNetPacket *pk = new InfoNetPacket(Content);
//        packet_queue << pk;
//        break;
//    }
//    case NetPacket::PACKET_ID_CMD: {
//        QString cmd, args;

//        SplitContent(cmd,args);

//        CmdNetPacket *pk = new CmdNetPacket(cmd,args);
//        packet_queue << pk;
//        break;
//    }
//    case NetPacket::PACKET_ID_STATUS: {
//        QString err_no_str, err_str;
//        status_t err_no;

//        SplitContent(err_no_str,err_str);

//        bool ok;

//        err_no = err_no_str.toLong(&ok);
//        if ( !ok ) {
//            lastError = NetPacketHandler::PACKET_ERROR_BAD_NUMERIC;
//            break;
//        }

//        StatusNetPacket *pk =  new StatusNetPacket(err_no,err_str);
//        packet_queue << pk;
//        break;
//    }
//    case NetPacket::PACKET_ID_HELLO: {
//        NetPacket *pk = new NetPacket(NetPacket::PACKET_ID_HELLO,Content);
//        packet_queue << pk;
//        break;
//    }
//    default:
//        lastError = NetPacketHandler::PACKET_ERROR_UNKNOWN_PROTOCOL;
//        break;
//    }

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
