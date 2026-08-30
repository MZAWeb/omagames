#include "level.h"

#include <algorithm>

namespace {

// The ladder, in the units the engine ticks in: `*Period` is ticks per move
// at 60 ticks a second, so a smaller period is a faster mover and the marker's
// own 4 is the yardstick. Normal is meant to be a short step off Easy rather
// than most of the way to Hard: one more ball, one more chaser, one tick off
// the ball period, and a ramp every third level. Hard is where the pace
// really changes.
struct Ramp {
    int startBalls;
    int startBallPeriod;
    int minBallPeriod;
    int startChasers;
    int levelsPerSpeedUp;   // one tick shaved off the ball period every N levels
    int levelsPerChaser;    // one more chaser every N levels
    int chaserPeriod;
};

Ramp rampFor(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Easy:
        return {2, 9, 6, 1, 4, 5, 12};
    case Difficulty::Hard:
        return {4, 4, 3, 3, 2, 2, 7};
    case Difficulty::Normal:
        break;
    }
    return {3, 8, 5, 2, 3, 4, 11};
}

constexpr int kPlayerPeriod = 4;

}  // namespace

LevelParams Level::params(Difficulty difficulty, int level) {
    const Ramp ramp = rampFor(difficulty);
    const int steps = std::max(0, level - kFirstLevel);
    return {
        std::min(kMaxBalls, ramp.startBalls + steps),
        std::max(ramp.minBallPeriod, ramp.startBallPeriod - steps / ramp.levelsPerSpeedUp),
        std::min(kMaxChasers, ramp.startChasers + steps / ramp.levelsPerChaser),
        ramp.chaserPeriod,
        kPlayerPeriod,
    };
}
