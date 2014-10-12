#-------------------------------------------------
#
# Project created by QtCreator 2014-10-11T10:11:18
#
#-------------------------------------------------

QT       -= gui

TARGET = server
TEMPLATE = lib

DEFINES += SERVER_LIBRARY

SOURCES += server.cpp

HEADERS += server.h\
        server_global.h

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}
