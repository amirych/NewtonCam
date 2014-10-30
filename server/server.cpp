#include "server.h"
#include "netpacket.h"

#include<QDebug>

                        /********************************
                        *                               *
                        *  Server class implementation  *
                        *                               *
                        ********************************/

#define DOUBLE_TO_INT(double_vec, int_vec) {\
    foreach (double val, double_vec) {\
        int_vec << static_cast<int>(val); \
    }\
}


static QList<QHostAddress> default_hosts_list = QList<QHostAddress>() << QHostAddress(SERVER_DEFAULT_ALLOWED_HOST);


                        /*  Constructors and destructor  */

Server::Server(QList<QHostAddress> &hosts, quint16 port, QObject *parent):
    QTcpServer(parent), serverPort(port), allowed_hosts(hosts),
    guiSocket(QList<QTcpSocket*>()), clientSocket(nullptr),
    packetHandler(nullptr), NetworkTimeout(NETPROTOCOL_TIMEOUT)
{
    if ( allowed_hosts.isEmpty() ) { // set default host
        QString host = SERVER_DEFAULT_ALLOWED_HOST;
        allowed_hosts.append(QHostAddress(host));
    }

    // start listenning

    if ( !listen(QHostAddress::Any,serverPort) ) {
        lastServerError = serverError();
        emit ServerError(lastServerError);
        return;
    }

    packetHandler = new NetPacketHandler(this);

    connect(this,SIGNAL(newConnection()),this,SLOT(ClientConnection()));

    connect(packetHandler,SIGNAL(PacketIsReceived()),this,SLOT(ExecuteCommand()));
}


Server::Server(quint16 port, QObject *parent): Server(default_hosts_list, port, parent)
{

}


Server::Server(QObject *parent): Server(default_hosts_list, NETPROTOCOL_DEFAULT_PORT, parent)
{
}


Server::~Server()
{
}



                    /*  Public members  */

void Server::SetNetworkTimeout(const int timeout)
{
    NetworkTimeout = timeout;
}


                    /*  Private slots  */

// macro for sending server status
#define SEND_STATUS(sk) { \
    ok = server_status_packet.Send(sk,NetworkTimeout); \
    if ( !ok ) { \
        qDebug() << "ERROR OF SERVER STATUS SENDING! " << "Packet error: " << server_status_packet.GetPacketError(); \
        lastServerError = sk->error(); \
        emit ServerError(lastServerError); \
        sk->disconnectFromHost(); \
        return; \
    } \
}
void Server::ClientConnection()
{
    bool ok;
    StatusNetPacket server_status_packet;

    QTcpSocket *socket = nextPendingConnection();

    if ( clientSocket != nullptr ) { // client already connected
        server_status_packet.SetStatus(SERVER_ERROR_BUSY,"Server is busy");

#ifdef QT_DEBUG
        qDebug() << "SERVER MSG: Other client is already connected!";
        qDebug() << "SERVER SENT: " << server_status_packet.GetByteView();
#endif

        SEND_STATUS(socket);
    }

    QHostAddress client_address = socket->peerAddress();

#ifdef QT_DEBUG
    qDebug() << "SERVER: connection from " << client_address;
#endif

    if ( !allowed_hosts.contains(client_address) ) { // check for client is in allowed host list
        server_status_packet.SetStatus(SERVER_ERROR_DENIED,"Server refused the connection");
#ifdef QT_DEBUG
        qDebug() << "SERVER MSG: client address is not allowed!";
        qDebug() << "SERVER SENT: " << server_status_packet.GetByteView();
#endif

        SEND_STATUS(socket);
    }

#ifdef QT_DEBUG
    qDebug() << "SERVER: client IP is valid!";
#endif

            /*  hand-shake with new connection  */

    // send server HELLO message

    HelloNetPacket hello(NETPROTOCOL_SENDER_TYPE_SERVER,"");
    ok = hello.Send(socket,NetworkTimeout);
    if ( !ok ) {
        lastServerError = socket->error();
        emit ServerError(lastServerError);
        socket->disconnectFromHost();
        return;
    }

#ifdef QT_DEBUG
    qDebug() << "SERVER: sent HELLO: " << hello.GetSenderType();
#endif

    // who asked for connection. Receive HELLO message from client

    hello.Receive(socket,NetworkTimeout);
    if ( !hello.isPacketValid() ) {
        lastServerError = socket->error();
        emit ServerError(lastServerError);
        socket->disconnectFromHost();
        return;
    }
#ifdef QT_DEBUG
    qDebug() << "SERVER: received HELLO from client: " << hello.GetSenderType();
#endif

    QString senderVersion;
    QString senderType = hello.GetSenderType(&senderVersion);
    QString hello_msg;
    server_status_packet.SetStatus(SERVER_ERROR_OK,"OK");

    if ( senderType == NETPROTOCOL_SENDER_TYPE_CLIENT ) {
        hello_msg = "New NEWTON CLIENT connection from " + client_address.toString();
        emit HelloIsReceived(hello_msg);

        SEND_STATUS(socket);

        clientSocket = socket;

        packetHandler->SetSocket(clientSocket);

        connect(clientSocket,SIGNAL(disconnected()),this,SLOT(ClientDisconnected()));

        connect(clientSocket,SIGNAL(readyRead()),packetHandler,SLOT(ReadDataStream()));

    } else if ( senderType == NETPROTOCOL_SENDER_TYPE_GUI ) {
        guiSocket << socket;
        connect(socket,SIGNAL(disconnected()),this,SLOT(GUIDisconnected()));
        hello_msg = "New NEWTON SERVER GUI connection from " + client_address.toString();
        emit HelloIsReceived(hello_msg);

    } else { // unknown, just reject
        socket->disconnectFromHost();
        return;
    }

}



