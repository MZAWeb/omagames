include(../../common/common.pri)

CONFIG += c++17 release
TARGET = omadoku
TEMPLATE = app

HEADERS += \
    src/sudoku.h \
    src/sudokugenerator.h \
    src/sudokugrader.h \
    src/sudokuboard.h \
    src/cellmodel.h \
    src/sudokugame.h

SOURCES += \
    src/main.cpp \
    src/sudoku.cpp \
    src/sudokugenerator.cpp \
    src/sudokugrader.cpp \
    src/sudokuboard.cpp \
    src/cellmodel.cpp \
    src/sudokugame.cpp

RESOURCES += src/resources.qrc
