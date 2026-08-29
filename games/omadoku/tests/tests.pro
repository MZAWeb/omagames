include(../../../common/common-tests.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_omadoku

INCLUDEPATH += ../src

HEADERS += \
    ../src/cellmodel.h \
    ../src/sudokugame.h \
    testgrids.h \
    solvertests.h \
    generatortests.h \
    gradertests.h \
    boardtests.h \
    gametests.h

SOURCES += \
    ../src/sudoku.cpp \
    ../src/sudokugenerator.cpp \
    ../src/sudokugrader.cpp \
    ../src/sudokuboard.cpp \
    ../src/cellmodel.cpp \
    ../src/sudokugame.cpp \
    solvertests.cpp \
    generatortests.cpp \
    gradertests.cpp \
    boardtests.cpp \
    gametests.cpp \
    tst_omadoku.cpp
