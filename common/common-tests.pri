# Headless subset of common/ for QtTest binaries (no QML engine, no fonts).
QT += core gui dbus
INCLUDEPATH += $$PWD/src
HEADERS += \
    $$PWD/src/omarchytheme.h \
    $$PWD/src/scoretable.h \
    $$PWD/src/windowgeometry.h \
    $$PWD/src/pacer.h

SOURCES += \
    $$PWD/src/omarchytheme.cpp \
    $$PWD/src/scoretable.cpp \
    $$PWD/src/windowgeometry.cpp \
    $$PWD/src/pacer.cpp
