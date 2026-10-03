include(../../../common/common-tests.pri)
include(../../../common/env/env.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_omasnake

INCLUDEPATH += ../src ../env
# The recorded games in the repo, which the suite plays to the end.
DEFINES += OMASNAKE_REPLAYS=\\\"$$PWD/../replays\\\"

HEADERS += \
    ../src/rules.h \
    ../src/choices.h \
    ../src/snake.h \
    ../src/game.h \
    ../src/replayplayer.h \
    ../src/omasnakegame.h \
    ../env/omasnakeenv.h \
    scenario.h \
    snaketests.h \
    foodtests.h \
    speedtests.h \
    bridgetests.h \
    envtests.h \
    replaytests.h

SOURCES += \
    ../src/rules.cpp \
    ../src/choices.cpp \
    ../src/snake.cpp \
    ../src/game.cpp \
    ../src/replayplayer.cpp \
    ../src/omasnakegame.cpp \
    ../src/omasnakegamemodel.cpp \
    ../src/omasnakegamereplay.cpp \
    ../env/omasnakeenv.cpp \
    scenario.cpp \
    snaketests.cpp \
    foodtests.cpp \
    speedtests.cpp \
    bridgetests.cpp \
    envtests.cpp \
    replaytests.cpp \
    tst_omasnake.cpp
