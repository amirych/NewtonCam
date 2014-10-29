#-------------------------------------------------
#
# Project created by QtCreator 2014-10-24T11:12:21
#
#-------------------------------------------------

QT       += widgets

TARGET = serverGUI
TEMPLATE = lib

DEFINES += SERVERGUI_LIBRARY

QMAKE_CXXFLAGS += -std=c++11


SOURCES += servergui.cpp

HEADERS += servergui.h\
        servergui_global.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}
