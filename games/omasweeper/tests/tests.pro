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
    ../src/omasweepergame.h \
    boardtests.h \
    solvertests.h \
    generatortests.h \
    bridgetests.h

SOURCES += \
    ../src/presets.cpp \
    ../src/board.cpp \
    ../src/frontier.cpp \
    ../src/solver.cpp \
    ../src/generator.cpp \
    ../src/omasweepergame.cpp \
    ../src/omasweepergamemodel.cpp \
    boardtests.cpp \
    solvertests.cpp \
    generatortests.cpp \
    bridgetests.cpp \
    tst_omasweeper.cpp
