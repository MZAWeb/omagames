include(../../common/common.pri)

CONFIG += c++17 release
TARGET = omanix
TEMPLATE = app

HEADERS += \
    src/field.h \
    src/entities.h \
    src/level.h \
    src/game.h \
    src/highscores.h \
    src/omanixgame.h \
    src/fieldview.h
SOURCES += \
    src/main.cpp \
    src/field.cpp \
    src/entities.cpp \
    src/level.cpp \
    src/game.cpp \
    src/highscores.cpp \
    src/omanixgame.cpp \
    src/fieldview.cpp
RESOURCES += src/resources.qrc
