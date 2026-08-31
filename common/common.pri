# Shared code for every Omagames app. Include from a game's .pro with
#   include(../../common/common.pri)
# It is compiled straight into each game (no static lib), which keeps qmake,
# tests and packaging trivial. Everything here is relative to this file.
QT += core gui qml quick quickcontrols2 dbus

INCLUDEPATH += $$PWD/src

# Qt's own headers are not our code: -isystem keeps their diagnostics out of
# the build, so a warning here is always about something we can fix. It is the
# same line CI's -Werror job uses, which is what makes a clean local build and
# a green CI mean the same thing.
QMAKE_CXXFLAGS += -isystem $$[QT_INSTALL_HEADERS]

HEADERS += \
    $$PWD/src/systemtheme.h \
    $$PWD/src/omarchytheme.h \
    $$PWD/src/appsetup.h \
    $$PWD/src/scoretable.h \
    $$PWD/src/windowgeometry.h \
    $$PWD/src/pacer.h

SOURCES += \
    $$PWD/src/systemtheme.cpp \
    $$PWD/src/omarchytheme.cpp \
    $$PWD/src/appsetup.cpp \
    $$PWD/src/scoretable.cpp \
    $$PWD/src/windowgeometry.cpp \
    $$PWD/src/pacer.cpp

RESOURCES += $$PWD/common.qrc