void Server::ClientDisconnected()
{
#ifdef QT_DEBUG
    qDebug() << "SERVER MSG: client from " << clientSocket->peerAddress() << " is disconnected!";
#endif

    clientSocket->disconnect(packetHandler,SLOT(ReadDataStream()));
    packetHandler->Reset();

    clientSocket = nullptr;

}


void Server::ExecuteCommand()
{
    NetPacket *clientPacket = packetHandler->GetPacket();

    if ( !clientPacket->isPacketValid() ) {
        return;
    }

    StatusNetPacket server_status_packet;
    bool ok;

    switch ( clientPacket->GetPacketID() ) {
    case NetPacket::PACKET_ID_INFO: {
        InfoNetPacket *pk = static_cast<InfoNetPacket*>(clientPacket);

#ifdef QT_DEBUG
        qDebug() << "SERVER: INFO-packet has been recieved [" << pk->GetInfo() << "]";
#endif

        server_status_packet.SetStatus(Server::SERVER_ERROR_OK,"OK");
        SEND_STATUS(clientSocket);

        packetHandler->SendPacket(&guiSocket,pk);

#ifdef QT_DEBUG
        qDebug() << "SERVER: send status [" << server_status_packet.GetByteView() << "] status = " << ok;
#endif
        break;
    }
    case NetPacket::PACKET_ID_CMD: {
        CmdNetPacket *pk = static_cast<CmdNetPacket*>(clientPacket);

#ifdef QT_DEBUG
        qDebug() << "SERVER: CMD-packet has been recieved [" << pk->GetCmdName() << "]";
#endif

        QString command_name = pk->GetCmdName();

        if ( command_name == NETPROTOCOL_COMMAND_STOP ) {

        } else if ( command_name == NETPROTOCOL_COMMAND_INIT ) {

        } else if ( command_name == NETPROTOCOL_COMMAND_BINNING ) {
            QVector<double> bin_vals;
            QVector<int> bin_vals_int;
            ok = pk->GetCmdArgs(&bin_vals);
            if ( !ok ) {
                server_status_packet.SetStatus(Server::SERVER_ERROR_INVALID_ARGS,"");
                SEND_STATUS(clientSocket);
            }
#ifdef QT_DEBUG
            DOUBLE_TO_INT(bin_vals,bin_vals_int);
            qDebug() << "SERVER: ARGS: " << bin_vals_int;
#endif
        } else if ( command_name == NETPROTOCOL_COMMAND_ROI ) {
            QVector<double> roi_vals;
            QVector<int> roi_vals_int;

            ok = pk->GetCmdArgs(&roi_vals);
            if ( !ok ) {
                server_status_packet.SetStatus(Server::SERVER_ERROR_INVALID_ARGS,"");
                SEND_STATUS(clientSocket);
            }
#ifdef QT_DEBUG
            DOUBLE_TO_INT(roi_vals,roi_vals_int);
            qDebug() << "SERVER: ARGS: " << roi_vals_int;
#endif
        } else if ( command_name == NETPROTOCOL_COMMAND_GAIN ) {

        } else if ( command_name == NETPROTOCOL_COMMAND_RATE ) {

        } else if ( command_name == NETPROTOCOL_COMMAND_SHUTTER ) {
            double flag;

            ok = pk->GetCmdArgs(&flag);
#ifdef QT_DEBUG
            qDebug() << "shutter state: " << flag;
#endif
            if ( !ok ) {
                server_status_packet.SetStatus(Server::SERVER_ERROR_INVALID_ARGS,"");
                SEND_STATUS(clientSocket);
            }

        } else if ( command_name == NETPROTOCOL_COMMAND_EXPTIME ) {

        } else if ( command_name == NETPROTOCOL_COMMAND_SETTEMP ) {

        } else if ( command_name == NETPROTOCOL_COMMAND_GETTEMP ) {

        } else if ( command_name == NETPROTOCOL_COMMAND_FITSFILE ) {

        } else if ( command_name == NETPROTOCOL_COMMAND_HEADFILE ) {

        } else {
            server_status_packet.SetStatus(Server::SERVER_ERROR_UNKNOWN_COMMAND,"UNKNOWN COMMAND");
            qDebug() << "[[[" << server_status_packet.GetByteView() << "]]]";
            SEND_STATUS(clientSocket);
            return;
        }

        server_status_packet.SetStatus(Server::SERVER_ERROR_OK,"OK");
#ifdef QT_DEBUG
        qDebug() << "SERVER: sending status [" << server_status_packet.GetByteView();
#endif
        SEND_STATUS(clientSocket);
#ifdef QT_DEBUG
        qDebug() << "net. status = " << ok;
#endif


        break;
    }
    case NetPacket::PACKET_ID_STATUS: {
        StatusNetPacket *pk = static_cast<StatusNetPacket*>(clientPacket);
        status_t err_no;
        QString err_str;

        err_no = pk->GetStatus(&err_str);

#ifdef QT_DEBUG
        qDebug() << "SERVER: STATUS-packet has been recieved [err = " << err_no << ", " << err_str << "]";
#endif

        break;
    }
    case NetPacket::PACKET_ID_HELLO: {
#ifdef QT_DEBUG
        qDebug() << "SERVER: HELLO-packet has been recieved [" << clientPacket->GetPacketContent() << "]";
#endif
        server_status_packet.SetStatus(Server::SERVER_ERROR_OK,"OK");
        SEND_STATUS(clientSocket);

#ifdef QT_DEBUG
        qDebug() << "SERVER: send status [" << server_status_packet.GetByteView() << "] status = " << ok;
#endif
        break;
    }
    default: break;
    }
}


void Server::GUIDisconnected() // remove socket of disconnected server GUI
{
    QObject* sender = QObject::sender();

    if ( sender != nullptr ) {
        int i = 0;
        foreach (QTcpSocket* sk, guiSocket) {
            if ( sk == sender ) {
                guiSocket.removeAt(i);
                return;
            }
            ++i;
        }
    }
}

                /*  Private members  */


