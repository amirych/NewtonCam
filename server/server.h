#ifndef SERVER_H
#define SERVER_H

#include "server_global.h"
#include "netpacket.h"

#include<iostream>
#include<fstream>

#include<QList>
#include<QtNetwork/QTcpServer>
#include<QtNetwork/QTcpSocket>



#define SERVER_DEFAULT_PORT 7777
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

    enum ServerErrorCode {SERVER_ERROR_OK, SERVER_ERROR_BUSY, SERVER_ERROR_DENIED};

    explicit Server(QObject *parent = 0);
    Server(quint16 port, QObject *parent = 0);
    Server(QList<QHostAddress> &hosts, quint16 port = SERVER_DEFAULT_PORT, QObject *parent = 0);

    ~Server();


signals:
    void ServerError(QAbstractSocket::SocketError err_code);

private slots:
    void ClientConnection();
    void ClientDisconnected();
    void ReadClientStream();


private:
    QTcpSocket *clientSocket;
    QList<QTcpSocket*> guiSocket;

    quint16 serverPort;

    QList<QHostAddress> allowed_hosts;

    QAbstractSocket::SocketError lastServerError;

    NetPacket *clientPacket;

    void ExecuteCommand();
};

#endif // SERVER_H
