#ifndef NETPACKET_H
#define NETPACKET_H

#include "net_protocol_global.h"
#include "proto_defs.h"

#include<QString>
#include<QByteArray>
#include<QVector>
#include<QObject>
#include<QtNetwork/QTcpSocket>


typedef unsigned int NetPacketID; // packet ID type
typedef long status_t; // type of returned status of command execution


        /*  Base class for network protocol realization */

class NET_PROTOCOLSHARED_EXPORT NetPacket: public QObject
{

    Q_OBJECT
public:

    enum NetPacketID {PACKET_ID_INFO, PACKET_ID_CMD, PACKET_ID_TEMP, PACKET_ID_STATUS, PACKET_ID_HELLO};
    enum NetPacketError {PACKET_ERROR_OK, PACKET_ERROR_NETWORK, PACKET_ERROR_UNKNOWN_PROTOCOL, PACKET_ERROR_CONTENT_LEN,
                        PACKET_ERROR_BAD_NUMERIC, PACKET_ERROR_WAIT};

    NetPacket();
    NetPacket(const NetPacketID id);
    NetPacket(const NetPacketID id, const QString &content);
    NetPacket(const NetPacketID id, const char *content);

    void SetContent(const NetPacketID id, const QString &content);
    void SetContent(const NetPacketID id, const char* content);

    QByteArray GetByteView() const;

    NetPacketID GetPacketID() const;
    QString GetPacketContent() const;

    bool isPacketValid() const;
    NetPacketError GetPacketError() const;

    bool Send(QTcpSocket *socket, int timeout = NETPROTOCOL_TIMEOUT);

signals:
    void PacketReceived();

public slots:
    NetPacket* Receive(QTcpSocket *socket, int timeout = NETPROTOCOL_TIMEOUT);

private:
    NetPacketID ID;
    long Content_LEN;
    QString Content;
    QByteArray Packet;

    bool ValidPacket;
    NetPacketError packetError;

    void MakePacket();
};


        /*  Derived form base specialized classes  */

class NET_PROTOCOLSHARED_EXPORT InfoNetPacket: public NetPacket
{
public:
    InfoNetPacket();
    InfoNetPacket(const QString &info);
    InfoNetPacket(const char* info);

    void SetInfo(const QString &info);
    void SetInfo(const char* info);

    QString GetInfo() const;
};



class NET_PROTOCOLSHARED_EXPORT CmdNetPacket: public NetPacket
{
public:
    CmdNetPacket();
    CmdNetPacket(const QString &cmdname, const QString &args);
    CmdNetPacket(const char* cmdname, const char* args);
    CmdNetPacket(const QString &cmdname, const char* args);
    CmdNetPacket(const char* cmdname, const QString &args);

    CmdNetPacket(const QString &cmdname, const double args);
    CmdNetPacket(const char* cmdname, const double args);

    CmdNetPacket(const QString &cmdname, const QVector<double> &args);
    CmdNetPacket(const char* cmdname, const QVector<double> &args);

    void SetCommand(const QString &cmdname, const QString &args);
    void SetCommand(const char* cmdname, const char* args);
    void SetCommand(const QString &cmdname, const char* args);
    void SetCommand(const char* cmdname, const QString &args);

    void SetCommand(const QString &cmdname, const double args);
    void SetCommand(const char* cmdname, const double args);

    void SetCommand(const QString &cmdname, const QVector<double> &args);
    void SetCommand(const char* cmdname, const QVector<double> &args);

    QString GetCmdName() const;
    bool GetCmdArgs(QString *args);
    bool GetCmdArgs(double *args);
    bool GetCmdArgs(QVector<double> *args);

private:
    QString CmdName;
    QString Args;
    QVector<double> ArgsVec;

    void Init();
    void Init(const double arg);
    void Init(const QVector<double> &args);
};



class NET_PROTOCOLSHARED_EXPORT StatusNetPacket: public NetPacket
{
public:
    StatusNetPacket();
    StatusNetPacket(const status_t err_no);
    StatusNetPacket(const status_t err_no, const QString &err_str);
    StatusNetPacket(const status_t err_no, const char* err_str);

    void SetStatus(const status_t err_no, const QString &err_str = "");
    void SetStatus(const status_t err_no, const char* err_str = 0);

    status_t GetStatus(QString *err_str = 0) const;

private:
    status_t Err_Code;
    QString Err_string;

    void Init();
};

#endif // NETPACKET_H
