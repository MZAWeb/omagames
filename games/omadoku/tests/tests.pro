include(../../../common/common-tests.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_omadoku

INCLUDEPATH += ../src

HEADERS += \
    testgrids.h \
    solvertests.h

SOURCES += \
    ../src/sudoku.cpp \
    solvertests.cpp \
    tst_omadoku.cpp
