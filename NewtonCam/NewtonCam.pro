#-------------------------------------------------
#
# Project created by QtCreator 2014-11-04T14:10:03
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QMAKE_CXXFLAGS += -std=c++11


TARGET = NewtonCam
CONFIG   += console
#CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../server/release/ -lserver
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../server/debug/ -lserver
else:unix:!macx: LIBS += -L$$OUT_PWD/../server/ -lserver

INCLUDEPATH += $$PWD/../server
DEPENDPATH += $$PWD/../server

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../serverGUI/release/ -lserverGUI
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../serverGUI/debug/ -lserverGUI
else:unix:!macx: LIBS += -L$$OUT_PWD/../serverGUI/ -lserverGUI

INCLUDEPATH += $$PWD/../serverGUI
DEPENDPATH += $$PWD/../serverGUI

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../net_protocol/release/ -lnet_protocol
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../net_protocol/debug/ -lnet_protocol
else:unix:!macx: LIBS += -L$$OUT_PWD/../net_protocol/ -lnet_protocol

INCLUDEPATH += $$PWD/../net_protocol
DEPENDPATH += $$PWD/../net_protocol

#win32: LIBS += -L$$PWD/../AndorSDK -latmcd64m
#unix:!macx: LIBS += -L$$PWD/../AndorSDK/ -landor

#INCLUDEPATH += $$PWD/../AndorSDK
#DEPENDPATH += $$PWD/../AndorSDK

include("../andor.pri")

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../camera/release/ -lcamera
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../camera/debug/ -lcamera
else:unix:!macx: LIBS += -L$$OUT_PWD/../camera/ -lcamera

INCLUDEPATH += $$PWD/../camera
DEPENDPATH += $$PWD/../camera

#win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../cfitsio/ -lcfitsio
#else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../cfitsio/ -lcfitsio
#else:unix:!macx: LIBS += -L$$PWD/../cfitsio/ -lcfitsio

#INCLUDEPATH += $$PWD/../cfitsio
#DEPENDPATH += $$PWD/../cfitsio

include("../cfitsio.pri")
