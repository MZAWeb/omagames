include(../../../common/common-tests.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_omasweeper

INCLUDEPATH += ../src

HEADERS += \
    ../src/presets.h \
    ../src/board.h \
    boardtests.h

SOURCES += \
    ../src/board.cpp \
    boardtests.cpp \
    tst_omasweeper.cpp
