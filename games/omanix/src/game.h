#pragma once

#include <QRandomGenerator>
#include <vector>

#include "entities.h"
#include "field.h"
#include "level.h"

enum class Phase { Playing, LevelComplete, GameOver };

enum class LifeLostReason { BallHitTrail, ChaserHit, SelfCross };

// One thing that happened during a tick, in order. `cells` lists the cells a
// claim won or a lost trail wiped, so a renderer can animate them.
struct Event {
    enum Type { TrailStarted, Claimed, LifeLost, LevelComplete, ExtraLife, GameOver, Paused, Resumed };

    Type type;
    std::vector<int> cells;
    int points = 0;
    int multiplier = 1;
    bool closeCall = false;
    QPoint at;
    LifeLostReason reason = LifeLostReason::BallHitTrail;
};

// What the level-complete overlay reports.
struct LevelStats {
    double percent = 0;
    int livesBonus = 0;
    int ticks = 0;
};

// The whole game as a deterministic state machine: every `tick()` advances
// one sixtieth of a second from an explicit seed and reports what happened.
// No timers, no Qt GUI: the bridge paces it and the tests drive it directly.
class Game {
public:
    // Scoring. A claim pays per cell times the level; cutting a big slice at
    // once multiplies it, closing with a ball breathing down the trail adds a
    // flat bonus, and each life still standing at the end of a level pays out.
    static constexpr int kCellScore = 1;
    static constexpr double kBigCutPercent = 10.0;
    static constexpr int kBigCutMultiplier = 2;
    static constexpr double kHugeCutPercent = 25.0;
    static constexpr int kHugeCutMultiplier = 4;
    static constexpr int kCloseCallDistance = 3;
    static constexpr int kCloseCallBonus = 500;
    static constexpr int kLifeBonus = 500;
    static constexpr double kGoalPercent = 75.0;
    static constexpr int kStartLives = 3;
    static constexpr int kExtraLifeScore = 10000;
    // Everything holds still for a moment after a lost life, and for a
    // little longer while a level announces itself.
    static constexpr int kRespawnTicks = Level::kTicksPerSecond;
    static constexpr int kLevelIntroTicks = Level::kTicksPerSecond * 6 / 5;

    Game(Difficulty difficulty, quint32 seed);

    Phase phase() const { return m_phase; }
    Difficulty difficulty() const { return m_difficulty; }
    int level() const { return m_level; }
    int score() const { return m_score; }
    int lives() const { return m_lives; }
    bool paused() const { return m_paused; }
    // True while the level intro holds everything still.
    bool inLevelIntro() const { return m_introTicks > 0; }
    // A ball is within kCloseCallDistance of the trail right now.
    bool trailThreatened() const;
    int levelTicks() const { return m_levelTicks; }
    double claimedPercent() const { return m_field.claimedPercent(); }
    const LevelStats &lastLevel() const { return m_lastLevel; }
    const LevelParams &params() const { return m_params; }

    const Field &field() const { return m_field; }
    const Player &player() const { return m_player; }
    const std::vector<Ball> &balls() const { return m_balls; }
    const std::vector<Chaser> &chasers() const { return m_chasers; }

    // Key down / key up. A tap moves one cell on claimed ground; holding keeps
    // moving; on open sea the marker never stops until it reaches ground.
    void setDirection(Direction direction);
    void releaseDirection(Direction direction);
    void setPaused(bool paused, std::vector<Event> *events = nullptr);
    // Next level after LevelComplete.
    void nextLevel();
    // Same level again from scratch, keeping score and lives.
    void restartLevel();

    std::vector<Event> tick();

    // Scenario hooks for tests: the rules never call these.
    Field &mutableField() { return m_field; }
    void placePlayer(QPoint pos);
    void placeBalls(const std::vector<Ball> &balls);
    void placeChasers(const std::vector<Chaser> &chasers);
    void addScore(int points, std::vector<Event> &events);

private:
    void startLevel();
    void respawnPlayer();
    void movePlayer(std::vector<Event> &events);
    void moveBalls(std::vector<Event> &events);
    void moveChasers(std::vector<Event> &events);
    void closeTrail(std::vector<Event> &events);
    void loseLife(LifeLostReason reason, std::vector<Event> &events);
    bool chaserAt(QPoint p) const;
    bool anyBallNearTrail() const;

    Difficulty m_difficulty;
    QRandomGenerator m_rng;
    Field m_field;
    Player m_player;
    std::vector<Ball> m_balls;
    std::vector<Chaser> m_chasers;
    LevelParams m_params;
    LevelStats m_lastLevel;
    Phase m_phase = Phase::Playing;
    int m_level = Level::kFirstLevel;
    int m_score = 0;
    int m_lives = kStartLives;
    int m_nextExtraLife = kExtraLifeScore;
    int m_ticks = 0;
    int m_levelTicks = 0;
    int m_freezeTicks = 0;
    int m_introTicks = 0;
    bool m_paused = false;
};
