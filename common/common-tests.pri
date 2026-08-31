# Headless subset of common/ for QtTest binaries (no QML engine, no fonts).
QT += core gui dbus
INCLUDEPATH += $$PWD/src

# Qt's own headers are not our code: -isystem keeps their diagnostics out of
# the build, so a warning here is always about something we can fix. It is the
# same line CI's -Werror job uses, which is what makes a clean local build and
# a green CI mean the same thing.
QMAKE_CXXFLAGS += -isystem $$[QT_INSTALL_HEADERS]
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
