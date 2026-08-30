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
    ../src/besttimes.h \
    ../src/omasweepergame.h \
    boardtests.h \
    solvertests.h \
    generatortests.h \
    bridgetests.h

SOURCES += \
    ../src/board.cpp \
    ../src/frontier.cpp \
    ../src/solver.cpp \
    ../src/generator.cpp \
    ../src/besttimes.cpp \
    ../src/omasweepergame.cpp \
    boardtests.cpp \
    solvertests.cpp \
    generatortests.cpp \
    bridgetests.cpp \
    tst_omasweeper.cpp
