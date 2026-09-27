#pragma once

#include "handling.h"
#include "rules.h"

// What Omatris remembers between launches besides the scores and the window:
// the mode chosen last, whether the ghost shows, and the handling. Each
// setter writes through to QSettings at once, so a crash loses nothing.
class Preferences {
public:
    static Preferences load();

    Mode mode() const { return m_mode; }
    bool ghost() const { return m_ghost; }
    const Handling &handling() const { return m_handling; }

    void setMode(Mode mode);
    void setGhost(bool ghost);
    // False, and nothing written, when `handling` is what is already kept.
    bool setHandling(const Handling &handling);

private:
    Mode m_mode = Mode::Marathon;
    bool m_ghost = true;
    Handling m_handling = Handling::defaults();
};
