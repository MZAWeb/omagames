include(../../../common/common-tests.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_omatris

INCLUDEPATH += ../src

HEADERS += \
    ../src/piece.h \
    ../src/board.h \
    ../src/bag.h \
    ../src/rules.h \
    ../src/modes.h \
    ../src/game.h \
    ../src/autoshift.h \
    ../src/omatrisgame.h \
    enginefixture.h \
    piecetests.h \
    boardtests.h \
    scoringtests.h \
    autoshifttests.h \
    bridgefixture.h \
    inputtests.h \
    persistencetests.h

SOURCES += \
    ../src/piece.cpp \
    ../src/board.cpp \
    ../src/bag.cpp \
    ../src/rules.cpp \
    ../src/modes.cpp \
    ../src/game.cpp \
    ../src/autoshift.cpp \
    ../src/omatrisgame.cpp \
    piecetests.cpp \
    boardtests.cpp \
    scoringtests.cpp \
    autoshifttests.cpp \
    inputtests.cpp \
    persistencetests.cpp \
    tst_omatris.cpp
