include(../../common/common.pri)

CONFIG += c++17 release
TARGET = blackomack
TEMPLATE = app

HEADERS += \
    src/cards.h \
    src/hand.h \
    src/blackjackrules.h \
    src/basicstrategy.h \
    src/coach.h \
    src/botplayer.h \
    src/table.h \
    src/tablelog.h \
    src/gamestate.h \
    src/sessionstats.h \
    src/seatlayout.h \
    src/blackjackgame.h
SOURCES += \
    src/main.cpp \
    src/cards.cpp \
    src/hand.cpp \
    src/blackjackrules.cpp \
    src/basicstrategy.cpp \
    src/coach.cpp \
    src/botplayer.cpp \
    src/table.cpp \
    src/tableround.cpp \
    src/tablelog.cpp \
    src/gamestate.cpp \
    src/sessionstats.cpp \
    src/seatlayout.cpp \
    src/blackjackgame.cpp \
    src/blackjackgamestore.cpp \
    src/blackjackgamemodel.cpp
RESOURCES += src/resources.qrc
