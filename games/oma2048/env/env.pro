# liboma2048_env.so: Oma2048 for an agent to play (docs/AGENT-ENV.md).
# Built by bin/build-env oma2048; the engine's sources, never the bridge's.
include(../../../common/env/env.pri)

QT = core
CONFIG += c++17 release plugin
TEMPLATE = lib
TARGET = oma2048_env

INCLUDEPATH += ../src

HEADERS += \
    ../src/rules.h \
    ../src/board.h \
    ../src/game.h \
    oma2048env.h

SOURCES += \
    ../src/board.cpp \
    ../src/game.cpp \
    oma2048env.cpp
