#include "scoring.h"

#include <algorithm>

ClearInfo Scoring::award(int lines, Spin spin) {
    ClearInfo info;
    info.lines = lines;
    info.spin = spin;
    // The level in force when the piece locked is the one that pays.
    const int base = Rules::clearPoints(lines, spin);
    if (lines == 0) {
        m_combo = -1;
        info.points = base * m_level;
        m_score += info.points;
        return info;
    }
    ++m_combo;
    info.combo = m_combo;
    const bool difficult = Rules::isDifficult(lines, spin);
    info.backToBack = difficult && m_backToBack;
    int points = base;
    if (info.backToBack)
        points = points * Rules::kBackToBackNumerator / Rules::kBackToBackDenominator;
    points += Rules::kComboStep * m_combo;
    info.points = points * m_level;
    m_score += info.points;
    m_backToBack = difficult;
    m_lines += lines;
    m_level = std::max(m_level, Rules::kFirstLevel + m_lines / Rules::kLinesPerLevel);
    return info;
}
