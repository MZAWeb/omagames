#pragma once

#include <QPoint>
#include <QRandomGenerator>
#include <vector>

#include "rules.h"
#include "snake.h"

enum class Phase { Playing, GameOver };

// Why a run ended. `Filled` is the perfect game: the snake covers the board
// and there is nowhere left to put a dot.
enum class Death { Wall, Self, Filled };

// One thing that happened during a tick, in order. `at` is the field cell it
// happened on, so a renderer can put a label there.
struct Event {
    enum Type { Ate, BonusSpawned, BonusEaten, BonusExpired, SpeedUp, GameOver };

    Type type;
    QPoint at;
    int points = 0;
    int multiplier = 1;
};

// The whole game as a deterministic state machine: every `tick()` advances
// one sixtieth of a second from an explicit seed and reports what happened.
// No timers, no Qt GUI: the bridge paces it and the tests drive it directly.
class Game {
public:
    static constexpr int kWidth = 32;
    static constexpr int kHeight = 24;
    static constexpr int kStartLength = 4;

    // Scoring. A food pays ten times the length multiplier, which climbs a
    // step every kLengthPerMultiplier cells the snake has grown; a bonus pays
    // a flat fifty whenever you reach it.
    static constexpr int kFoodScore = 10;
    static constexpr int kBonusScore = 50;
    static constexpr int kLengthPerMultiplier = 8;
    static constexpr int kMaxMultiplier = 5;

    // A bonus follows every fifth food and keeps for six seconds.
    static constexpr int kFoodsPerBonus = 5;
    static constexpr int kBonusLifetimeTicks = 6 * Rules::kTicksPerSecond;

    // Nothing moves for the first three quarters of a second, so the field is
    // read before the snake is committed to anything.
    static constexpr int kReadyTicks = Rules::kTicksPerSecond * 3 / 4;

    Game(Mode mode, Difficulty difficulty, quint32 seed);

    Phase phase() const { return m_phase; }
    Mode mode() const { return m_mode; }
    Difficulty difficulty() const { return m_difficulty; }
    Death death() const { return m_death; }
    bool paused() const { return m_paused; }
    // True while the opening beat holds the snake still.
    bool ready() const { return m_readyTicks > 0; }

    int score() const { return m_score; }
    int length() const { return m_snake.length(); }
    int foodsEaten() const { return m_foodsEaten; }
    int multiplier() const;
    int moveTicks() const { return Rules::moveTicks(m_difficulty, m_foodsEaten); }
    double cellsPerSecond() const { return double(Rules::kTicksPerSecond) / moveTicks(); }

    const Snake &snake() const { return m_snake; }
    QPoint food() const { return m_food; }
    bool hasBonus() const { return m_bonusTicks > 0; }
    QPoint bonus() const { return m_bonus; }
    // How much of the bonus's life is left, 1 down to 0.
    double bonusRemaining() const;

    static bool contains(QPoint p);

    void turn(Direction direction) { m_snake.turn(direction); }
    void setPaused(bool paused) { m_paused = paused; }

    std::vector<Event> tick();

    // Scenario hooks for tests: the rules never call these.
    void placeSnake(const std::deque<QPoint> &body, Direction heading);
    void placeFood(QPoint food) { m_food = food; }
    void placeBonus(QPoint bonus, int ticks);

private:
    QPoint wrapped(QPoint p) const;
    void move(std::vector<Event> &events);
    void eat(bool bonus, std::vector<Event> &events);
    // Puts a dot on a free cell; false when the board leaves none.
    bool spawn(QPoint *cell);
    void die(Death death, std::vector<Event> &events);

    Mode m_mode;
    Difficulty m_difficulty;
    QRandomGenerator m_rng;
    Snake m_snake;
    QPoint m_food;
    QPoint m_bonus;
    int m_bonusTicks = 0;
    Phase m_phase = Phase::Playing;
    Death m_death = Death::Wall;
    int m_score = 0;
    int m_foodsEaten = 0;
    int m_moveCounter = 0;
    int m_readyTicks = kReadyTicks;
    bool m_paused = false;
};
