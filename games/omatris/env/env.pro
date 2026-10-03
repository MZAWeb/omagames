# libomatris_env.so: Omatris for an agent to play (docs/AGENT-ENV.md).
# Built by bin/build-env omatris; the engine's sources, never the bridge's.
include(../../../common/env/env.pri)

QT = core
CONFIG += c++17 release plugin
TEMPLATE = lib
TARGET = omatris_env

INCLUDEPATH += ../src ../../../common/src

HEADERS += \
    ../src/piece.h \
    ../src/board.h \
    ../src/bag.h \
    ../src/rules.h \
    ../src/modes.h \
    ../src/scoring.h \
    ../src/challenge.h \
    ../src/difficulty.h \
    ../src/dealtstack.h \
    ../src/lockdelay.h \
    ../src/game.h \
    ../src/handling.h \
    ../src/calls.h \
    ../src/boardmetrics.h \
    ../src/placements.h \
    ../../../common/src/scoretable.h \
    omatrisobservation.h \
    omatrisenv.h

SOURCES += \
    ../src/piece.cpp \
    ../src/board.cpp \
    ../src/bag.cpp \
    ../src/rules.cpp \
    ../src/modes.cpp \
    ../src/scoring.cpp \
    ../src/challenge.cpp \
    ../src/difficulty.cpp \
    ../src/dealtstack.cpp \
    ../src/lockdelay.cpp \
    ../src/game.cpp \
    ../src/handling.cpp \
    ../src/calls.cpp \
    ../src/boardmetrics.cpp \
    ../src/placements.cpp \
    ../../../common/src/scoretable.cpp \
    omatrisobservation.cpp \
    omatrisenv.cpp
