include(../../common/common.pri)

CONFIG += c++17 release
TARGET = oma2048
TEMPLATE = app

HEADERS += \
    src/board.h \
    src/game.h \
    src/oma2048game.h
SOURCES += \
    src/main.cpp \
    src/board.cpp \
    src/game.cpp \
    src/oma2048game.cpp \
    src/oma2048gamemodel.cpp
RESOURCES += src/resources.qrc
