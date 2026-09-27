#include "preferences.h"

#include <QSettings>

#include "modes.h"

namespace {

const auto kModeKey = QStringLiteral("play/mode");
const auto kGhostKey = QStringLiteral("play/ghost");

}  // namespace

Preferences Preferences::load() {
    Preferences preferences;
    QSettings settings;
    Modes::fromId(settings.value(kModeKey).toString(), &preferences.m_mode);
    preferences.m_ghost = settings.value(kGhostKey, true).toBool();
    preferences.m_handling = Handling::load();
    return preferences;
}

void Preferences::setMode(Mode mode) {
    m_mode = mode;
    QSettings().setValue(kModeKey, Modes::id(mode));
}

void Preferences::setGhost(bool ghost) {
    m_ghost = ghost;
    QSettings().setValue(kGhostKey, ghost);
}

bool Preferences::setHandling(const Handling &handling) {
    if (handling == m_handling)
        return false;
    m_handling = handling;
    m_handling.save();
    return true;
}
