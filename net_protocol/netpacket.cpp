#include "netpacket.h"
#include "proto_defs.h"

#include<QTextStream>


                /************************************
                *                                   *
                *   NetPacket class implementation  *
                *                                   *
                ************************************/


static void split_content(QString &content, QString &left, QString &right)
{
    int idx = content.indexOf(NETPROTOCOL_CONTENT_DELIMETER, Qt::CaseInsensitive);

    if ( idx == -1 ) {
        left = content;
        right = "";
    } else {
        left = content.mid(0,idx);
        right = content.mid(idx);
    }
}


                /*  Constructors  */

NetPacket::NetPacket(const NetPacketID id, const QString &content):
    ID(id), Content(content)
{
    MakePacket();
}


NetPacket::NetPacket(const NetPacketID id, const char* content):
    ID(id), Content(content)
{
    MakePacket();
}


NetPacket::NetPacket(): NetPacket(PACKET_ID_INFO,"")
{

}


NetPacket::NetPacket(const NetPacketID id): NetPacket(id,"")
{

}


                /*  Private methods  */

void NetPacket::MakePacket()
{
    ValidPacket = true;

    // some checks
    if ( Content.length() > NETPROTOCOL_MAX_CONTENT_LEN ) {
        ValidPacket = false;
        packetError = PACKET_ERROR_CONTENT_LEN;
        return;
    }

    QString str;
    QTextStream pk(&str);

    pk << qSetFieldWidth(NETPROTOCOL_ID_FIELD_LEN) << right << ID
       << qSetFieldWidth(NETPROTOCOL_SIZE_FIELD_LEN) << right
          << Content.length() << Content;

    Content_LEN = Content.length();

    Packet = str.toUtf8();

    packetError = PACKET_ERROR_OK;
}



                /*  Public methods  */

QByteArray NetPacket::GetByteView() const
{
    return Packet;
}


NetPacket::NetPacketID NetPacket::GetPacketID() const
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


NetPacket::NetPacketError NetPacket::GetPacketError() const
{
    return packetError;
}

void NetPacket::SetContent(const NetPacketID id, const char *content)
{
    ID = id;
    Content = content;
    MakePacket();
}


void NetPacket::SetContent(const NetPacketID id, const QString &content)
{
    ID = id;
    Content = content;
    MakePacket();
}


bool NetPacket::Send(QTcpSocket *socket, int timeout) // WARNING: THIS CALL BLOCKS CURRENT THREAD!!!
{
    qint64 n = socket->write(Packet);

    if ( n == -1 ) return false;

    return socket->waitForBytesWritten(timeout);
}


NetPacket* NetPacket::Receive(QTcpSocket *socket, int timeout)
{
    bool ok;

    if ( ValidPacket ) {
        if ( socket->bytesAvailable() < (NETPROTOCOL_ID_FIELD_LEN + NETPROTOCOL_SIZE_FIELD_LEN) ) {
            packetError = PACKET_ERROR_WAIT;
            return nullptr;
        }
        ValidPacket = false;

        Packet = socket->read(NETPROTOCOL_ID_FIELD_LEN + NETPROTOCOL_SIZE_FIELD_LEN);
        if ( Packet.size() < (NETPROTOCOL_ID_FIELD_LEN + NETPROTOCOL_SIZE_FIELD_LEN) ) { // IT IS AN ERROR!!!
            packetError = PACKET_ERROR_NETWORK;
            return nullptr;
        }

        long id_num = Packet.left(NETPROTOCOL_ID_FIELD_LEN).toLong(&ok);
        if (!ok) { // IT IS AN ERROR!!!
            packetError = PACKET_ERROR_BAD_NUMERIC;
            return nullptr;
        }

        ID = static_cast<NetPacket::NetPacketID>(id_num);

        Content_LEN = Packet.right(NETPROTOCOL_SIZE_FIELD_LEN).toLong(&ok);
        if (!ok) { // IT IS AN ERROR!!!
            packetError = PACKET_ERROR_BAD_NUMERIC;
            return nullptr;
        }

        if ( Content_LEN < 0 ) { // IT IS AN ERROR!!!
            packetError = PACKET_ERROR_BAD_NUMERIC;
            return nullptr;
        }

        if ( socket->bytesAvailable() < Content_LEN ) return nullptr; // waiting for entire packet content

    }

    // at least ID, LEN and part of CONTENT fields are already should be read

    if ( socket->bytesAvailable() < Content_LEN ) { // waiting for entire packet content
        packetError = PACKET_ERROR_WAIT;
        return nullptr;
    }
    Packet = socket->read(Content_LEN);

    if ( Packet.size() < Content_LEN ) { // IT IS AN ERROR!!!
        packetError = PACKET_ERROR_NETWORK;
        return nullptr;
    }

    Content = Packet.data();
    ValidPacket = true;
    packetError = PACKET_ERROR_OK;

    // parse content
    switch (ID) {
    case PACKET_ID_INFO: {
        InfoNetPacket *pk = new InfoNetPacket(Content);
        return pk;
    }
    case PACKET_ID_CMD: {
        QString cmd, args;

        split_content(Content,cmd,args);

        CmdNetPacket *pk = new CmdNetPacket(cmd,args);
        return pk;
    }
    case PACKET_ID_STATUS: {
        QString err_no_str, err_str;
        status_t err_no;

        split_content(Content,err_no_str,err_str);

        bool ok;

        err_no = err_no_str.toLong(&ok);
        if ( !ok ) {
            ValidPacket = false;
            packetError = PACKET_ERROR_BAD_NUMERIC;
            return nullptr;
        }

        return new StatusNetPacket(err_no,err_str);
    }
    case PACKET_ID_HELLO: {
        return new NetPacket(PACKET_ID_HELLO,Content);
    }
    default:
        ValidPacket = false;
        packetError = PACKET_ERROR_UNKNOWN_PROTOCOL;
        return nullptr;
    }
}



                /****************************************
                *                                       *
                *   InfoNetPacket class implementation  *
                *                                       *
                ****************************************/


