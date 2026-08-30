include(../../common/common.pri)

CONFIG += c++17 release
TARGET = omasweeper
TEMPLATE = app

HEADERS += \
    src/presets.h \
    src/board.h
SOURCES += \
    src/main.cpp \
    src/board.cpp
RESOURCES += src/resources.qrc
