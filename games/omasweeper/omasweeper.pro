include(../../common/common.pri)

CONFIG += c++17 release
TARGET = omasweeper
TEMPLATE = app

HEADERS += \
    src/presets.h
SOURCES += \
    src/main.cpp
RESOURCES += src/resources.qrc
