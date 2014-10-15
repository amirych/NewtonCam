TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp

QMAKE_CXXFLAGS += -std=c++11

win32: LIBS += -L$$PWD/../AndorSDK -latmcd64m
unix:!macx: LIBS += -L$$PWD/../AndorSDK/ -landor

INCLUDEPATH += $$PWD/../AndorSDK
DEPENDPATH += $$PWD/../AndorSDK
