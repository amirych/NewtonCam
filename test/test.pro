#-------------------------------------------------
#
# Project created by QtCreator 2014-10-09T17:46:05
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = test
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../net_protocol/release/ -lnet_protocol
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../net_protocol/debug/ -lnet_protocol
else:unix: LIBS += -L$$OUT_PWD/../net_protocol/ -lnet_protocol

INCLUDEPATH += $$PWD/../net_protocol
DEPENDPATH += $$PWD/../net_protocol
