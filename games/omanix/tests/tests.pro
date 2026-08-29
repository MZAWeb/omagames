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
    ../src/game.h \
    ../src/highscores.h \
    ../src/omanixgame.h \
    enginetests.h \
    bridgetests.h

SOURCES += \
    ../src/field.cpp \
    ../src/entities.cpp \
    ../src/level.cpp \
    ../src/game.cpp \
    ../src/highscores.cpp \
    ../src/omanixgame.cpp \
    enginetests.cpp \
    bridgetests.cpp \
    tst_omanix.cpp
