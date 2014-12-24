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
#DEFINES += EMULATOR_MODE

SOURCES += camera.cpp \
    temppollingthread.cpp \
    statuspollingthread.cpp

HEADERS += camera.h\
        camera_global.h \
    temppollingthread.h \
    statuspollingthread.h

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

#win32: LIBS += -L$$PWD/../AndorSDK -latmcd64m
#unix:!macx: LIBS += -L$$PWD/../AndorSDK/ -landor

#INCLUDEPATH += $$PWD/../AndorSDK
#DEPENDPATH += $$PWD/../AndorSDK

include("../andor.pri")

#win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../cfitsio/ -lcfitsio

#win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../cfitsio/x64/ -lcfitsio
#else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../cfitsio/x64/ -lcfitsio
#else:unix:!macx: LIBS += -L$$PWD/../cfitsio/ -lcfitsio

#INCLUDEPATH += $$PWD/../cfitsio
#DEPENDPATH += $$PWD/../cfitsio

include("../cfitsio.pri")

#message($$INCLUDEPATH)
#contains(QMAKE_TARGET.arch, x86_64) {
#    message("ZZZZ")
#}
