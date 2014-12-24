contains(QMAKE_TARGET.arch, x86_64) {
  win32: LIBS += -L$$PWD/cfitsio/x64/ -lcfitsio
  else:unix:!macx: LIBS += -L$$PWD/cfitsio/ -lcfitsio
} else {
  win32: LIBS += -L$$PWD/cfitsio/ -lcfitsio
  else:unix:!macx: LIBS += -L$$PWD/cfitsio/ -lcfitsio
}
INCLUDEPATH += $$PWD/cfitsio
DEPENDPATH += $$PWD/cfitsio
