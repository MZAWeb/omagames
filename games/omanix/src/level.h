#pragma once

enum class Difficulty { Easy, Normal, Hard };
constexpr int kDifficultyCount = 3;

// Pace of one level: how many of each enemy, and how many ticks each entity
// waits between moves (smaller is faster). Speeds are integers on purpose so
// a level plays identically at any frame rate.
struct LevelParams {
    int balls;
    int ballPeriod;
    int chasers;
    int chaserPeriod;
    int playerPeriod;
};

namespace Level {

constexpr int kTicksPerSecond = 60;
constexpr int kFirstLevel = 1;
constexpr int kMaxBalls = 12;
constexpr int kMaxChasers = 6;

LevelParams params(Difficulty difficulty, int level);

}  // namespace Level
