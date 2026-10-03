include(../../common/common.pri)

CONFIG += c++17 release
TARGET = omasnake
TEMPLATE = app

# replay/v1 and its pacing, the pieces of the agent environment the app reads.
INCLUDEPATH += ../../common/env

HEADERS += \
    src/rules.h \
    src/choices.h \
    src/snake.h \
    src/game.h \
    src/replayplayer.h \
    ../../common/env/replay.h \
    ../../common/env/replaypace.h \
    ../../common/env/replayplaylist.h \
    src/omasnakegame.h \
    src/fieldview.h
SOURCES += \
    src/main.cpp \
    src/rules.cpp \
    src/choices.cpp \
    src/snake.cpp \
    src/game.cpp \
    src/replayplayer.cpp \
    ../../common/env/replay.cpp \
    ../../common/env/replaypace.cpp \
    ../../common/env/replayplaylist.cpp \
    src/omasnakegame.cpp \
    src/omasnakegamemodel.cpp \
    src/omasnakegamereplay.cpp \
    src/fieldview.cpp
RESOURCES += src/resources.qrc
