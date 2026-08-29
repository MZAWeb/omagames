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
    techniquetests.h \
    gradertests.h \
    boardtests.h \
    gametests.h \
    persistencetests.h \
    savedgame.h

SOURCES += \
    ../src/sudoku.cpp \
    ../src/sudokugenerator.cpp \
    ../src/candidategrid.cpp \
    ../src/sudokutechniques.cpp \
    ../src/sudokufish.cpp \
    ../src/sudokugrader.cpp \
    ../src/sudokuboard.cpp \
    ../src/cellmodel.cpp \
    ../src/sudokugame.cpp \
    solvertests.cpp \
    generatortests.cpp \
    techniquetests.cpp \
    gradertests.cpp \
    boardtests.cpp \
    gametests.cpp \
    persistencetests.cpp \
    tst_omadoku.cpp
