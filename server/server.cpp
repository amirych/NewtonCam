#include "server.h"
#include "netpacket.h"

#include<QDebug>

                        /********************************
                        *                               *
                        *  Server class implementation  *
                        *                               *
                        ********************************/


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

void Server::ClientConnection()
{
    bool ok;

    QTcpSocket *socket = nextPendingConnection();

    if ( clientSocket != nullptr ) { // client already connected
        StatusNetPacket packet(SERVER_ERROR_BUSY,"Server is busy");

#ifdef QT_DEBUG
        qDebug() << "SERVER MSG: Other client is already connected!";
        qDebug() << "SERVER SENT: " << packet.GetByteView();
#endif

        ok = packet.Send(socket,NetworkTimeout);
        if ( !ok ) {
            lastServerError = socket->error();
            emit ServerError(lastServerError);
        }

        socket->disconnectFromHost();
        return;
    }

    QHostAddress client_address = socket->peerAddress();

#ifdef QT_DEBUG
    qDebug() << "SERVER: connection from " << client_address;
#endif

    if ( !allowed_hosts.contains(client_address) ) { // check for client is in allowed host list
        StatusNetPacket packet(SERVER_ERROR_DENIED,"Server refused the connection");
#ifdef QT_DEBUG
        qDebug() << "SERVER MSG: client address is not allowed!";
        qDebug() << "SERVER SENT: " << packet.GetByteView();
#endif

        ok = packet.Send(socket,NetworkTimeout);
        if ( !ok ) {
            lastServerError = socket->error();
            emit ServerError(lastServerError);
        }

        socket->disconnectFromHost();
        return;
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
        return;
    }
    std::cerr << "hello sending status: " << ok << std::endl;
#ifdef QT_DEBUG
    qDebug() << "SERVER: sent HELLO: " << hello.GetSenderType();
#endif

    // who asked for connection. Receive HELLO message from client

    hello.Receive(socket,NetworkTimeout);
    if ( !hello.isPacketValid() ) {
        lastServerError = socket->error();
        emit ServerError(lastServerError);
        return;
    }
#ifdef QT_DEBUG
    qDebug() << "SERVER: received HELLO from client: " << hello.GetSenderType();
#endif

    QString senderVersion;
    QString senderType = hello.GetSenderType(&senderVersion);
    QString hello_msg;
    StatusNetPacket server_ok(SERVER_ERROR_OK,"OK");

    if ( senderType == NETPROTOCOL_SENDER_TYPE_CLIENT ) {
        hello_msg = "New NEWTON CLIENT connection from " + client_address.toString();
        emit HelloIsReceived(hello_msg);

        ok = server_ok.Send(socket,NetworkTimeout);
        if ( !ok ) {
            socket->disconnectFromHost();
            lastServerError = socket->error();
            emit ServerError(lastServerError);
            return;
        }

        clientSocket = socket;

        packetHandler->SetSocket(clientSocket);

        connect(clientSocket,SIGNAL(disconnected()),this,SLOT(ClientDisconnected()));

        connect(clientSocket,SIGNAL(readyRead()),packetHandler,SLOT(ReadDataStream()));

    } else if ( senderType == NETPROTOCOL_SENDER_TYPE_GUI ) {
        guiSocket << socket;
        hello_msg = "New NEWTON SERVER GUI connection from " + client_address.toString();
        emit HelloIsReceived(hello_msg);

    } else { // unknown command, just reject
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

    StatusNetPacket status;
    bool ok;

    switch ( clientPacket->GetPacketID() ) {
    case NetPacket::PACKET_ID_INFO: {
        InfoNetPacket *pk = static_cast<InfoNetPacket*>(clientPacket);

#ifdef QT_DEBUG
        qDebug() << "SERVER: INFO-packet has been recieved [" << pk->GetInfo() << "]";
#endif

        status.SetStatus(Server::SERVER_ERROR_OK,"OK");
        ok = status.Send(clientSocket,NetworkTimeout);

#ifdef QT_DEBUG
        qDebug() << "SERVER: send status [" << status.GetByteView() << "] status = " << ok;
#endif
        break;
    }
    case NetPacket::PACKET_ID_CMD: {
        CmdNetPacket *pk = static_cast<CmdNetPacket*>(clientPacket);

#ifdef QT_DEBUG
        qDebug() << "SERVER: CMD-packet has been recieved [" << pk->GetCmdName();
#endif

        QString command_name = pk->GetCmdName();

        if ( command_name == NETPROTOCOL_COMMAND_STOP ) {

        } else if ( command_name == NETPROTOCOL_COMMAND_INIT ) {

        } else if ( command_name == NETPROTOCOL_COMMAND_BINNING ) {
            QVector<double> bin_vals;
            ok = pk->GetCmdArgs(&bin_vals);
            if ( !ok ) {
                status.SetStatus(Server::SERVER_ERROR_INVALID_ARGS,"");
            }
#ifdef QT_DEBUG
            qDebug() << "SERVER: COMMAND: [" << NETPROTOCOL_COMMAND_BINNING << "] ARGS: [" << bin_vals << "]";
#endif
        } else {
            status.SetStatus(Server::SERVER_ERROR_UNKNOWN_COMMAND,"");
        }

        status.SetStatus(Server::SERVER_ERROR_OK,"OK");
        ok = status.Send(clientSocket,NetworkTimeout);

#ifdef QT_DEBUG
        qDebug() << "SERVER: send status [" << status.GetByteView() << "] status = " << ok;
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
        status.SetStatus(Server::SERVER_ERROR_OK,"OK");
        ok = status.Send(clientSocket,NetworkTimeout);

#ifdef QT_DEBUG
        qDebug() << "SERVER: send status [" << status.GetByteView() << "] status = " << ok;
#endif
        break;
    }
    default: break;
    }
}


                /*  Private members  */


