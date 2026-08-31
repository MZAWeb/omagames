include(../../../common/common-tests.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_oma2048

INCLUDEPATH += ../src

HEADERS += \
    ../src/board.h \
    ../src/game.h \
    ../src/oma2048game.h \
    boardtests.h \
    bridgetests.h \
    gametests.h

SOURCES += \
    ../src/board.cpp \
    ../src/game.cpp \
    ../src/oma2048game.cpp \
    ../src/oma2048gamemodel.cpp \
    boardtests.cpp \
    bridgetests.cpp \
    gametests.cpp \
    tst_oma2048.cpp
