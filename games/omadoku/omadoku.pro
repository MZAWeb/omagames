include(../../common/common.pri)

CONFIG += c++17 release
TARGET = omadoku
TEMPLATE = app

HEADERS += \
    src/sudoku.h

SOURCES += \
    src/main.cpp \
    src/sudoku.cpp

RESOURCES += src/resources.qrc
