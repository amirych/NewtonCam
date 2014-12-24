#include "server.h"
#include "netpacket.h"
#include "../version.h"

#include<QDebug>

#ifdef Q_OS_WIN
    #include "atmcd32d.h"
#endif

#ifdef Q_OS_LINUX
    #include "atmcdLXd.h"
#endif

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

Server::Server(std::ostream &log_file, QList<QHostAddress> &hosts, quint16 port, QObject *parent):
//    QTcpServer(parent), serverPort(port), allowed_hosts(hosts),
    Camera(log_file,0,parent), net_server(nullptr),
    clientSocket(nullptr), guiSocket(QList<QTcpSocket*>()),
    serverPort(port), allowed_hosts(hosts),
    lastSocketError(QAbstractSocket::UnknownSocketError),
    NetworkTimeout(NETPROTOCOL_TIMEOUT), packetHandler(nullptr),
    serverVersionString(QString("")), newClientConnection(true)
{
#ifdef QT_DEBUG
    qDebug() << "Start server";
#endif
    if ( allowed_hosts.isEmpty() ) { // set default host
        QString host = SERVER_DEFAULT_ALLOWED_HOST;
        allowed_hosts.append(QHostAddress(host));
    }


    // start listenning
    net_server = new QTcpServer(this);

    LogOutput("   [SERVER] Start server ...");

    if ( net_server == nullptr ) {
        LogOutput("   [SERVER] Cannot start server! Cannot create QTcpServer object!!!");
        lastError = CAMERA_ERROR_BADALLOC;
        emit CameraError(lastError);
        return;
    }

    if ( !net_server->listen(QHostAddress::Any,serverPort) ) {
#ifdef QT_DEBUG
        qDebug() << "CAN NOT START TO LISTEN!!!";
#endif        
        lastSocketError = net_server->serverError();        
        emit ServerSocketError(lastSocketError);
        LogOutput("   [SERVER] Cannot start listening! Socket error: " + net_server->errorString());
        return;
    }

//    packetHandler = new NetPacketHandler(this);
    packetHandler = new NetPacketHandler;
    packetHandler->moveToThread(&packetHandlerThread);

    connect(net_server,SIGNAL(newConnection()),this,SLOT(ClientConnection()));

    connect(packetHandler,SIGNAL(PacketIsReceived()),this,SLOT(ExecuteCommand()));

    connect(this,SIGNAL(UpdateRemoteGui(NetPacket*)),packetHandler,SLOT(SendPacket(NetPacket*)));

    packetHandlerThread.start();

    connect(this,SIGNAL(ExposureClock(double)),this,SLOT(SendServerState()));
    connect(this,SIGNAL(TemperatureChanged(double)),this,SLOT(SendServerState()));
    connect(this,SIGNAL(CoolerStatusChanged(uint)),this,SLOT(SendServerState()));
    connect(this,SIGNAL(CameraError(uint)),this,SLOT(SendServerState()));
    connect(this,SIGNAL(CameraStatus(QString)),this,SLOT(SendServerState()));

    connect(this,SIGNAL(InfoIsReceived(QString)),this,SLOT(SendServerLog(QString)));

    serverVersionString.setNum(NEWTONCAM_PACKAGE_VERSION_MAJOR);
    QString str;
    str.setNum(NEWTONCAM_PACKAGE_VERSION_MINOR);
    serverVersionString += "." + str;
}


Server::Server(QList<QHostAddress> &hosts, quint16 port, QObject *parent):
    Server(std::cerr,hosts,port,parent)
{

}

Server::Server(quint16 port, QObject *parent): Server(std::cerr, default_hosts_list, port, parent)
{

}


Server::Server(QObject *parent): Server(std::cerr, default_hosts_list, NETPROTOCOL_DEFAULT_PORT, parent)
{
}


Server::~Server()
{
//    if ( !guiSocket.empty() ) {
//        foreach (QTcpSocket *s, guiSocket) if ( s != nullptr ) s->disconnectFromHost();
//    }


    packetHandlerThread.quit();
    bool ok = packetHandlerThread.wait(NetworkTimeout);
    if ( !ok ) packetHandlerThread.terminate();

    delete packetHandler;

    if ( clientSocket != nullptr) {
        clientSocket->disconnectFromHost();
    }

    LogOutput("   [SERVER] Stop server.");
#ifdef QT_DEBUG
    qDebug() << "Stop server!";
#endif
}



                    /*  Public members  */

