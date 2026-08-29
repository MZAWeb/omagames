include(../../../common/common-tests.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_omanix

INCLUDEPATH += ../src

HEADERS += \
    ../src/field.h \
    ../src/entities.h \
    ../src/level.h \
    ../src/game.h \
    enginetests.h

SOURCES += \
    ../src/field.cpp \
    ../src/entities.cpp \
    ../src/level.cpp \
    ../src/game.cpp \
    enginetests.cpp \
    tst_omanix.cpp
