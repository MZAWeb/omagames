include(../../../common/common-tests.pri)
include(../../../common/env/env.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_omasnake

INCLUDEPATH += ../src ../env

HEADERS += \
    ../src/rules.h \
    ../src/choices.h \
    ../src/snake.h \
    ../src/game.h \
    ../src/omasnakegame.h \
    ../env/omasnakeenv.h \
    scenario.h \
    snaketests.h \
    foodtests.h \
    speedtests.h \
    bridgetests.h \
    envtests.h

SOURCES += \
    ../src/rules.cpp \
    ../src/choices.cpp \
    ../src/snake.cpp \
    ../src/game.cpp \
    ../src/omasnakegame.cpp \
    ../src/omasnakegamemodel.cpp \
    ../env/omasnakeenv.cpp \
    scenario.cpp \
    snaketests.cpp \
    foodtests.cpp \
    speedtests.cpp \
    bridgetests.cpp \
    envtests.cpp \
    tst_omasnake.cpp
