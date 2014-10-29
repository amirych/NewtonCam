#include "newtongui.h"
#include "../version.h"

#include <QDateTime>
#include <QString>


NewtonGui::NewtonGui(QObject *parent, QHostAddress &server_addr, quint16 port): QObject(parent)
{
    serverGUI = new ServerGUI;
    serverGUI->show();

    QString str = QDateTime::currentDateTime().toString(" dd-MM-yyyy hh:mm:ss");

    serverGUI->LogMessage("<b> " + str + ":</b> Starting NewtonCam server GUI ...");

    str = QDateTime::currentDateTime().toString(" dd-MM-yyyy hh:mm:ss");

    serverGUI->LogMessage("<b> " + str + ":</b> Trying to connect to NewtonCam server ...");


    socket = new QTcpSocket(this);

    // try to open connection
    socket->connectToHost(server_addr, port);

    if ( !socket->waitForConnected(NETPROTOCOL_TIMEOUT) ) {
        QMessageBox::StandardButton bt = QMessageBox::critical(serverGUI,"Error","Can not connect to NewtonCam server!");
        emit Error(1);
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
        QMessageBox::StandardButton bt = QMessageBox::critical(serverGUI,"Error","Network error!");
        emit Error(1);
        return;
    }

    QString serverVersion;
    QString serverType = hello.GetSenderType(&serverVersion);

    if ( serverType != NETPROTOCOL_SENDER_TYPE_SERVER ) {
        socket->disconnectFromHost();
        QMessageBox::StandardButton bt = QMessageBox::critical(serverGUI,"Error","It seems this is not NewtonCam server!");
        emit Error(1);
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
        QMessageBox::StandardButton bt = QMessageBox::critical(serverGUI,"Error","Network error!");
        emit Error(1);
        return;
    }

    connect(socket,SIGNAL(disconnected()),this,SLOT(ServerDisconnected()));

    pk_handler = new NetPacketHandler(socket,this);

    connect(pk_handler,SIGNAL(PacketIsReceived()),this,SLOT(ServerMsgIsReceived()));

    connect(socket,SIGNAL(readyRead()),pk_handler,SLOT(ReadDataStream()));
}


NewtonGui::~NewtonGui()
{
    socket->disconnectFromHost();

    delete serverGUI;
}




                            /*  Private slots  */

void NewtonGui::ServerDisconnected()
{
    QString str = QDateTime::currentDateTime().toString(" dd-MM-yyyy hh:mm:ss");

    serverGUI->LogMessage("<b> " + str + ":</b> NewtonCam server disconnected!");
}


void NewtonGui::ServerMsgIsReceived()
{
    NetPacket* pk;

    pk = pk_handler->GetPacket();

    serverGUI->LogMessage(pk->GetPacketContent());

    delete pk;
}