InfoNetPacket::InfoNetPacket(const QString &info): NetPacket(NetPacket::PACKET_ID_INFO,info)
{

}


InfoNetPacket::InfoNetPacket(const char* info): NetPacket(NetPacket::PACKET_ID_INFO,info)
{

}


InfoNetPacket::InfoNetPacket(): NetPacket(NetPacket::PACKET_ID_INFO,"")
{

}

QString InfoNetPacket::GetInfo() const
{
    return GetPacketContent();
}


void InfoNetPacket::SetInfo(const char *info){
    SetContent(NetPacket::PACKET_ID_INFO, info);
}


void InfoNetPacket::SetInfo(const QString &info){
    SetContent(NetPacket::PACKET_ID_INFO, info);
}



                /***************************************
                *                                      *
                *   CmdNetPacket class implementation  *
                *                                      *
                ***************************************/


CmdNetPacket::CmdNetPacket(const char *cmdname, const char *args):
    NetPacket(NetPacket::PACKET_ID_CMD), CmdName(cmdname), Args(args), ArgsVec(QVector<double>())
{
    Init();
}


CmdNetPacket::CmdNetPacket(const QString &cmdname, const QString &args):
    NetPacket(NetPacket::PACKET_ID_CMD), CmdName(cmdname), Args(args), ArgsVec(QVector<double>())
{
    Init();
}


CmdNetPacket::CmdNetPacket(const char *cmdname, const QString &args):
    NetPacket(NetPacket::PACKET_ID_CMD), CmdName(cmdname), Args(args), ArgsVec(QVector<double>())
{
    Init();
}


CmdNetPacket::CmdNetPacket(const QString &cmdname, const char *args):
    NetPacket(NetPacket::PACKET_ID_CMD), CmdName(cmdname), Args(args), ArgsVec(QVector<double>())
{
    Init();
}


CmdNetPacket::CmdNetPacket(const char *cmdname, const double args):
    NetPacket(NetPacket::PACKET_ID_CMD), CmdName(cmdname), Args(""), ArgsVec(QVector<double>())
{
    Init(args);
}


CmdNetPacket::CmdNetPacket(const QString &cmdname, const double args):
    NetPacket(NetPacket::PACKET_ID_CMD), CmdName(cmdname), Args(""), ArgsVec(QVector<double>())
{
    Init(args);
}


CmdNetPacket::CmdNetPacket(const char *cmdname, const QVector<double> &args):
    NetPacket(NetPacket::PACKET_ID_CMD), CmdName(cmdname), Args(""), ArgsVec(args)
{
    Init(args);
}


CmdNetPacket::CmdNetPacket(const QString &cmdname, const QVector<double> &args):
    NetPacket(NetPacket::PACKET_ID_CMD), CmdName(cmdname), Args(""), ArgsVec(args)
{
    Init(args);
}


CmdNetPacket::CmdNetPacket(): CmdNetPacket("","")
{

}


void CmdNetPacket::Init()
{
    CmdName = CmdName.simplified(); // remove whitespaces from start and end, and replace multiple whitespace by single one
//    CmdName.replace(' ', "_");
    CmdName.replace(NETPROTOCOL_CONTENT_DELIMETER , "_");

    CmdName = CmdName.toUpper();

    SetContent(NetPacket::PACKET_ID_CMD,CmdName + NETPROTOCOL_CONTENT_DELIMETER + Args);
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

    if ( ArgsVec.isEmpty() ) {
        return false;
    }

    return true;
}



                /******************************************
                *                                         *
                *   StatusNetPacket class implementation  *
                *                                         *
                ******************************************/


StatusNetPacket::StatusNetPacket(const status_t err_no, const QString &err_str):
    NetPacket(NetPacket::PACKET_ID_STATUS), Err_Code(err_no), Err_string(err_str)
{
    Init();
}


StatusNetPacket::StatusNetPacket(const status_t err_no, const char* err_str):
    NetPacket(NetPacket::PACKET_ID_STATUS), Err_Code(err_no), Err_string(err_str)
{
    Init();
}


StatusNetPacket::StatusNetPacket(const status_t err_no):
    StatusNetPacket(err_no,"")
{

}


StatusNetPacket::StatusNetPacket():
    StatusNetPacket(NETPROTOCOL_ERROR_UNKNOWN,"")
{

}


void StatusNetPacket::Init()
{
    QString str;

    QTextStream content(&str);

    content << Err_Code << NETPROTOCOL_CONTENT_DELIMETER << Err_string;

    SetContent(NetPacket::PACKET_ID_STATUS,str);
}


void StatusNetPacket::SetStatus(const status_t err_no, const QString &err_str)
{
    Err_Code = err_no;
    Err_string = err_str;

    Init();
}


void StatusNetPacket::SetStatus(const status_t err_no, const char *err_str)
{
    Err_Code = err_no;
    Err_string = err_str;

    Init();
}


status_t StatusNetPacket::GetStatus(QString *err_str) const
{
    *err_str = Err_string;
    return Err_Code;
}
