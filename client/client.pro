#-------------------------------------------------
#
# Project created by QtCreator 2014-10-16T21:34:23
#
#-------------------------------------------------

QT       += core network

QT       -= gui

TARGET = client
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11

SOURCES += main.cpp

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../net_protocol/release/ -lnet_protocol
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../net_protocol/debug/ -lnet_protocol
else:unix:!macx: LIBS += -L$$OUT_PWD/../net_protocol/ -lnet_protocol

INCLUDEPATH += $$PWD/../net_protocol
DEPENDPATH += $$PWD/../net_protocol
