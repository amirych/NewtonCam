#include "newtongui.h"
#include "../version.h"

#include <QDateTime>
#include <QString>
#include <QFont>
#include <QThread>

//NewtonGui::NewtonGui(int fontsize): QWidget(), socket(nullptr), serverGUI(nullptr)
NewtonGui::NewtonGui(int fontsize): ServerGUI(fontsize), socket(nullptr),
    pk_handler(nullptr), pk_handler_thread(nullptr)
{
//    serverGUI = new ServerGUI(fontsize,this);
//    serverGUI->show();

    QString str = QDateTime::currentDateTime().toString(" dd-MM-yyyy hh:mm:ss");

//    serverGUI->LogMessage("<b> " + str + ":</b> Starting NewtonCam server GUI ...");
    LogMessage("<b> " + str + ":</b> Starting NewtonCam server GUI ...");
}


NewtonGui::~NewtonGui()
{
    if ( pk_handler_thread != nullptr ) {
        pk_handler_thread->quit();
        if ( !pk_handler_thread->wait(3000) ) pk_handler_thread->terminate();
    }

    if ( socket != nullptr ) socket->disconnectFromHost();

#ifdef QT_DEBUG
    qDebug() << "NewtonGUI is destroyed!";
#endif
}


void NewtonGui::Connect(QHostAddress &server_addr, quint16 port)
{
    QString str = QDateTime::currentDateTime().toString(" dd-MM-yyyy hh:mm:ss");

//    serverGUI->LogMessage("<b> " + str + ":</b> Trying to connect to NewtonCam server at " + server_addr.toString() + " ...");
//    LogMessage("<b> " + str + ":</b> Trying to connect to NewtonCam server at " + server_addr.toString() + " ...");


    socket = new QTcpSocket(this);

    // try to open connection
    socket->connectToHost(server_addr, port);

    if ( !socket->waitForConnected(NETPROTOCOL_TIMEOUT) ) {
        str = QDateTime::currentDateTime().toString(" dd-MM-yyyy hh:mm:ss");
//        serverGUI->LogMessage("<b> " + str + ":</b> Can not connect to NewtonCam server!");
//        serverGUI->NetworkError(socket->error());
        LogMessage("<b> " + str + ":</b> Can not connect to NewtonCam server!");
        NetworkError(socket->error());
//        QMessageBox::StandardButton bt =
//        QMessageBox::critical(serverGUI,"Error","Can not connect to NewtonCam server!");
        QMessageBox::critical(this,"Error","Can not connect to NewtonCam server!");
        emit Error(NEWTONGUI_ERROR_CANNOT_CONNECT);
        return;
    }


                /*  hand-sake with server  */

    HelloNetPacket hello;
    QString serverVersion,serverType;

    // form version string for GUI-client
    serverVersion.setNum(NEWTONCAM_PACKAGE_VERSION_MAJOR);
    serverType.setNum(NEWTONCAM_PACKAGE_VERSION_MINOR);

    serverVersion += ".";
    serverVersion += serverType;

    hello.SetSenderType(NETPROTOCOL_SENDER_TYPE_GUI,serverVersion);

    bool ok = hello.Send(socket);
    if ( !ok ) {
        socket->disconnectFromHost();
//        QMessageBox::StandardButton bt =
//        QMessageBox::critical(serverGUI,"Error","Network error!");
        QMessageBox::critical(this,"Error","Network error!");
        emit Error(NEWTONGUI_ERROR_NETWORK);
        return;
    }

    // receive answer from server
    hello.Receive(socket,NETPROTOCOL_TIMEOUT);
    if ( !hello.isPacketValid() ) {
        socket->disconnectFromHost();
//        QMessageBox::StandardButton bt =
//        QMessageBox::critical(serverGUI,"Error","Network error!");
        QMessageBox::critical(this,"Error","Network error!");
        emit Error(NEWTONGUI_ERROR_NETWORK);
        return;
    }

    serverType = hello.GetSenderType(&serverVersion);

    if ( serverType != NETPROTOCOL_SENDER_TYPE_SERVER ) {
        str = QDateTime::currentDateTime().toString(" dd-MM-yyyy hh:mm:ss");
//        serverGUI->LogMessage("<b> " + str + ":</b> Could not find a NewtonCam server!");
        LogMessage("<b> " + str + ":</b> Could not find a NewtonCam server!");
        socket->disconnectFromHost();
//        QMessageBox::StandardButton bt =
//        QMessageBox::critical(serverGUI,"Error","Could not find a NewtonCam server!");
        QMessageBox::critical(this,"Error","Could not find a NewtonCam server!");
        emit Error(NEWTONGUI_ERROR_INVALID_SERVER);
        return;
    }


    str = QDateTime::currentDateTime().toString(" dd-MM-yyyy hh:mm:ss");

//    serverGUI->LogMessage("<b> " + str + ":</b> Found NewtonCam server of version " + serverVersion + " ...");
//    serverGUI->LogMessage("<b> " + str + ":</b> Connection is established!");
//    serverGUI->LogMessage("           ");
    LogMessage("<b> " + str + ":</b> Found NewtonCam server of version " + serverVersion + " ...");
    LogMessage("<b> " + str + ":</b> Connection is established!");
    LogMessage("           ");


    connect(socket,SIGNAL(disconnected()),this,SLOT(ServerDisconnected()));

    pk_handler = new NetPacketHandler(socket,this);

    pk_handler_thread = new QThread(this);
    pk_handler->moveToThread(pk_handler_thread);

    connect(pk_handler,SIGNAL(PacketIsReceived()),this,SLOT(ServerMsgIsReceived()));

    connect(socket,SIGNAL(readyRead()),pk_handler,SLOT(ReadDataStream()));

    pk_handler_thread->start();
}

