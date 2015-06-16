#-------------------------------------------------
#
# Project created by QtCreator 2014-10-29T11:58:36
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = NewtonGUI
TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11

SOURCES += main.cpp \
    newtongui.cpp

HEADERS  += \
    newtongui.h

RC_ICONS = ../andor.ico


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
