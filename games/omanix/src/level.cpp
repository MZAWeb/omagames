#include "level.h"

#include <algorithm>

namespace {

struct Ramp {
    int startBalls;
    int startBallPeriod;
    int minBallPeriod;
    int levelsPerSpeedUp;   // one tick shaved off the ball period every N levels
    int levelsPerChaser;    // one more chaser every N levels
};

Ramp rampFor(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Easy:
        return {2, 8, 5, 3, 5};
    case Difficulty::Hard:
        return {4, 5, 3, 2, 2};
    case Difficulty::Normal:
        break;
    }
    return {3, 6, 4, 2, 3};
}

constexpr int kPlayerPeriod = 4;
constexpr int kChaserPeriod = 6;

}  // namespace

LevelParams Level::params(Difficulty difficulty, int level) {
    const Ramp ramp = rampFor(difficulty);
    const int steps = std::max(0, level - kFirstLevel);
    return {
        std::min(kMaxBalls, ramp.startBalls + steps),
        std::max(ramp.minBallPeriod, ramp.startBallPeriod - steps / ramp.levelsPerSpeedUp),
        std::min(kMaxChasers, 1 + steps / ramp.levelsPerChaser),
        kChaserPeriod,
        kPlayerPeriod,
    };
}
