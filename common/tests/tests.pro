include(../common-tests.pri)
include(../env/env.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_common

HEADERS += \
    themetests.h \
    scoretabletests.h \
    windowgeometrytests.h \
    pacertests.h \
    walkenv.h \
    envpartstests.h \
    envabitests.h

SOURCES += \
    themetests.cpp \
    scoretabletests.cpp \
    windowgeometrytests.cpp \
    pacertests.cpp \
    walkenv.cpp \
    envpartstests.cpp \
    envabitests.cpp \
    tst_common.cpp
