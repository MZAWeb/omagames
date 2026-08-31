include(../../../common/common-tests.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_oma2048

INCLUDEPATH += ../src

SOURCES += \
    tst_oma2048.cpp
