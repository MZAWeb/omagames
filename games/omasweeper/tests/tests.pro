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
    ../src/generator.h \
    boardtests.h \
    solvertests.h \
    generatortests.h

SOURCES += \
    ../src/board.cpp \
    ../src/frontier.cpp \
    ../src/solver.cpp \
    ../src/generator.cpp \
    boardtests.cpp \
    solvertests.cpp \
    generatortests.cpp \
    tst_omasweeper.cpp
