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

QMAKE_CXXFLAGS += -std=c++11


SOURCES += main.cpp

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../net_protocol/release/ -lnet_protocol
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../net_protocol/debug/ -lnet_protocol
else:unix: LIBS += -L$$OUT_PWD/../net_protocol/ -lnet_protocol

INCLUDEPATH += $$PWD/../net_protocol
DEPENDPATH += $$PWD/../net_protocol

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../server/release/ -lserver
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../server/debug/ -lserver
else:unix: LIBS += -L$$OUT_PWD/../server/ -lserver

INCLUDEPATH += $$PWD/../server
DEPENDPATH += $$PWD/../server

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../camera/release/ -lcamera
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../camera/debug/ -lcamera
else:unix: LIBS += -L$$OUT_PWD/../camera/ -lcamera

INCLUDEPATH += $$PWD/../camera
DEPENDPATH += $$PWD/../camera

#unix:!macx|win32: LIBS += -L$$PWD/../AndorSDK/ -landor

win32: LIBS += -L$$PWD/../AndorSDK -latmcd64m
unix:!macx: LIBS += -L$$PWD/../AndorSDK/ -landor

INCLUDEPATH += $$PWD/../AndorSDK
DEPENDPATH += $$PWD/../AndorSDK
