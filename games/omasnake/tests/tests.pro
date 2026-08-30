include(../../../common/common-tests.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_omasnake

INCLUDEPATH += ../src

HEADERS += \
    ../src/rules.h \
    ../src/choices.h \
    ../src/snake.h \
    ../src/game.h \
    ../src/omasnakegame.h \
    scenario.h \
    snaketests.h \
    foodtests.h \
    speedtests.h \
    bridgetests.h

SOURCES += \
    ../src/rules.cpp \
    ../src/choices.cpp \
    ../src/snake.cpp \
    ../src/game.cpp \
    ../src/omasnakegame.cpp \
    ../src/omasnakegamemodel.cpp \
    scenario.cpp \
    snaketests.cpp \
    foodtests.cpp \
    speedtests.cpp \
    bridgetests.cpp \
    tst_omasnake.cpp
