#include "netpacket.h"
#include "proto_defs.h"

#include<QTextStream>
#include<QStringList>
#include <iostream>


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
    if ( socket == nullptr ) {
        packetError = NetPacket::PACKET_ERROR_NO_SOCKET;
        return false;
    }

    qint64 n = socket->write(Packet);

    if ( n == -1 ) {
        packetError = NetPacket::PACKET_ERROR_NETWORK;
        return false;
    }

    return socket->waitForBytesWritten(timeout);
}


//
// This function is blocking a thread for <= timeout millisecs!!!
//
NetPacket& NetPacket::Receive(QTcpSocket *socket, int timeout)
{

    if ( socket == nullptr ) {
        packetError = NetPacket::PACKET_ERROR_NO_SOCKET;
        return *this;
    }

    bool ok;

    if ( ValidPacket ) {
        while ( socket->bytesAvailable() < (NETPROTOCOL_ID_FIELD_LEN + NETPROTOCOL_SIZE_FIELD_LEN) ) {
            if ( !socket->waitForReadyRead(timeout) ) {
                packetError = NetPacket::PACKET_ERROR_WAIT;
                return *this;
            }
        }
        ValidPacket = false;

        Packet = socket->read(NETPROTOCOL_ID_FIELD_LEN + NETPROTOCOL_SIZE_FIELD_LEN);
        if ( Packet.size() < (NETPROTOCOL_ID_FIELD_LEN + NETPROTOCOL_SIZE_FIELD_LEN) ) { // IT IS AN ERROR!!!
            packetError = PACKET_ERROR_NETWORK;
            return *this;
        }

//    #ifdef QT_DEBUG
//        qDebug() << "NETPACKET: read header [" << Packet << "]";
//    #endif

        long id_num = Packet.left(NETPROTOCOL_ID_FIELD_LEN).toLong(&ok);
        if (!ok) { // IT IS AN ERROR!!!
            packetError = PACKET_ERROR_UNKNOWN_PROTOCOL;
            return *this;
        }

        ID = static_cast<NetPacket::NetPacketID>(id_num);

        Content_LEN = Packet.right(NETPROTOCOL_SIZE_FIELD_LEN).toLong(&ok);
        if (!ok) { // IT IS AN ERROR!!!
            packetError = PACKET_ERROR_UNKNOWN_PROTOCOL;
            return *this;
        }

//    #ifdef QT_DEBUG
//        qDebug() << "NETPACKET: ID = " << ID << ", LEN = " << Content_LEN;
//    #endif

        if ( Content_LEN < 0 ) { // IT IS AN ERROR!!!
            packetError = PACKET_ERROR_BAD_NUMERIC;
            return *this;
        }
    }

    while ( socket->bytesAvailable() < Content_LEN ) { // waiting for entire packet content
        if ( !socket->waitForReadyRead(timeout) ) {
            packetError = NetPacket::PACKET_ERROR_WAIT;
            return *this;
        }
    }

    Packet += socket->read(Content_LEN);

    if ( Packet.size() < (Content_LEN + NETPROTOCOL_ID_FIELD_LEN + NETPROTOCOL_SIZE_FIELD_LEN) ) { // IT IS AN ERROR!!!
        packetError = PACKET_ERROR_NETWORK;
        return *this;
    }

    Content = Packet.right(Content_LEN).data();

//#ifdef QT_DEBUG
//        qDebug() << "NETPACKET: content = [" << Packet << "]";
//#endif

    ValidPacket = true;
    packetError = PACKET_ERROR_OK;

    return *this;
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


InfoNetPacket& InfoNetPacket::Receive(QTcpSocket *socket, int timeout)
{
    NetPacket::Receive(socket,timeout);

    if ( !ValidPacket ) return *this;

    if ( ID != NetPacket::PACKET_ID_INFO ) {
        ValidPacket = false;
        packetError = NetPacket::PACKET_ERROR_ID_MISMATCH;
    }

    return *this;
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


CmdNetPacket::CmdNetPacket(const NetPacket &packet): NetPacket(NetPacket::PACKET_ID_CMD)
{
    SetContent(NetPacket::PACKET_ID_CMD,packet.GetPacketContent());
    ParseContent();
}


CmdNetPacket& CmdNetPacket::operator =(const NetPacket &packet)
{
    SetContent(NetPacket::PACKET_ID_CMD,packet.GetPacketContent());
    ParseContent();

    return *this;
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
    if ( args == nullptr ) return false;

    if ( ArgsVec.isEmpty() ) { // may be packet was just read from network, then try to re-parse content
        bool ok;
        double num;

        QStringList str_vec = Args.split(NETPROTOCOL_CONTENT_DELIMETER,QString::SkipEmptyParts);
        if ( str_vec.isEmpty() ) return false;

        foreach (QString num_str, str_vec) {
            num = num_str.toDouble(&ok);
            if ( !ok ) {
                ArgsVec.clear();
                return false;
            }
            ArgsVec << num;
        }
    }

    *args = ArgsVec;

    return true;
}


void CmdNetPacket::ParseContent()
{
    ArgsVec.clear();

    split_content(Content,CmdName,Args);
}

CmdNetPacket& CmdNetPacket::Receive(QTcpSocket *socket, int timeout)
{
    NetPacket::Receive(socket,timeout);
    if ( !ValidPacket ) return *this;

    if ( ID != NetPacket::PACKET_ID_CMD ) {
        ValidPacket = false;
        packetError = NetPacket::PACKET_ERROR_ID_MISMATCH;
        return *this;
    }

    ParseContent();

    return *this;
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


StatusNetPacket::StatusNetPacket(const NetPacket &packet): NetPacket(NetPacket::PACKET_ID_STATUS)
{
    SetContent(NetPacket::PACKET_ID_STATUS,packet.GetPacketContent());
    ParseContent();
}


StatusNetPacket& StatusNetPacket::operator =(const NetPacket &packet)
{
    SetContent(NetPacket::PACKET_ID_STATUS,packet.GetPacketContent());
    ParseContent();

    return *this;
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
    if ( err_str != nullptr ) *err_str = Err_string;
    return Err_Code;
}


void StatusNetPacket::ParseContent()
{
    QString err_no_str;

    split_content(Content,err_no_str,Err_string);

    bool ok;

    Err_Code = err_no_str.toLong(&ok);
    if ( !ok ) {
        ValidPacket = false;
        packetError = PACKET_ERROR_BAD_NUMERIC;
    }

    ValidPacket = true;
}

StatusNetPacket& StatusNetPacket::Receive(QTcpSocket *socket, int timeout)
{
    NetPacket::Receive(socket,timeout);
    if ( !ValidPacket ) return *this;

    if ( ID != NetPacket::PACKET_ID_STATUS) {
        ValidPacket = false;
        packetError = NetPacket::PACKET_ERROR_ID_MISMATCH;
        return *this;
    }

    ParseContent();

    return *this;
}


                /****************************************
                *                                       *
                *   TempNetPacket class implementation  *
                *                                       *
                ****************************************/


TempNetPacket::TempNetPacket(const double temp, const unsigned int cooling_status):
    NetPacket(NetPacket::PACKET_ID_TEMP), Temp(temp), CoollingStatus(cooling_status)
{
    QString str;
    QTextStream st(&str);

    st << Temp << NETPROTOCOL_CONTENT_DELIMETER << CoollingStatus;

    SetContent(NetPacket::PACKET_ID_TEMP,str);
}


TempNetPacket::TempNetPacket(): TempNetPacket(0.0,0)
{

}


TempNetPacket::TempNetPacket(const NetPacket &packet): NetPacket(NetPacket::PACKET_ID_TEMP)
{
    SetContent(NetPacket::PACKET_ID_TEMP,packet.GetPacketContent());
    ParseContent();
}


TempNetPacket& TempNetPacket::operator =(const NetPacket &packet)
{
    Content = packet.GetPacketContent();
    ParseContent();

    return *this;
}


void TempNetPacket::SetTemp(const double temp, const unsigned int cooling_status)
{
    Temp = temp;
    CoollingStatus = cooling_status;
}


double TempNetPacket::GetTemp(unsigned int *cooling_status) const
{
    if ( cooling_status != nullptr ) *cooling_status = CoollingStatus;
    return Temp;
}


void TempNetPacket::ParseContent()
{
    QStringList vals = Content.split(NETPROTOCOL_CONTENT_DELIMETER, QString::SkipEmptyParts);

    if ( vals.empty() ) {
        packetError = NetPacket::PACKET_ERROR_BAD_NUMERIC;
        return;
    }

    bool ok;

    Temp = vals[0].toDouble(&ok);
    if ( !ok ) {
        packetError = NetPacket::PACKET_ERROR_BAD_NUMERIC;
        return;
    }

    if ( vals.length() > 1 ) {
        CoollingStatus = vals[1].toUInt(&ok);
        if ( !ok ) {
            packetError = NetPacket::PACKET_ERROR_BAD_NUMERIC;
            return;
        }
    } else CoollingStatus = 0;

    packetError = NetPacket::PACKET_ERROR_OK;
}


TempNetPacket& TempNetPacket::Receive(QTcpSocket *socket, int timeout)
{
    NetPacket::Receive(socket, timeout);

    if ( !ValidPacket ) return *this;

    if ( ID != NetPacket::PACKET_ID_TEMP ) {
        ValidPacket = false;
        packetError = NetPacket::PACKET_ERROR_ID_MISMATCH;
        return *this;
    }

    ParseContent();

    return *this;
}


                /*****************************************
                *                                        *
                *   HelloNetPacket class implementation  *
                *                                        *
                *****************************************/

HelloNetPacket::HelloNetPacket(const QString &sender_type, const QString &version):
    NetPacket(NetPacket::PACKET_ID_HELLO), SenderType(sender_type), SenderVersion(version)
{
    if ( SenderVersion.isEmpty() ) SetContent(NetPacket::PACKET_ID_HELLO,SenderType);
    else SetContent(NetPacket::PACKET_ID_HELLO,SenderType + NETPROTOCOL_CONTENT_DELIMETER + SenderVersion);
}


HelloNetPacket::HelloNetPacket(const char *sender_type, const char *version):
    HelloNetPacket(QString(sender_type),QString(version))
{

}


HelloNetPacket::HelloNetPacket(const char *sender_type, const QString &version):
    HelloNetPacket(QString(sender_type),version)
{

}


HelloNetPacket::HelloNetPacket(const QString &sender_type, const char *version):
    HelloNetPacket(sender_type,QString(version))
{

}


HelloNetPacket::HelloNetPacket(): HelloNetPacket("","")
{

}


HelloNetPacket::HelloNetPacket(const NetPacket &packet): NetPacket(NetPacket::PACKET_ID_HELLO,"")
{
    Content = packet.GetPacketContent();

    ParseContent();
}


HelloNetPacket& HelloNetPacket::operator = (const NetPacket &packet)
{
    Content = packet.GetPacketContent();

    ParseContent();

    return *this;
}


void HelloNetPacket::SetSenderType(const QString &sender_type, const QString &version)
{
    SenderType = sender_type;
    SenderVersion = version;

    SetContent(NetPacket::PACKET_ID_HELLO,SenderType + NETPROTOCOL_CONTENT_DELIMETER + SenderVersion);
}


void HelloNetPacket::SetSenderType(const char *sender_type, const char *version)
{
    SetSenderType(QString(sender_type),QString(version));
}


void HelloNetPacket::SetSenderType(const QString &sender_type, const char *version)
{
    SetSenderType(sender_type,QString(version));
}


void HelloNetPacket::SetSenderType(const char *sender_type, const QString &version)
{
    SetSenderType(QString(sender_type),version);
}


QString HelloNetPacket::GetSenderType(QString *version) const
{
    if (version != nullptr ) *version = SenderVersion;
    return SenderType;
}


void HelloNetPacket::ParseContent()
{
    QString str = Content.trimmed();

    int idx = str.indexOf(NETPROTOCOL_CONTENT_DELIMETER);

    if ( idx != -1 ) {
        SenderType = str.left(idx);
        SenderVersion = str.right(str.length()-idx-1);
    } else {
        SenderType = str;
        SenderVersion = "";
    }

    packetError = NetPacket::PACKET_ERROR_OK;
}


HelloNetPacket& HelloNetPacket::Receive(QTcpSocket *socket, int timeout)
{
    NetPacket::Receive(socket,timeout);

    if ( !ValidPacket ) return *this;

    if ( ID != NetPacket::PACKET_ID_HELLO ) {
        ValidPacket = false;
        packetError = NetPacket::PACKET_ERROR_ID_MISMATCH;
        return *this;
    }

    ParseContent();

    return *this;
}
