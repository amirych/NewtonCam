#ifndef SERVER_H
#define SERVER_H

#include "server_global.h"
#include "netpacket.h"
#include "netpackethandler.h"
#include "../camera/camera.h"
//#include "camera.h"

#include<iostream>
#include<fstream>

#include<QList>
#include<QtNetwork/QTcpServer>
#include<QtNetwork/QTcpSocket>
#include <QtNetwork/QHostInfo>



//#define SERVER_DEFAULT_PORT 7777
#define SERVER_DEFAULT_ALLOWED_HOST "127.0.0.1"


            /*  Error codes from Server  */

/*

SERVER_ERROR_BUSY   - server already has connection from client
SERVER_ERROR_DENIED - server denied connection (client IP is not in list of allowed)

*/

//using namespace std;

class SERVERSHARED_EXPORT Server: public Camera
{
    Q_OBJECT
public:

    enum ServerErrorCode {SERVER_ERROR_OK, SERVER_ERROR_BUSY, SERVER_ERROR_DENIED,
                          SERVER_ERROR_CONNECTION, SERVER_ERROR_UNKNOWN_CLIENT,
                          SERVER_ERROR_UNKNOWN_COMMAND, SERVER_ERROR_INVALID_ARGS};

    explicit Server(QObject *parent = 0);
    Server(quint16 port, QObject *parent = 0);
    Server(QList<QHostAddress> &hosts, quint16 port = NETPROTOCOL_DEFAULT_PORT, QObject *parent = 0);
    Server(std::ostream &log_file, QList<QHostAddress> &hosts, quint16 port = NETPROTOCOL_DEFAULT_PORT, QObject *parent = 0);

    QAbstractSocket::SocketError getLastSocketError() const;

    ~Server();

    void SetNetworkTimeout(const int timeout);
signals:
    void ServerSocketError(QAbstractSocket::SocketError err_code);
    void HelloIsReceived(QString hello);
    void InfoIsReceived(QString info);
    void ServerLogMessage(QString msg);

protected slots:
    void ClientConnection();
    void ClientDisconnected();
    void ExecuteCommand();
    void GUIDisconnected();

protected:
    QTcpServer *net_server;
    QTcpSocket *clientSocket;
    QList<QTcpSocket*> guiSocket;

    quint16 serverPort;

    QList<QHostAddress> allowed_hosts;

    QAbstractSocket::SocketError lastSocketError;

    int NetworkTimeout;

    NetPacketHandler *packetHandler;

    QString serverVersionString;

    bool newClientConnection;
};

#endif // SERVER_H
