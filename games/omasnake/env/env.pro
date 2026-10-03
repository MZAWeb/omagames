# libomasnake_env.so: Omasnake for an agent to play (docs/AGENT-ENV.md).
# Built by bin/build-env omasnake; the engine's sources, never the bridge's.
include(../../../common/env/env.pri)

QT = core
CONFIG += c++17 release plugin
TEMPLATE = lib
TARGET = omasnake_env

INCLUDEPATH += ../src

HEADERS += \
    ../src/rules.h \
    ../src/choices.h \
    ../src/snake.h \
    ../src/game.h \
    omasnakeenv.h

SOURCES += \
    ../src/rules.cpp \
    ../src/choices.cpp \
    ../src/snake.cpp \
    ../src/game.cpp \
    omasnakeenv.cpp
