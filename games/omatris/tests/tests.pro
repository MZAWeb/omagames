include(../../../common/common-tests.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_omatris

INCLUDEPATH += ../src

HEADERS += \
    ../src/piece.h \
    ../src/board.h \
    ../src/bag.h \
    ../src/rules.h \
    ../src/game.h \
    ../src/omatrisgame.h \
    enginetests.h \
    bridgetests.h

SOURCES += \
    ../src/piece.cpp \
    ../src/board.cpp \
    ../src/bag.cpp \
    ../src/rules.cpp \
    ../src/game.cpp \
    ../src/omatrisgame.cpp \
    enginetests.cpp \
    bridgetests.cpp \
    tst_omatris.cpp
