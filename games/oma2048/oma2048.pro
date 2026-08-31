include(../../common/common.pri)

CONFIG += c++17 release
TARGET = oma2048
TEMPLATE = app

HEADERS += \
    src/board.h \
    src/game.h
SOURCES += \
    src/main.cpp \
    src/board.cpp \
    src/game.cpp
RESOURCES += src/resources.qrc
