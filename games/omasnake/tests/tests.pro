include(../../../common/common-tests.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_omasnake

INCLUDEPATH += ../src

HEADERS += \
    ../src/rules.h \
    ../src/snake.h \
    ../src/game.h \
    enginetests.h

SOURCES += \
    ../src/rules.cpp \
    ../src/snake.cpp \
    ../src/game.cpp \
    enginetests.cpp \
    tst_omasnake.cpp
