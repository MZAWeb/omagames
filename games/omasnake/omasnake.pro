include(../../common/common.pri)

CONFIG += c++17 release
TARGET = omasnake
TEMPLATE = app

HEADERS += \
    src/rules.h \
    src/snake.h \
    src/game.h
SOURCES += \
    src/main.cpp \
    src/rules.cpp \
    src/snake.cpp \
    src/game.cpp
RESOURCES += src/resources.qrc
