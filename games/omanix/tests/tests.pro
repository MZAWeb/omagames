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
    ../src/difficulties.h \
    ../src/game.h \
    ../src/omanixgame.h \
    enginetests.h \
    bridgetests.h

SOURCES += \
    ../src/field.cpp \
    ../src/entities.cpp \
    ../src/level.cpp \
    ../src/difficulties.cpp \
    ../src/game.cpp \
    ../src/omanixgame.cpp \
    ../src/omanixgamemodel.cpp \
    enginetests.cpp \
    bridgetests.cpp \
    tst_omanix.cpp
