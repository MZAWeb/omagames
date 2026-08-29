include(../../../common/common-tests.pri)
QT += testlib
CONFIG += testcase c++17
TEMPLATE = app
TARGET = tst_blackomack

INCLUDEPATH += ../src
SOURCES += tst_blackomack.cpp
# TODO(agent): add engine sources, e.g. ../src/sudoku.cpp
