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
    src/omasweepergame.h \
    src/fieldview.h
SOURCES += \
    src/main.cpp \
    src/presets.cpp \
    src/board.cpp \
    src/frontier.cpp \
    src/solver.cpp \
    src/generator.cpp \
    src/omasweepergame.cpp \
    src/fieldview.cpp
RESOURCES += src/resources.qrc
