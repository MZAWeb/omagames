include(../../common/common.pri)

CONFIG += c++17 release
TARGET = omatris
TEMPLATE = app

HEADERS += \
    src/piece.h \
    src/board.h \
    src/bag.h \
    src/rules.h \
    src/modes.h \
    src/bonuses.h \
    src/game.h \
    src/autoshift.h \
    src/omatrisgame.h \
    src/fieldview.h
SOURCES += \
    src/main.cpp \
    src/piece.cpp \
    src/board.cpp \
    src/bag.cpp \
    src/rules.cpp \
    src/modes.cpp \
    src/bonuses.cpp \
    src/game.cpp \
    src/autoshift.cpp \
    src/omatrisgame.cpp \
    src/fieldview.cpp
RESOURCES += src/resources.qrc
