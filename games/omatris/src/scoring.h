#pragma once

#include "rules.h"

// What one locked piece earned. `combo` is the chain length minus one, so the
// first clear of a chain is 0 and pays no combo bonus; -1 means the placement
// broke the chain.
struct ClearInfo {
    int lines = 0;
    Spin spin = Spin::None;
    bool backToBack = false;
    int combo = -1;
    int points = 0;
};

// The running tally of a run: score, lines, level and the two chains a clear
// can extend. It knows nothing of pieces or the board; the game reports what
// a lock cleared and how far a piece was dropped, and this pays for it.
class Scoring {
public:
    int score() const { return m_score; }
    int lines() const { return m_lines; }
    int level() const { return m_level; }
    int combo() const { return m_combo; }
    bool backToBack() const { return m_backToBack; }

    // A lock that cleared `lines` (0 for none) with this spin.
    ClearInfo award(int lines, Spin spin);
    void addSoftDrop(int cells) { m_score += cells * Rules::kSoftDropPoints; }
    void addHardDrop(int cells) { m_score += cells * Rules::kHardDropPoints; }

private:
    int m_score = 0;
    int m_lines = 0;
    int m_level = Rules::kFirstLevel;
    int m_combo = -1;
    bool m_backToBack = false;
};
