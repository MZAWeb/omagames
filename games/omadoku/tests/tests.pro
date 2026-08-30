include(../../../common/common-tests.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_omadoku

INCLUDEPATH += ../src

HEADERS += \
    ../src/cellmodel.h \
    ../src/sudokukeys.h \
    ../src/sudokulevels.h \
    ../src/sudokuselection.h \
    ../src/sudokustore.h \
    ../src/sudokugame.h \
    testgrids.h \
    solvertests.h \
    generatortests.h \
    techniquetests.h \
    gradertests.h \
    boardtests.h \
    leveltests.h \
    gametests.h \
    selectiontests.h \
    inputtests.h \
    besttimestests.h \
    persistencetests.h \
    gameprobe.h \
    savedgame.h

SOURCES += \
    ../src/sudoku.cpp \
    ../src/sudokugenerator.cpp \
    ../src/candidategrid.cpp \
    ../src/sudokutechniques.cpp \
    ../src/sudokufish.cpp \
    ../src/sudokugrader.cpp \
    ../src/sudokuboard.cpp \
    ../src/sudokulevels.cpp \
    ../src/sudokuselection.cpp \
    ../src/sudokustore.cpp \
    ../src/sudokukeys.cpp \
    ../src/cellmodel.cpp \
    ../src/sudokugame.cpp \
    solvertests.cpp \
    generatortests.cpp \
    techniquetests.cpp \
    gradertests.cpp \
    boardtests.cpp \
    leveltests.cpp \
    gametests.cpp \
    selectiontests.cpp \
    inputtests.cpp \
    besttimestests.cpp \
    persistencetests.cpp \
    tst_omadoku.cpp
