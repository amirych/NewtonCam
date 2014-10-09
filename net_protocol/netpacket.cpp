#include "netpacket.h"
#include "proto_defs.h"

#include<QTextStream>


                /************************************
                *                                   *
                *   NetPacket class implementation  *
                *                                   *
                ************************************/

                /*  Constructors  */

NetPacket::NetPacket(const unsigned int id, const QString &content):
    ID(id), Content(content)
{
    MakePacket();
}


NetPacket::NetPacket(const unsigned int id, const char* content):
    ID(id), Content(content)
{
    MakePacket();
}


NetPacket::NetPacket(): NetPacket(NETPROTOCOL_PACKET_ID_INFO,"")
{

}


NetPacket::NetPacket(const unsigned int id): NetPacket(id,"")
{

}


                /*  Private methods  */

void NetPacket::MakePacket()
{
    ValidPacket = true;

    // some checks
    if ( Content.length() > NETPROTOCOL_MAX_CONTENT_LEN ) {
        ValidPacket = false;
        return;
    }

    QString str;
    QTextStream pk(&str);

    pk << qSetFieldWidth(NETPROTOCOL_ID_FIELD_LEN) << right << ID
       << qSetFieldWidth(NETPROTOCOL_SIZE_FIELD_LEN) << right
          << Content.length() << Content;

    Packet = str.toUtf8();
}



                /*  Public methods  */

QByteArray NetPacket::GetByteView() const
{
    return Packet;
}


unsigned int NetPacket::GetPacketID() const
{
    return ID;
}


QString NetPacket::GetPacketContent() const
{
    return Content;
}


bool NetPacket::isPacketValid() const
{
    return ValidPacket;
}


void NetPacket::SetContent(const unsigned int id, const char *content)
{
    ID = id;
    Content = content;
    MakePacket();
}


void NetPacket::SetContent(const unsigned int id, const QString &content)
{
    ID = id;
    Content = content;
    MakePacket();
}



                /****************************************
                *                                       *
                *   InfoNetPacket class implementation  *
                *                                       *
                ****************************************/


InfoNetPacket::InfoNetPacket(const QString &info): NetPacket(NETPROTOCOL_PACKET_ID_INFO,info)
{

}


InfoNetPacket::InfoNetPacket(const char* info): NetPacket(NETPROTOCOL_PACKET_ID_INFO,info)
{

}


InfoNetPacket::InfoNetPacket(): NetPacket(NETPROTOCOL_PACKET_ID_INFO,"")
{

}

QString InfoNetPacket::GetInfo() const
{
    return GetPacketContent();
}


void InfoNetPacket::SetInfo(const char *info){
    SetContent(NETPROTOCOL_PACKET_ID_INFO, info);
}


void InfoNetPacket::SetInfo(const QString &info){
    SetContent(NETPROTOCOL_PACKET_ID_INFO, info);
}



                /***************************************
                *                                      *
                *   CmdNetPacket class implementation  *
                *                                      *
                ***************************************/


CmdNetPacket::CmdNetPacket(const char *cmdname, const char *args):
    NetPacket(NETPROTOCOL_PACKET_ID_CMD), CmdName(cmdname), Args(args), ArgsVec(QVector<double>())
{
    Init();
}


CmdNetPacket::CmdNetPacket(const QString &cmdname, const QString &args):
    NetPacket(NETPROTOCOL_PACKET_ID_CMD), CmdName(cmdname), Args(args), ArgsVec(QVector<double>())
{
    Init();
}


CmdNetPacket::CmdNetPacket(const char *cmdname, const QString &args):
    NetPacket(NETPROTOCOL_PACKET_ID_CMD), CmdName(cmdname), Args(args), ArgsVec(QVector<double>())
{
    Init();
}


CmdNetPacket::CmdNetPacket(const QString &cmdname, const char *args):
    NetPacket(NETPROTOCOL_PACKET_ID_CMD), CmdName(cmdname), Args(args), ArgsVec(QVector<double>())
{
    Init();
}


