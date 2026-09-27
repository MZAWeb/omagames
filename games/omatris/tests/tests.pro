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
    ../src/scoring.h \
    ../src/bonuses.h \
    ../src/challenge.h \
    ../src/difficulty.h \
    ../src/game.h \
    ../src/autoshift.h \
    ../src/handling.h \
    ../src/omatrisgame.h \
    enginefixture.h \
    challengetests.h \
    difficultytests.h \
    piecetests.h \
    boardtests.h \
    scoringtests.h \
    autoshifttests.h \
    handlingtests.h \
    bridgefixture.h \
    inputtests.h \
    persistencetests.h

SOURCES += \
    ../src/piece.cpp \
    ../src/board.cpp \
    ../src/bag.cpp \
    ../src/rules.cpp \
    ../src/modes.cpp \
    ../src/scoring.cpp \
    ../src/bonuses.cpp \
    ../src/challenge.cpp \
    ../src/difficulty.cpp \
    ../src/game.cpp \
    ../src/autoshift.cpp \
    ../src/handling.cpp \
    ../src/omatrisgame.cpp \
    piecetests.cpp \
    challengetests.cpp \
    difficultytests.cpp \
    boardtests.cpp \
    scoringtests.cpp \
    autoshifttests.cpp \
    handlingtests.cpp \
    inputtests.cpp \
    persistencetests.cpp \
    tst_omatris.cpp
