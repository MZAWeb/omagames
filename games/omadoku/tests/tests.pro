include(../../../common/common-tests.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_omadoku

INCLUDEPATH += ../src

HEADERS += \
    testgrids.h \
    solvertests.h \
    generatortests.h \
    gradertests.h \
    boardtests.h

SOURCES += \
    ../src/sudoku.cpp \
    ../src/sudokugenerator.cpp \
    ../src/sudokugrader.cpp \
    ../src/sudokuboard.cpp \
    solvertests.cpp \
    generatortests.cpp \
    gradertests.cpp \
    boardtests.cpp \
    tst_omadoku.cpp
