# Shared code for every Omagames app. Include from a game's .pro with
#   include(../../common/common.pri)
# It is compiled straight into each game (no static lib), which keeps qmake,
# tests and packaging trivial. Everything here is relative to this file.
QT += core gui qml quick quickcontrols2 dbus

INCLUDEPATH += $$PWD/src

HEADERS += \
    $$PWD/src/systemtheme.h \
    $$PWD/src/omarchytheme.h \
    $$PWD/src/appsetup.h

SOURCES += \
    $$PWD/src/systemtheme.cpp \
    $$PWD/src/omarchytheme.cpp \
    $$PWD/src/appsetup.cpp

RESOURCES += $$PWD/common.qrc
