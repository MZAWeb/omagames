include(../../../common/common-tests.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_omanix

INCLUDEPATH += ../src

HEADERS += \
    ../src/field.h \
    ../src/entities.h \
    ../src/level.h \
    ../src/spawn.h \
    ../src/difficulties.h \
    ../src/game.h \
    ../src/omanixgame.h \
    scenario.h \
    fieldtests.h \
    painttests.h \
    movertests.h \
    playtests.h \
    scoringtests.h \
    bridgetests.h

SOURCES += \
    ../src/field.cpp \
    ../src/fieldpainter.cpp \
    ../src/entities.cpp \
    ../src/level.cpp \
    ../src/spawn.cpp \
    ../src/difficulties.cpp \
    ../src/game.cpp \
    ../src/omanixgame.cpp \
    ../src/omanixgamemodel.cpp \
    scenario.cpp \
    fieldtests.cpp \
    painttests.cpp \
    movertests.cpp \
    playtests.cpp \
    scoringtests.cpp \
    bridgetests.cpp \
    tst_omanix.cpp
