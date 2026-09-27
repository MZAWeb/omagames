include(../../../common/common-tests.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_omadrop

INCLUDEPATH += ../src

HEADERS += \
    ../src/dropengine.h \
    ../src/omadropgame.h

SOURCES += \
    ../src/dropengine.cpp \
    ../src/omadropgame.cpp \
    tst_omadrop.cpp
