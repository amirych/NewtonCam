#include "newtongui.h"
#include "../version.h"

#include <QDateTime>
#include <QString>
#include <QFont>

NewtonGui::NewtonGui(int fontsize): QWidget(), socket(nullptr)
{
    serverGUI = new ServerGUI(fontsize,this);
    serverGUI->show();

    QString str = QDateTime::currentDateTime().toString(" dd-MM-yyyy hh:mm:ss");

    serverGUI->LogMessage("<b> " + str + ":</b> Starting NewtonCam server GUI ...");
}


NewtonGui::~NewtonGui()
{
    if ( socket != nullptr ) socket->disconnectFromHost();

    qDebug() << "NewtonGUI is destroying!";
}


void NewtonGui::Connect(QHostAddress &server_addr, quint16 port)
{
    QString str = QDateTime::currentDateTime().toString(" dd-MM-yyyy hh:mm:ss");

    serverGUI->LogMessage("<b> " + str + ":</b> Trying to connect to NewtonCam server ...");


    socket = new QTcpSocket(this);

    // try to open connection
    socket->connectToHost(server_addr, port);

    if ( !socket->waitForConnected(NETPROTOCOL_TIMEOUT) ) {
        str = QDateTime::currentDateTime().toString(" dd-MM-yyyy hh:mm:ss");
        serverGUI->LogMessage("<b> " + str + ":</b> Can not connect to NewtonCam server!");
        serverGUI->NetworkError(socket->error());
//        QMessageBox::StandardButton bt =
        QMessageBox::critical(serverGUI,"Error","Can not connect to NewtonCam server!");
        emit Error(NEWTONGUI_ERROR_CANNOT_CONNECT);

//        serverGUI->TempChanged(-10.0003);

//        serverGUI->CoolerStatusChanged(20034);

        return;
    }

    str = QDateTime::currentDateTime().toString(" dd-MM-yyyy hh:mm:ss");

    serverGUI->LogMessage("<b> " + str + ":</b> Connection is established!");
    serverGUI->LogMessage("           ");


                /*  hand-sake with server  */

    HelloNetPacket hello;

    hello.Receive(socket,NETPROTOCOL_TIMEOUT);
    if ( !hello.isPacketValid() ) {
        socket->disconnectFromHost();
//        QMessageBox::StandardButton bt =
        QMessageBox::critical(serverGUI,"Error","Network error!");
        emit Error(NEWTONGUI_ERROR_NETWORK);
        return;
    }

    QString serverVersion;
    QString serverType = hello.GetSenderType(&serverVersion);

    if ( serverType != NETPROTOCOL_SENDER_TYPE_SERVER ) {
        socket->disconnectFromHost();
//        QMessageBox::StandardButton bt =
        QMessageBox::critical(serverGUI,"Error","It seems this is not NewtonCam server!");
        emit Error(NEWTONGUI_ERROR_INVALID_SERVER);
        return;
    }

    serverVersion.setNum(NEWTONCAM_PACKAGE_VERSION_MAJOR);
    serverType.setNum(NEWTONCAM_PACKAGE_VERSION_MINOR);

    serverVersion += ".";
    serverVersion += serverType;

    hello.SetSenderType(NETPROTOCOL_SENDER_TYPE_GUI,serverVersion);

    bool ok = hello.Send(socket);
    if ( !ok ) {
        socket->disconnectFromHost();
//        QMessageBox::StandardButton bt =
        QMessageBox::critical(serverGUI,"Error","Network error!");
        emit Error(NEWTONGUI_ERROR_NETWORK);
        return;
    }

    connect(socket,SIGNAL(disconnected()),this,SLOT(ServerDisconnected()));

    pk_handler = new NetPacketHandler(socket,this);

    connect(pk_handler,SIGNAL(PacketIsReceived()),this,SLOT(ServerMsgIsReceived()));

    connect(socket,SIGNAL(readyRead()),pk_handler,SLOT(ReadDataStream()));
}



                            /*  Private slots  */

void NewtonGui::ServerDisconnected()
{
    QString str = QDateTime::currentDateTime().toString(" dd-MM-yyyy hh:mm:ss");

    serverGUI->LogMessage("<b> " + str + ":</b> NewtonCam server disconnected!");
    serverGUI->Reset();
}


void NewtonGui::ServerMsgIsReceived()
{
    NetPacket* pk;

    pk = pk_handler->GetPacket();


    switch (pk->GetPacketID()) {
        case NetPacket::PACKET_ID_STATUS: {
            StatusNetPacket *st_pk = static_cast<StatusNetPacket*>(pk);
            QString cameraStatus;
            unsigned int status = st_pk->GetStatus(&cameraStatus);
            serverGUI->ServerError(status);
            serverGUI->ServerStatus(cameraStatus);
            break;
        }
        case NetPacket::PACKET_ID_INFO: {
            serverGUI->LogMessage(pk->GetPacketContent());
            break;
        }
        case NetPacket::PACKET_ID_TEMP: {
            TempNetPacket *t_pk = static_cast<TempNetPacket*>(pk);
            unsigned int cstatus;
            double temp = t_pk->GetTemp(&cstatus);
            serverGUI->TempChanged(temp);
            serverGUI->CoolerStatusChanged(cstatus);
            break;
        }
        default: // still just ignoring other types of packet
            break;
    }

    delete pk;
}
