include(../../../common/common-tests.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_blackomack

INCLUDEPATH += ../src
HEADERS += \
    ../src/cards.h \
    ../src/hand.h \
    ../src/blackjackrules.h \
    ../src/basicstrategy.h \
    ../src/coach.h \
    ../src/botplayer.h \
    ../src/table.h \
    ../src/tablelog.h \
    ../src/gamestate.h \
    ../src/sessionstats.h \
    ../src/seatlayout.h \
    ../src/blackjackgame.h \
    testhelpers.h \
    cardtests.h \
    ruletests.h \
    tabletests.h \
    seatlayouttests.h \
    insurancetests.h \
    bridgetests.h \
    persistencetests.h \
    houseedgetests.h
SOURCES += \
    cardtests.cpp \
    ruletests.cpp \
    tabletests.cpp \
    seatlayouttests.cpp \
    insurancetests.cpp \
    bridgetests.cpp \
    persistencetests.cpp \
    houseedgetests.cpp \
    tst_blackomack.cpp \
    ../src/cards.cpp \
    ../src/hand.cpp \
    ../src/blackjackrules.cpp \
    ../src/basicstrategy.cpp \
    ../src/coach.cpp \
    ../src/botplayer.cpp \
    ../src/table.cpp \
    ../src/tableround.cpp \
    ../src/tablelog.cpp \
    ../src/gamestate.cpp \
    ../src/sessionstats.cpp \
    ../src/seatlayout.cpp \
    ../src/blackjackgame.cpp \
    ../src/blackjackgamestore.cpp \
    ../src/blackjackgamemodel.cpp
