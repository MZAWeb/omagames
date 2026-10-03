include(../../../common/common-tests.pri)
include(../../../common/env/env.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_oma2048

INCLUDEPATH += ../src ../env

HEADERS += \
    ../src/rules.h \
    ../src/board.h \
    ../env/oma2048env.h \
    envtests.h \
    ../src/game.h \
    ../src/oma2048game.h \
    boardtests.h \
    bridgetests.h \
    gametests.h

SOURCES += \
    ../src/board.cpp \
    ../env/oma2048env.cpp \
    envtests.cpp \
    ../src/game.cpp \
    ../src/oma2048game.cpp \
    ../src/oma2048gamemodel.cpp \
    boardtests.cpp \
    bridgetests.cpp \
    gametests.cpp \
    tst_oma2048.cpp