//void NewtonGui::SetFonts(int fontsize, int status_fontsize, int log_fontsize)
//{
//    serverGUI->SetFonts(fontsize, status_fontsize, log_fontsize);
//    serverGUI->update();
//}


                            /*  Private slots  */

void NewtonGui::ServerDisconnected()
{
    QString str = QDateTime::currentDateTime().toString(" dd-MM-yyyy hh:mm:ss");

//    serverGUI->LogMessage("<b> " + str + ":</b> NewtonCam server disconnected!");
//    serverGUI->Reset();
    LogMessage("<b> " + str + ":</b> NewtonCam server disconnected!");
    Reset();
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
//            serverGUI->ServerError(status);
//            serverGUI->ServerStatus(cameraStatus);
            ServerError(status);
            ServerStatus(cameraStatus);
            break;
        }
        case NetPacket::PACKET_ID_INFO: {
//            serverGUI->LogMessage(pk->GetPacketContent());
            LogMessage(pk->GetPacketContent());
            break;
        }
        case NetPacket::PACKET_ID_TEMP: {
            TempNetPacket *t_pk = static_cast<TempNetPacket*>(pk);
            unsigned int cstatus;
            double temp = t_pk->GetTemp(&cstatus);
//            serverGUI->TempChanged(temp);
//            serverGUI->CoolerStatusChanged(cstatus);
            TempChanged(temp);
            CoolerStatusChanged(cstatus);
            break;
        }
        case NetPacket::PACKET_ID_GUI: {
            GuiNetPacket *gui_pk = static_cast<GuiNetPacket*>(pk);
            double temp, exp_clock;
            unsigned int cam_err, cool_status;
            QString cam_state;

            gui_pk->GetParams(&cam_err,&temp,&cool_status,&exp_clock,&cam_state);

//            serverGUI->TempChanged(temp);
//            serverGUI->CoolerStatusChanged(cool_status);
//            serverGUI->ServerError(cam_err);
//            serverGUI->ServerStatus(cam_state);
//            serverGUI->ExposureProgress(exp_clock);
            TempChanged(temp);
            CoolerStatusChanged(cool_status);
            ServerError(cam_err);
            ServerStatus(cam_state);
            ExposureProgress(exp_clock);
            break;
        }
        default: // still just ignoring other types of packet
            break;
    }

    delete pk;
}
