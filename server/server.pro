#-------------------------------------------------
#
# Project created by QtCreator 2014-10-11T10:11:18
#
#-------------------------------------------------

QT       -= gui
QT       += network

TARGET = server
TEMPLATE = lib

QMAKE_CXXFLAGS += -std=c++11

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


win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../net_protocol/release/ -lnet_protocol
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../net_protocol/debug/ -lnet_protocol
else:unix: LIBS += -L$$OUT_PWD/../net_protocol/ -lnet_protocol

INCLUDEPATH += $$PWD/../net_protocol
DEPENDPATH += $$PWD/../net_protocol

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../camera/release/ -lcamera
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../camera/debug/ -lcamera
else:unix:!macx: LIBS += -L$$OUT_PWD/../camera/ -lcamera

INCLUDEPATH += $$PWD/../camera
DEPENDPATH += $$PWD/../camera

INCLUDEPATH += $$PWD/../AndorSDK
