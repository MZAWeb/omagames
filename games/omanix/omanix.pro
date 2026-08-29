include(../../common/common.pri)

CONFIG += c++17 release
TARGET = omanix
TEMPLATE = app

HEADERS += \
    src/field.h \
    src/entities.h \
    src/level.h \
    src/game.h
SOURCES += \
    src/main.cpp \
    src/field.cpp \
    src/entities.cpp \
    src/level.cpp \
    src/game.cpp
RESOURCES += src/resources.qrc
