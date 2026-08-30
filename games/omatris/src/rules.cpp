#include "rules.h"

#include <algorithm>
#include <cmath>

ModeParams Rules::params(Mode mode) {
    switch (mode) {
    case Mode::Sprint:
        return {false, kSprintLines, true};
    case Mode::Zen:
        return {false, 0, false};
    case Mode::Marathon:
        break;
    }
    return {true, 0, false};
}

int Rules::gravityPerTick(int level) {
    // The guideline curve: one row takes (0.8 - 0.007 * (level - 1)) raised to
    // the (level - 1) seconds, one second at level 1 down to a fraction of a
    // frame at level 20, where it stops getting faster.
    const int clamped = std::clamp(level, kFirstLevel, kMaxGravityLevel);
    const double seconds = std::pow(0.8 - 0.007 * (clamped - 1), clamped - 1);
    return int(std::lround(kGravityUnit / (seconds * kTicksPerSecond)));
}

int Rules::clearPoints(int lines, Spin spin) {
    switch (spin) {
    case Spin::Mini:
        switch (lines) {
        case 0: return kTSpinMini;
        case 1: return kTSpinMiniSingle;
        default: return kTSpinMiniDouble;
        }
    case Spin::Full:
        switch (lines) {
        case 0: return kTSpin;
        case 1: return kTSpinSingle;
        case 2: return kTSpinDouble;
        default: return kTSpinTriple;
        }
    case Spin::None:
        break;
    }
    switch (lines) {
    case 1: return kSingle;
    case 2: return kDouble;
    case 3: return kTriple;
    case 4: return kTetris;
    default: return 0;
    }
}

bool Rules::isDifficult(int lines, Spin spin) {
    return lines > 0 && (lines == 4 || spin != Spin::None);
}