CmdNetPacket::CmdNetPacket(const char *cmdname, const double args):
    NetPacket(NETPROTOCOL_PACKET_ID_CMD), CmdName(cmdname), Args(""), ArgsVec(QVector<double>())
{
    Init(args);
}


CmdNetPacket::CmdNetPacket(const QString &cmdname, const double args):
    NetPacket(NETPROTOCOL_PACKET_ID_CMD), CmdName(cmdname), Args(""), ArgsVec(QVector<double>())
{
    Init(args);
}


CmdNetPacket::CmdNetPacket(const char *cmdname, const QVector<double> &args):
    NetPacket(NETPROTOCOL_PACKET_ID_CMD), CmdName(cmdname), Args(""), ArgsVec(args)
{
    Init(args);
}


CmdNetPacket::CmdNetPacket(const QString &cmdname, const QVector<double> &args):
    NetPacket(NETPROTOCOL_PACKET_ID_CMD), CmdName(cmdname), Args(""), ArgsVec(args)
{
    Init(args);
}


CmdNetPacket::CmdNetPacket(): CmdNetPacket("","")
{

}


void CmdNetPacket::Init()
{
    CmdName.simplified(); // remove whitespaces from start and end, and replace multiple whitespace by single one
//    CmdName.replace(' ', "_");
    CmdName.replace(NETPROTOCOL_CONTENT_DELIMETER , "_");

    SetContent(NETPROTOCOL_PACKET_ID_CMD,CmdName + NETPROTOCOL_CONTENT_DELIMETER + Args);
}


void CmdNetPacket::Init(const double arg)
{
    Args.setNum(arg,'g',12);
    ArgsVec.prepend(arg);
    Init();
}


void CmdNetPacket::Init(const QVector<double> &args)
{
    QString str;

    if ( args.isEmpty() ) return;

    Args.setNum(args[0],'g',12);

    for ( long i = 1; i < args.size(); ++i ) {
        str.setNum(args[i],'g',12);
        Args += NETPROTOCOL_CONTENT_DELIMETER + str;
    }

    Init();
}


void CmdNetPacket::SetCommand(const QString &cmdname, const QString &args)
{
    CmdName = cmdname;
    Args = args;
    ArgsVec.clear();

    Init();
}


void CmdNetPacket::SetCommand(const char *cmdname, const char *args)
{
    CmdName = cmdname;
    Args = args;
    ArgsVec.clear();

    Init();
}


void CmdNetPacket::SetCommand(const char *cmdname, const QString &args)
{
    CmdName = cmdname;
    Args = args;
    ArgsVec.clear();

    Init();
}


void CmdNetPacket::SetCommand(const QString &cmdname, const char *args)
{
    CmdName = cmdname;
    Args = args;
    ArgsVec.clear();

    Init();
}


void CmdNetPacket::SetCommand(const char *cmdname, const double args)
{
    CmdName = cmdname;
    Args = "";
    ArgsVec.clear();

    Init(args);
}


void CmdNetPacket::SetCommand(const QString &cmdname, const double args)
{
    CmdName = cmdname;
    Args = "";
    ArgsVec.clear();

    Init(args);
}


void CmdNetPacket::SetCommand(const char *cmdname, const QVector<double> &args)
{
    CmdName = cmdname;
    Args = "";
    ArgsVec = args;

    Init(args);
}


void CmdNetPacket::SetCommand(const QString &cmdname, const QVector<double> &args)
{
    CmdName = cmdname;
    Args = "";
    ArgsVec = args;

    Init(args);
}


QString CmdNetPacket::GetCmdName() const
{
    return CmdName;
}


bool CmdNetPacket::GetCmdArgs(QString *args)
{
    *args = Args;
    return Args.isEmpty() ? true : false;
}


bool CmdNetPacket::GetCmdArgs(double *args)
{
    bool ok;
    *args = Args.toDouble(&ok);

    return ok;
}


bool CmdNetPacket::GetCmdArgs(QVector<double> *args)
{
    *args = ArgsVec;

    if ( Args.isEmpty() ) {
        return false;
    }

    return true;
}
