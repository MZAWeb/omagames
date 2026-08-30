include(../../common/common.pri)

CONFIG += c++17 release
TARGET = omasnake
TEMPLATE = app

HEADERS += \
    src/rules.h \
    src/choices.h \
    src/snake.h \
    src/game.h \
    src/omasnakegame.h \
    src/fieldview.h
SOURCES += \
    src/main.cpp \
    src/rules.cpp \
    src/choices.cpp \
    src/snake.cpp \
    src/game.cpp \
    src/omasnakegame.cpp \
    src/omasnakegamemodel.cpp \
    src/fieldview.cpp
RESOURCES += src/resources.qrc
