#-------------------------------------------------
#
# Project created by QtCreator 2014-10-11T19:28:12
#
#-------------------------------------------------

QT       += core
QT       -= gui

TARGET = camera
TEMPLATE = lib

QMAKE_CXXFLAGS += -std=c++11

DEFINES += CAMERA_LIBRARY

SOURCES += camera.cpp \
    temppollingthread.cpp

HEADERS += camera.h\
        camera_global.h \
    temppollingthread.h

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

#unix:!macx|win32: LIBS += -L$$PWD/../AndorSDK/ -landor

win32: LIBS += -L$$PWD/../AndorSDK -latmcd64m
unix:!macx: LIBS += -L$$PWD/../AndorSDK/ -landor

INCLUDEPATH += $$PWD/../AndorSDK
DEPENDPATH += $$PWD/../AndorSDK
