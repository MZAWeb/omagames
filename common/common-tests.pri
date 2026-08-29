# Headless subset of common/ for QtTest binaries (no QML engine, no fonts).
QT += core gui dbus
INCLUDEPATH += $$PWD/src
HEADERS += $$PWD/src/omarchytheme.h
SOURCES += $$PWD/src/omarchytheme.cpp
