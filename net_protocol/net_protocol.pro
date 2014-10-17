#-------------------------------------------------
#
# Project created by QtCreator 2014-10-07T16:35:20
#
#-------------------------------------------------

QT       -= gui
QT       += network

TARGET = net_protocol
TEMPLATE = lib

QMAKE_CXXFLAGS += -std=c++11

DEFINES += NET_PROTOCOL_LIBRARY

SOURCES += netpacket.cpp \
    netpackethandler.cpp

HEADERS += netpacket.h\
        net_protocol_global.h \
    proto_defs.h \
    netpackethandler.h

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}
