include(../../common/common.pri)

CONFIG += c++17 release
TARGET = omasweeper
TEMPLATE = app

HEADERS += \
    src/presets.h \
    src/board.h \
    src/frontier.h \
    src/solver.h \
    src/generator.h \
    src/besttimes.h \
    src/omasweepergame.h
SOURCES += \
    src/main.cpp \
    src/board.cpp \
    src/frontier.cpp \
    src/solver.cpp \
    src/generator.cpp \
    src/besttimes.cpp \
    src/omasweepergame.cpp
RESOURCES += src/resources.qrc
