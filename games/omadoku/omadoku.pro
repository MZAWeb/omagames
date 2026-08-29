include(../../common/common.pri)

CONFIG += c++17 release
TARGET = omadoku
TEMPLATE = app

HEADERS += \
    src/sudoku.h \
    src/sudokugenerator.h \
    src/sudokugrader.h

SOURCES += \
    src/main.cpp \
    src/sudoku.cpp \
    src/sudokugenerator.cpp \
    src/sudokugrader.cpp

RESOURCES += src/resources.qrc
