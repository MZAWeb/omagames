#pragma once

#include "piece.h"

// Marathon ramps forever, Sprint races forty lines against the clock, Zen
// never speeds up.
enum class Mode : quint8 { Marathon, Sprint, Zen };
constexpr int kModeCount = 3;

// How a mode differs: whether gravity follows the level, how many lines end
// the run, and whether its table ranks by the clock or by the score.
struct ModeParams {
    bool gravityRamps;
    int lineGoal;  // 0 = endless
    bool rankByTime;
};

namespace Rules {

constexpr int kTicksPerSecond = 60;
// Gravity accumulates in millionths of a cell per tick, so a level falls at
// the same speed whatever a frame happens to cost.
constexpr int kGravityUnit = 1000000;
constexpr int kFirstLevel = 1;
constexpr int kLinesPerLevel = 10;
// Past this the guideline curve is already faster than one cell per frame.
constexpr int kMaxGravityLevel = 20;
constexpr int kSoftDropFactor = 20;

constexpr int kLockDelayMs = 500;
constexpr int kLockDelayTicks = kLockDelayMs * kTicksPerSecond / 1000;
constexpr int kMaxLockResets = 15;
constexpr int kClearDelayMs = 150;
constexpr int kClearDelayTicks = kClearDelayMs * kTicksPerSecond / 1000;

constexpr int kNextQueue = 3;
constexpr int kSprintLines = 40;

// Guideline scoring; every one of these is multiplied by the level.
constexpr int kSingle = 100;
constexpr int kDouble = 300;
constexpr int kTriple = 500;
constexpr int kTetris = 800;
constexpr int kTSpinMini = 100;
constexpr int kTSpinMiniSingle = 200;
constexpr int kTSpinMiniDouble = 400;
constexpr int kTSpin = 400;
constexpr int kTSpinSingle = 800;
constexpr int kTSpinDouble = 1200;
constexpr int kTSpinTriple = 1600;
constexpr int kComboStep = 50;
constexpr int kSoftDropPoints = 1;
constexpr int kHardDropPoints = 2;
// A difficult clear straight after another one pays half as much again.
constexpr int kBackToBackNumerator = 3;
constexpr int kBackToBackDenominator = 2;

ModeParams params(Mode mode);
// Millionths of a cell per tick at this level.
int gravityPerTick(int level);
// Points for one clear before the level multiplier and the combo bonus.
int clearPoints(int lines, Spin spin);
// Tetrises and T-spins with lines are what keeps a back-to-back chain alive.
bool isDifficult(int lines, Spin spin);

}  // namespace Rules
