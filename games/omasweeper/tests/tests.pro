include(../../../common/common-tests.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_omasweeper

INCLUDEPATH += ../src

HEADERS += \
    ../src/presets.h \
    ../src/board.h \
    ../src/frontier.h \
    ../src/solver.h \
    boardtests.h \
    solvertests.h

SOURCES += \
    ../src/board.cpp \
    ../src/frontier.cpp \
    ../src/solver.cpp \
    boardtests.cpp \
    solvertests.cpp \
    tst_omasweeper.cpp
