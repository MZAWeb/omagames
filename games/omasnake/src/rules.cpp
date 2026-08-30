#include "rules.h"

#include <algorithm>

SpeedParams Rules::params(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Slow:
        return {12, 8, 6};
    case Difficulty::Fast:
        return {7, 4, 5};
    case Difficulty::Normal:
        break;
    }
    return {9, 5, 5};
}

int Rules::moveTicks(Difficulty difficulty, int foodsEaten) {
    const SpeedParams speed = params(difficulty);
    return std::max(speed.minMoveTicks, speed.startMoveTicks - foodsEaten / speed.foodsPerSpeedUp);
}
