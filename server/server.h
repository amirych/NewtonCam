#ifndef SERVER_H
#define SERVER_H

#include "server_global.h"
#include "netpacket.h"
#include "netpackethandler.h"

#include<iostream>
#include<fstream>

#include<QList>
#include<QtNetwork/QTcpServer>
#include<QtNetwork/QTcpSocket>



//#define SERVER_DEFAULT_PORT 7777
#define SERVER_DEFAULT_ALLOWED_HOST "127.0.0.1"


            /*  Error codes from Server  */

/*

SERVER_ERROR_BUSY   - server already has connection from client
SERVER_ERROR_DENIED - server denied connection (client IP is not in list of allowed)

*/

//using namespace std;

class SERVERSHARED_EXPORT Server: public QTcpServer
{
    Q_OBJECT
public:

    enum ServerErrorCode {SERVER_ERROR_OK, SERVER_ERROR_BUSY, SERVER_ERROR_DENIED,
                          SERVER_ERROR_CONNECTION, SERVER_ERROR_UNKNOWN_CLIENT,
                          SERVER_ERROR_UNKNOWN_COMMAND, SERVER_ERROR_INVALID_ARGS};

    explicit Server(QObject *parent = 0);
    Server(quint16 port, QObject *parent = 0);
    Server(QList<QHostAddress> &hosts, quint16 port = NETPROTOCOL_DEFAULT_PORT, QObject *parent = 0);

    ~Server();

    void SetNetworkTimeout(const int timeout);
signals:
    void ServerError(QAbstractSocket::SocketError err_code);
    void HelloIsReceived(QString hello);
    void InfoIsReceived(QString info);

private slots:
    void ClientConnection();
    void ClientDisconnected();
    void ExecuteCommand();


private:
    QTcpSocket *clientSocket;
    QList<QTcpSocket*> guiSocket;

    int NetworkTimeout;

    quint16 serverPort;

    QList<QHostAddress> allowed_hosts;

    QAbstractSocket::SocketError lastServerError;

    NetPacketHandler *packetHandler;
};

#endif // SERVER_H
