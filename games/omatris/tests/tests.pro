include(../../../common/common-tests.pri)
include(../../../common/env/env.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_omatris

INCLUDEPATH += ../src ../env

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
    ../src/dealtstack.h \
    ../src/lockdelay.h \
    ../src/game.h \
    ../src/calls.h \
    ../src/boardmetrics.h \
    ../src/placements.h \
    ../src/autoshift.h \
    ../src/handling.h \
    ../src/preferences.h \
    ../src/omatrisgame.h \
    ../env/omatrisobservation.h \
    ../env/omatrisenv.h \
    enginefixture.h \
    challengetests.h \
    difficultytests.h \
    piecetests.h \
    boardtests.h \
    lockdelaytests.h \
    scoringtests.h \
    autoshifttests.h \
    handlingtests.h \
    bridgefixture.h \
    inputtests.h \
    persistencetests.h \
    boardmetricstests.h \
    placementtests.h \
    envtests.h

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
    ../src/dealtstack.cpp \
    ../src/lockdelay.cpp \
    ../src/game.cpp \
    ../src/calls.cpp \
    ../src/boardmetrics.cpp \
    ../src/placements.cpp \
    ../src/autoshift.cpp \
    ../src/handling.cpp \
    ../src/preferences.cpp \
    ../src/omatrisgame.cpp \
    ../env/omatrisobservation.cpp \
    ../env/omatrisenv.cpp \
    piecetests.cpp \
    challengetests.cpp \
    difficultytests.cpp \
    boardtests.cpp \
    lockdelaytests.cpp \
    scoringtests.cpp \
    autoshifttests.cpp \
    handlingtests.cpp \
    inputtests.cpp \
    persistencetests.cpp \
    boardmetricstests.cpp \
    placementtests.cpp \
    envtests.cpp \
    tst_omatris.cpp
