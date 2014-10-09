#ifndef NETPACKET_H
#define NETPACKET_H

#include "net_protocol_global.h"

#include<QString>
#include<QByteArray>
#include<QVector>


        /*  Base class for network protocol realization */

class NET_PROTOCOLSHARED_EXPORT NetPacket
{

public:
    NetPacket();
    NetPacket(const unsigned int id);
    NetPacket(const unsigned int id, const QString &content);
    NetPacket(const unsigned int id, const char *content);

    void SetContent(const unsigned int id, const QString &content);
    void SetContent(const unsigned int id, const char* content);

    QByteArray GetByteView() const;

    unsigned int GetPacketID() const;
    QString GetPacketContent() const;

    bool isPacketValid() const;

private:
    unsigned int ID;
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

#endif // NETPACKET_H
