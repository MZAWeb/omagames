# The agent environment layer (docs/AGENT-ENV.md). A game's env/env.pro
# includes this beside its engine sources and builds lib<game>_env.so; test
# suites include it to drive an Env headlessly. QtCore only, like an engine.
QT += core
INCLUDEPATH += $$PWD

QMAKE_CXXFLAGS += -isystem $$[QT_INSTALL_HEADERS]

HEADERS += \
    $$PWD/omagames_env.h \
    $$PWD/env.h \
    $$PWD/observationlayout.h \
    $$PWD/envconfig.h \
    $$PWD/replay.h

SOURCES += \
    $$PWD/envabi.cpp \
    $$PWD/observationlayout.cpp \
    $$PWD/envconfig.cpp \
    $$PWD/replay.cpp
