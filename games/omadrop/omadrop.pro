include(../../common/common.pri)

CONFIG += c++17 release
TARGET = omadrop
TEMPLATE = app

HEADERS += \
    src/dropengine.h \
    src/omadropgame.h \
    src/fieldview.h

SOURCES += \
    src/main.cpp \
    src/dropengine.cpp \
    src/omadropgame.cpp \
    src/fieldview.cpp

RESOURCES += src/resources.qrc
