TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp

QMAKE_CXXFLAGS += -std=c++11

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../AndorSDK/release/ -landor
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../AndorSDK/debug/ -landor
else:unix:!macx: LIBS += -L$$PWD/../AndorSDK/ -landor

INCLUDEPATH += $$PWD/../AndorSDK
DEPENDPATH += $$PWD/../AndorSDK
