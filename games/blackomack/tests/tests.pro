include(../../../common/common-tests.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_blackomack

INCLUDEPATH += ../src
HEADERS += \
    ../src/cards.h \
    ../src/hand.h \
    ../src/blackjackrules.h \
    ../src/basicstrategy.h \
    ../src/botplayer.h \
    ../src/table.h
SOURCES += tst_blackomack.cpp \
    ../src/cards.cpp \
    ../src/hand.cpp \
    ../src/blackjackrules.cpp \
    ../src/basicstrategy.cpp \
    ../src/botplayer.cpp \
    ../src/table.cpp \
    ../src/tableround.cpp
