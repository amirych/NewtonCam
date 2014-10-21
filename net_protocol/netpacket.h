#ifndef NETPACKET_H
#define NETPACKET_H

#include "net_protocol_global.h"
#include "proto_defs.h"

#include<QString>
#include<QByteArray>
#include<QVector>
#include<QObject>
#include<QtNetwork/QTcpSocket>


//typedef unsigned int NetPacketID; // packet ID type
typedef long status_t; // type of returned status of command execution


        /*  Base class for network protocol realization */

class NET_PROTOCOLSHARED_EXPORT NetPacket
{
public:

    enum NetPacketID {PACKET_ID_INFO, PACKET_ID_CMD, PACKET_ID_TEMP,
                      PACKET_ID_STATUS, PACKET_ID_HELLO, PACKET_ID_UNKNOWN};
    enum NetPacketError {PACKET_ERROR_OK, PACKET_ERROR_NETWORK, PACKET_ERROR_NO_SOCKET,
                         PACKET_ERROR_UNKNOWN_PROTOCOL, PACKET_ERROR_CONTENT_LEN,
                         PACKET_ERROR_BAD_NUMERIC, PACKET_ERROR_WAIT, PACKET_ERROR_ID_MISMATCH};

    explicit NetPacket();
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

    NetPacket& Receive(QTcpSocket *socket, int timeout = NETPROTOCOL_TIMEOUT);

protected:
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
    explicit InfoNetPacket();
    InfoNetPacket(const QString &info);
    InfoNetPacket(const char* info);

    void SetInfo(const QString &info);
    void SetInfo(const char* info);

    QString GetInfo() const;

    InfoNetPacket& Receive(QTcpSocket *socket, int timeout = NETPROTOCOL_TIMEOUT);
};



class NET_PROTOCOLSHARED_EXPORT CmdNetPacket: public NetPacket
{
public:
    explicit CmdNetPacket();
    CmdNetPacket(const QString &cmdname, const QString &args);
    CmdNetPacket(const char* cmdname, const char* args);
    CmdNetPacket(const QString &cmdname, const char* args);
    CmdNetPacket(const char* cmdname, const QString &args);

    CmdNetPacket(const QString &cmdname, const double args);
    CmdNetPacket(const char* cmdname, const double args);

    CmdNetPacket(const QString &cmdname, const QVector<double> &args);
    CmdNetPacket(const char* cmdname, const QVector<double> &args);

    CmdNetPacket(const NetPacket &packet);

    CmdNetPacket& operator=(const NetPacket &packet);

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

    CmdNetPacket& Receive(QTcpSocket *socket, int timeout = NETPROTOCOL_TIMEOUT);

private:
    QString CmdName;
    QString Args;
    QVector<double> ArgsVec;

    void Init();
    void Init(const double arg);
    void Init(const QVector<double> &args);

    void ParseContent();
};



class NET_PROTOCOLSHARED_EXPORT StatusNetPacket: public NetPacket
{
public:
    explicit StatusNetPacket();
    StatusNetPacket(const status_t err_no);
    StatusNetPacket(const status_t err_no, const QString &err_str);
    StatusNetPacket(const status_t err_no, const char* err_str);

    StatusNetPacket(const NetPacket &packet);

    StatusNetPacket& operator=(const NetPacket &packet);

    void SetStatus(const status_t err_no, const QString &err_str = "");
    void SetStatus(const status_t err_no, const char* err_str = 0);

    status_t GetStatus(QString *err_str = nullptr) const;

    StatusNetPacket& Receive(QTcpSocket *socket, int timeout = NETPROTOCOL_TIMEOUT);

private:
    status_t Err_Code;
    QString Err_string;

    void Init();
    void ParseContent();
};


class NET_PROTOCOLSHARED_EXPORT TempNetPacket: public NetPacket
{
public:
    explicit TempNetPacket();
    TempNetPacket(const double temp, const unsigned int cooling_status);

    TempNetPacket(const NetPacket &packet);

    TempNetPacket& operator=(const NetPacket &packet);

    void SetTemp(const double temp, const unsigned int cooling_status);

    double GetTemp(unsigned int *cooling_status = nullptr) const;

    TempNetPacket& Receive(QTcpSocket *socket, int timeout = NETPROTOCOL_TIMEOUT);
private:
    double Temp; // temperature
    unsigned int CoollingStatus;

    void ParseContent();
};


class NET_PROTOCOLSHARED_EXPORT HelloNetPacket: public NetPacket
{
public:
    explicit HelloNetPacket();
    HelloNetPacket(const QString &sender_type, const QString &version);
    HelloNetPacket(const char* sender_type, const char* version);
    HelloNetPacket(const QString &sender_type, const char* version);
    HelloNetPacket(const char* sender_type, const QString &version);

    HelloNetPacket(const NetPacket &packet);
    HelloNetPacket& operator =(const NetPacket &packet);

    void SetSenderType(const QString &sender_type, const QString &version);
    void SetSenderType(const char* sender_type, const char* version);
    void SetSenderType(const QString &sender_type, const char* version);
    void SetSenderType(const char* sender_type, const QString &version);

    QString GetSenderType(QString *version = nullptr) const;

    HelloNetPacket& Receive(QTcpSocket *socket, int timeout);
private:
    QString SenderType;
    QString SenderVersion;

    void ParseContent();
};

#endif // NETPACKET_H
