contains(QMAKE_TARGET.arch, x86_64) {
  win32: LIBS += -L$$PWD/AndorSDK -latmcd64m
} else {
  win32: LIBS += -L$$PWD/AndorSDK -latmcd32m
}
unix:!macx: LIBS += -L$$PWD/AndorSDK/ -landor

INCLUDEPATH += $$PWD/AndorSDK
DEPENDPATH += $$PWD/AndorSDK
