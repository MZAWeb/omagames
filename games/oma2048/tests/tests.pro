include(../../../common/common-tests.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_oma2048

INCLUDEPATH += ../src

HEADERS += \
    ../src/board.h \
    ../src/game.h \
    boardtests.h \
    gametests.h

SOURCES += \
    ../src/board.cpp \
    ../src/game.cpp \
    boardtests.cpp \
    gametests.cpp \
    tst_oma2048.cpp
