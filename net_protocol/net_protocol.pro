#-------------------------------------------------
#
# Project created by QtCreator 2014-10-07T16:35:20
#
#-------------------------------------------------

QT       -= gui

TARGET = net_protocol
TEMPLATE = lib

CMAKE_CXXFLAGS += -std=c++11

DEFINES += NET_PROTOCOL_LIBRARY

SOURCES += netpacket.cpp

HEADERS += netpacket.h\
        net_protocol_global.h \
    proto_defs.h

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}
