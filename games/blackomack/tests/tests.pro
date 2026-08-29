include(../../../common/common-tests.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_blackomack

INCLUDEPATH += ../src
HEADERS += \
    ../src/cards.h \
    ../src/hand.h \
    ../src/blackjackrules.h
SOURCES += tst_blackomack.cpp \
    ../src/cards.cpp \
    ../src/hand.cpp \
    ../src/blackjackrules.cpp
