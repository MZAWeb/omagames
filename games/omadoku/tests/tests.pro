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
    gradertests.h

SOURCES += \
    ../src/sudoku.cpp \
    ../src/sudokugenerator.cpp \
    ../src/sudokugrader.cpp \
    solvertests.cpp \
    generatortests.cpp \
    gradertests.cpp \
    tst_omadoku.cpp
