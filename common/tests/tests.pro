include(../common-tests.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_common

HEADERS += \
    themetests.h \
    scoretabletests.h \
    windowgeometrytests.h \
    pacertests.h

SOURCES += \
    themetests.cpp \
    scoretabletests.cpp \
    windowgeometrytests.cpp \
    pacertests.cpp \
    tst_common.cpp
