#ifndef NETPACKET_H
#define NETPACKET_H

#include "net_protocol_global.h"

#include<QString>
#include<QByteArray>
#include<QVector>



typedef unsigned int netpacket_id_t; // packet ID type
typedef long status_t; // type of returned status of command execution


        /*  Base class for network protocol realization */

class NET_PROTOCOLSHARED_EXPORT NetPacket
{

public:
    NetPacket();
    NetPacket(const netpacket_id_t id);
    NetPacket(const netpacket_id_t id, const QString &content);
    NetPacket(const netpacket_id_t id, const char *content);

    void SetContent(const netpacket_id_t id, const QString &content);
    void SetContent(const netpacket_id_t id, const char* content);

    QByteArray GetByteView() const;

    netpacket_id_t GetPacketID() const;
    QString GetPacketContent() const;

    bool isPacketValid() const;

private:
    netpacket_id_t ID;
    QString Content;
    QByteArray Packet;

    bool ValidPacket;

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
