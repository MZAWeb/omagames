#pragma once

#include <QSettings>
#include <QString>
#include <QTemporaryDir>

#include "omatrisgame.h"

// What every bridge suite needs: a game that never runs a timer, and a config
// that is not the player's.
namespace BridgeFixture {

constexpr quint32 kSeed = 20260830u;

const auto kPlaying = QStringLiteral("playing");

// stepInterval 0 leaves the pacer stopped, so the test is the clock.
inline void quietStart(OmatrisGame &game, Mode mode) {
    game.setStepInterval(0);
    game.startGame(mode, kSeed);
}

// The whole binary writes to one throwaway directory, never ~/.config/Omacom.
inline QString redirectSettings() {
    static QTemporaryDir dir;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());
    return dir.isValid() ? dir.path() : QString();
}

inline void clearSettings() {
    QSettings settings;
    settings.clear();
    settings.sync();
}

}  // namespace BridgeFixture
