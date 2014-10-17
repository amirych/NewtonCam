#include "server.h"
#include "netpacket.h"

#include<QDebug>

                        /********************************
                        *                               *
                        *  Server class implementation  *
                        *                               *
                        ********************************/


static QList<QHostAddress> empty_hosts_list;


                        /*  Constructors and destructor  */

Server::Server(QList<QHostAddress> &hosts, quint16 port, QObject *parent):
    QTcpServer(parent), serverPort(port), allowed_hosts(hosts),
    guiSocket(QList<QTcpSocket*>()), clientSocket(nullptr), clientPacket(nullptr)
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

    connect(this,SIGNAL(newConnection()),this,SLOT(ClientConnection()));
}


Server::Server(quint16 port, QObject *parent): Server(empty_hosts_list, port, parent)
{

}


Server::Server(QObject *parent): Server(empty_hosts_list, SERVER_DEFAULT_PORT, parent)
{
}


Server::~Server()
{
    delete clientPacket;
}



                    /*  Public members  */




                    /*  Private slots  */

void Server::ClientConnection()
{
    QTcpSocket *socket = nextPendingConnection();

    if ( clientSocket != nullptr ) { // client already connected
        StatusNetPacket packet(SERVER_ERROR_BUSY,"Server is busy");

#ifdef QT_DEBUG
        qDebug() << "SERVER MSG: Other client is already connected!";
        qDebug() << "SERVER SENT: " << packet.GetByteView();
#endif

//        socket->disconnectFromHost();
//        socket->deleteLater();
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

//        socket->deleteLater();
        return;
    }

    clientSocket = socket;

    connect(clientSocket,SIGNAL(disconnected()),this,SLOT(ClientDisconnected()));

    clientPacket = new NetPacket();

    connect(clientSocket,SIGNAL(readyRead()),this,SLOT(ReadClientStream()));
}


void Server::ReadClientStream()
{
#ifdef QT_DEBUG
    qDebug() << "SERVER: reading client packet ...";
#endif
    if ( clientPacket->Receive(clientSocket) != nullptr ) {
        ExecuteCommand();
    }
#ifdef QT_DEBUG
    qDebug() << "SERVER: client packet is read";
#endif
}


void Server::ClientDisconnected()
{
#ifdef QT_DEBUG
    qDebug() << "SERVER MSG: client from " << clientSocket->peerAddress() << " is disconnected!";
#endif

    clientSocket->disconnect(this,SLOT(ReadClientStream()));
//    clientSocket->deleteLater();
    clientSocket = nullptr;

    delete clientPacket;
    clientPacket = nullptr;
}


                    /*  Private members  */


void Server::ExecuteCommand()
{
    switch ( clientPacket->GetPacketID() ) {
    case NetPacket::PACKET_ID_INFO: {
        InfoNetPacket *pk = static_cast<InfoNetPacket*>(clientPacket);

#ifdef QT_DEBUG
        qDebug() << "SERVER: INFO-packet has been recieved [" << pk->GetInfo() << "]";
#endif

        break;
    }
    case NetPacket::PACKET_ID_CMD: {
        CmdNetPacket *pk = static_cast<CmdNetPacket*>(clientPacket);
        double args;

        bool ok = pk->GetCmdArgs(&args);

#ifdef QT_DEBUG
        qDebug() << "SERVER: CMD-packet has been recieved [" << pk->GetCmdName() << " -- "
                 << args << "] convert: " << ok;
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
        break;
    }
    default: break;
    }
}
