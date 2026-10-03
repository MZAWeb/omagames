#pragma once

// How the walls behave and how fast the snake runs: the two choices a player
// makes on the start screen, and the speed ladder they pick between.
enum class Mode { Classic, Wrap };
constexpr int kModeCount = 2;

enum class Difficulty { Slow, Normal, Fast };
constexpr int kDifficultyCount = 3;

// Pace of one game. Speeds are ticks per move at 60 ticks a second, so a
// smaller period is a faster snake; integers on purpose, so a game plays
// identically whatever the timer manages.
struct SpeedParams {
    int startMoveTicks;
    int minMoveTicks;
    int foodsPerSpeedUp;
};

namespace Rules {

// Bumped whenever the same turns on the same seed would play out
// differently. Agents' replays and trained models carry it, so a stale one
// is refused rather than misread.
constexpr int kVersion = 1;

constexpr int kTicksPerSecond = 60;

SpeedParams params(Difficulty difficulty);
// Ticks between moves after `foodsEaten` foods.
int moveTicks(Difficulty difficulty, int foodsEaten);

}  // namespace Rules