void Server::SetNetworkTimeout(const int timeout)
{
    NetworkTimeout = timeout;
}


QAbstractSocket::SocketError Server::getLastSocketError() const
{
    return lastSocketError;
}

                    /*  Private slots  */

// macro for sending server status
#define SEND_STATUS(sk) { \
    ok = server_status_packet.Send(sk,NetworkTimeout); \
    if ( !ok ) { \
        qDebug() << "ERROR OF SERVER STATUS SENDING! " << "Packet error: " << server_status_packet.GetPacketError() << "; err = " << sk->errorString(); \
        lastSocketError = sk->error(); \
        emit ServerSocketError(lastSocketError); \
        sk->disconnectFromHost(); \
        return; \
    } \
}
void Server::ClientConnection()
{
    bool ok;
    StatusNetPacket server_status_packet;

    QTcpSocket *socket = net_server->nextPendingConnection();
    QHostAddress client_address = socket->peerAddress();

    QString log_str = "   [SERVER] New connection from ";
    log_str += client_address.toString();
    log_str += " ... Trying handshaking ...";

    LogOutput(log_str);

    // who asked for connection. Receive HELLO message from client
    HelloNetPacket hello;

    LogOutput("   [SERVER] Receiving HELLO message ... ", true, false);
    hello.Receive(socket,NetworkTimeout);
    if ( !hello.isPacketValid() ) {
        LogOutput("Failed",false);
        lastSocketError = socket->error();
        emit ServerSocketError(lastSocketError);
        socket->disconnectFromHost();
        return;
    }
    LogOutput("OK",false);


#ifdef QT_DEBUG
    qDebug() << "[SERVER] received HELLO from client: " << hello.GetSenderType();
#endif

    QString senderVersion;
    QString senderType = hello.GetSenderType(&senderVersion);

    LogOutput("   [SERVER] Connection is from " + senderType);

    // send server HELLO message

    LogOutput("   [SERVER] Send HELLO message ... ", true, false);

    hello.SetSenderType(NETPROTOCOL_SENDER_TYPE_SERVER,serverVersionString);
    ok = hello.Send(socket,NetworkTimeout);
    if ( !ok ) {
        LogOutput("Failed",false);
        lastSocketError = socket->error();
        emit ServerSocketError(lastSocketError);
        socket->disconnectFromHost();
        return;
    }
    LogOutput("OK",false);

#ifdef QT_DEBUG
    qDebug() << "[SERVER] sent HELLO: " << hello.GetSenderType();
#endif


    QString hello_msg;
    server_status_packet.SetStatus(SERVER_ERROR_OK,"OK");


    if ( senderType == NETPROTOCOL_SENDER_TYPE_CLIENT ) {
//        if ( clientSocket != nullptr ) { // client already connected
        if ( !newClientConnection ) { // client already connected
            LogOutput("   [SERVER] Command client is already connected! Reject the connection!");
            server_status_packet.SetStatus(SERVER_ERROR_BUSY,"Server is busy");

#ifdef QT_DEBUG
            qDebug() << "SERVER MSG: Other client is already connected!";
            qDebug() << "SERVER SENT: " << server_status_packet.GetByteView();
#endif

            SEND_STATUS(socket);
            socket->disconnectFromHost();
            return;
        }

        // check for client address is in allowed list

        LogOutput("   [SERVER] Looking client address in allowed list ... ", true, false);

        if ( !allowed_hosts.contains(client_address) ) { // check for client is in allowed host list
            LogOutput("NO! Reject the connection!",false);

            server_status_packet.SetStatus(SERVER_ERROR_DENIED,"Server refused the connection");

#ifdef QT_DEBUG
            qDebug() << "SERVER MSG: client address is not allowed!";
            qDebug() << "SERVER SENT: " << server_status_packet.GetByteView();
#endif

            SEND_STATUS(socket);
            socket->disconnectFromHost();
            return;
        }

        LogOutput("OK",false);

        hello_msg = "<b>";
        hello_msg += TIME_STAMP;
        hello_msg += "</b>";
        hello_msg += "New NEWTON CLIENT connection from " + client_address.toString();
        hello_msg += " (version: " + senderVersion + ")";
        emit HelloIsReceived(hello_msg);

        SEND_STATUS(socket); // send OK-status, i.e., server is ready for command acception

        newClientConnection = false;
        clientSocket = socket;

        InfoNetPacket info;
        info.SetInfo(hello_msg);

        packetHandler->SendPacket(&info);

        packetHandler->SetSocket(clientSocket);

        connect(clientSocket,SIGNAL(disconnected()),this,SLOT(ClientDisconnected()));

        connect(clientSocket,SIGNAL(readyRead()),packetHandler,SLOT(ReadDataStream()));

    } else if ( senderType == NETPROTOCOL_SENDER_TYPE_GUI ) {
        guiSocket << socket;
//        connect(socket,SIGNAL(disconnected()),this,SLOT(GUIDisconnected()));
        packetHandler->AddSocket(socket);

        hello_msg = "<b>";
        hello_msg += TIME_STAMP;
        hello_msg += "</b>";
        hello_msg += "New NEWTON SERVER GUI connection from " + client_address.toString();
        emit HelloIsReceived(hello_msg);

        // send server current state
        SendServerState();
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

//    clientSocket = nullptr;
    newClientConnection = true;

    emit CameraError(lastError); // emit signal with the last returned error code
}


void Server::ExecuteCommand()
{
    NetPacket *clientPacket = packetHandler->GetPacket();

    if ( !clientPacket->isPacketValid() ) {
        return;
    }

    StatusNetPacket server_status_packet;
//    unsigned int last_oper_status;
    bool ok;

    switch ( clientPacket->GetPacketID() ) {
    case NetPacket::PACKET_ID_INFO: {
        InfoNetPacket *pk = static_cast<InfoNetPacket*>(clientPacket);
        QString str = "<b>";
        str += TIME_STAMP;
        str += "</b> " + pk->GetInfo();
        emit InfoIsReceived(str);
//        packetHandler->SendPacket(pk);

#ifdef QT_DEBUG
        qDebug() << "SERVER: INFO-packet has been recieved [" << pk->GetInfo() << "]";
#endif

        server_status_packet.SetStatus(Server::SERVER_ERROR_OK,"OK");
        SEND_STATUS(clientSocket);

//        packetHandler->SendPacket(&guiSocket,pk);

#ifdef QT_DEBUG
        qDebug() << "SERVER: send status [" << server_status_packet.GetByteView() << "] status = " << ok;
#endif
        break;
    }
    case NetPacket::PACKET_ID_CMD: {
        CmdNetPacket *pk = static_cast<CmdNetPacket*>(clientPacket);

#ifdef QT_DEBUG
        qDebug() << "SERVER: CMD-packet has been recieved [" << pk->GetCmdName() << "]";
        qDebug() << "SERVER: CMD-packet: " << pk->GetPacketContent();
#endif

        QString command_name = pk->GetCmdName();

        // execute the command
        QString time_stamp;

        if ( command_name == NETPROTOCOL_COMMAND_STOP ) {
            StopExposure();
            server_status_packet.SetStatus(lastError,"");
        } else if ( command_name == NETPROTOCOL_COMMAND_INIT ) { // re-init camera
            InitCamera();
            time_stamp = "<b>";
            time_stamp += TIME_STAMP;
            time_stamp += "</b>";
            if ( lastError != DRV_SUCCESS ) {
                emit InfoIsReceived(time_stamp + "<font color='red'> Initialization process failed!</font>");
            } else {
                emit InfoIsReceived(time_stamp + " Initialization has been completed!");
            }
            server_status_packet.SetStatus(lastError,"");
//        } else if ( command_name == NETPROTOCOL_COMMAND_BINNING ) {
//            QVector<double> bin_vals;
//            QVector<int> bin_vals_int;
//            ok = pk->GetCmdArgs(&bin_vals);
//            if ( !ok ) {
//                server_status_packet.SetStatus(Server::SERVER_ERROR_INVALID_ARGS,"");
//            } else {
//                DOUBLE_TO_INT(bin_vals,bin_vals_int);
//                server_status_packet.SetStatus(lastError,"");
//            }
//        } else if ( command_name == NETPROTOCOL_COMMAND_ROI ) {
//            QVector<double> roi_vals;
//            QVector<int> roi_vals_int;

//            ok = pk->GetCmdArgs(&roi_vals);
//            if ( !ok ) {
//                server_status_packet.SetStatus(Server::SERVER_ERROR_INVALID_ARGS,"");
//            } else {
//                DOUBLE_TO_INT(roi_vals,roi_vals_int);
//                server_status_packet.SetStatus(lastError,"");
//            }
        } else if ( command_name == NETPROTOCOL_COMMAND_FRAME ) {
            QVector<double> frame_vals;
            QVector<int> frame_vals_int;

            ok = pk->GetCmdArgs(&frame_vals);
            if ( !ok ) {
                server_status_packet.SetStatus(Server::SERVER_ERROR_INVALID_ARGS,"");
            } else {
                DOUBLE_TO_INT(frame_vals,frame_vals_int);
                SetFrame(frame_vals_int[0],frame_vals_int[1],frame_vals_int[2],frame_vals_int[3],frame_vals_int[4],frame_vals_int[5]);
                server_status_packet.SetStatus(lastError,"");
#ifdef QT_DEBUG
            qDebug() << "SERVER: ARGS: " << frame_vals_int;
#endif
            }
        } else if ( command_name == NETPROTOCOL_COMMAND_GAIN ) {
            QString gain_str;
            ok = pk->GetCmdArgs(&gain_str);
            SetCCDGain(gain_str);
            server_status_packet.SetStatus(lastError,"");
        } else if ( command_name == NETPROTOCOL_COMMAND_COOLER ) {
            QString cooler_state;
            ok = pk->GetCmdArgs(&cooler_state);
            cooler_state = cooler_state.toUpper().trimmed();
#ifdef QT_DEBUG
            qDebug() << "[SERVER]: Cooler is " << cooler_state << "; ok = " << ok;
#endif
            if ( !ok || cooler_state.isEmpty() ) {
                server_status_packet.SetStatus(Server::SERVER_ERROR_INVALID_ARGS,"");
            } else {
                if ( cooler_state == "ON" ) {
                    SetCoolerON();
                    server_status_packet.SetStatus(lastError,"");
                } else if ( cooler_state == "OFF" ) {
                    SetCoolerOFF();
                    server_status_packet.SetStatus(lastError,"");
                } else {
                    server_status_packet.SetStatus(Server::SERVER_ERROR_INVALID_ARGS,"");
                }
            }
        } else if ( command_name == NETPROTOCOL_COMMAND_FAN ) {
            QString fan_state;
            ok = pk->GetCmdArgs(&fan_state);
            if ( !ok ) {
                server_status_packet.SetStatus(Server::SERVER_ERROR_INVALID_ARGS,"");
            } else {
                SetFanState(fan_state);
                server_status_packet.SetStatus(lastError,"");
            }
        } else if ( command_name == NETPROTOCOL_COMMAND_RATE ) {
            QString rate_str;
            ok = pk->GetCmdArgs(&rate_str);
            SetRate(rate_str);
            server_status_packet.SetStatus(lastError,"");
        } else if ( command_name == NETPROTOCOL_COMMAND_SHUTTER ) {
            double flag;

            ok = pk->GetCmdArgs(&flag);
#ifdef QT_DEBUG
            qDebug() << "shutter state: " << flag;
#endif
            if ( !ok ) {
                server_status_packet.SetStatus(Server::SERVER_ERROR_INVALID_ARGS,"");
            } else {
                server_status_packet.SetStatus(lastError,"");
            }
        } else if ( command_name == NETPROTOCOL_COMMAND_EXPTIME ) {
            ok = pk->GetCmdArgs(&currentExposureClock);
            if ( !ok ) {
                server_status_packet.SetStatus(Server::SERVER_ERROR_INVALID_ARGS,"");
            } else {
                SetExpTime(currentExposureClock);
                server_status_packet.SetStatus(lastError,"");
            }
        } else if ( command_name == NETPROTOCOL_COMMAND_SETTEMP ) {
            double temp;
            int temp_int;
            ok = pk->GetCmdArgs(&temp);
            if ( !ok ) {
                server_status_packet.SetStatus(Server::SERVER_ERROR_INVALID_ARGS,"");
            } else {
                temp_int = static_cast<int>(temp);
                SetCCDTemperature(temp_int);
                server_status_packet.SetStatus(lastError,"");
            }
        } else if ( command_name == NETPROTOCOL_COMMAND_GETTEMP ) {
            double temp;
            unsigned int cool_stat;
            GetCCDTemperature(&temp,&cool_stat);
            server_status_packet.SetStatus(lastError,"");

            if ( lastError != DRV_SUCCESS ) { // if temperature asking function failed send fake values
                temp = 0.0;
                cool_stat = DRV_ERROR_ACK;
            }

            TempNetPacket pk;
            pk.SetTemp(temp,cool_stat);
            ok = pk.Send(clientSocket);
            if ( !ok ) {
                lastSocketError = clientSocket->error();
                emit ServerSocketError(lastSocketError);
                clientSocket->disconnectFromHost();
                return;
            }
        } else if ( command_name == NETPROTOCOL_COMMAND_FITSFILE ) {
            ok = pk->GetCmdArgs(&currentFITS_Filename);
            currentFITS_Filename = currentFITS_Filename.trimmed();
            if ( !ok || currentFITS_Filename.isEmpty() ) { // an empty filename
                server_status_packet.SetStatus(Server::SERVER_ERROR_INVALID_ARGS,"");
            } else {
                server_status_packet.SetStatus(Server::SERVER_ERROR_OK,"OK");
            }
        } else if ( command_name == NETPROTOCOL_COMMAND_HEADFILE ) {
            ok = pk->GetCmdArgs(&currentHDR_Filename);
            currentHDR_Filename = currentHDR_Filename.trimmed();
            if ( !ok || currentHDR_Filename.isEmpty() ) { // an empty filename
                server_status_packet.SetStatus(Server::SERVER_ERROR_INVALID_ARGS,"");
            } else {
                server_status_packet.SetStatus(Server::SERVER_ERROR_OK,"OK");
            }
        } else if ( command_name == NETPROTOCOL_COMMAND_START ) {
            StartExposure(currentFITS_Filename,currentHDR_Filename);
            server_status_packet.SetStatus(lastError,"");
        } else {
            server_status_packet.SetStatus(Server::SERVER_ERROR_UNKNOWN_COMMAND,"UNKNOWN COMMAND");
            qDebug() << "[[[" << server_status_packet.GetByteView() << "]]]";
        }

        // command was executed, send its exit status

//        server_status_packet.SetStatus(Server::SERVER_ERROR_OK,"OK");

        // DRV_SUCCESS means OK, so send error code Server::SERVER_ERROR_OK to client
        if ( server_status_packet.GetStatus() == DRV_SUCCESS ) {
            server_status_packet.SetStatus(Server::SERVER_ERROR_OK,"OK");
        }


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

    delete clientPacket;
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


bool Server::isListening() const
{
    if ( net_server != nullptr ) {
        return net_server->isListening();
    } else {
        return false;
    }
}

                /*  Protected methods  */

void Server::SendServerState()
{
//    double temp;
//    unsigned int cool_status;

//    GetCCDTemperature(&temp,&cool_status);

//    currentStatePacket.SetParams(lastError,temp,cool_status,currentExposureClock,cameraStatus);
    currentStatePacket.SetParams(lastError,currentTemperature,currentCoolerStatus,currentExposureClock,cameraStatus);

    emit UpdateRemoteGui(&currentStatePacket);
}


void Server::SendServerLog(QString log_str)
{
    InfoNetPacket pk;
    pk.SetInfo(log_str);
    packetHandler->SendPacket(&pk);
}
