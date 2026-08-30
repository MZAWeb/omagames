#include "game.h"

#include <algorithm>

namespace {

// Off the field, so the first dot may land anywhere.
constexpr QPoint kNowhere {-1, -1};

int index(QPoint p) {
    return p.y() * Game::kWidth + p.x();
}

}  // namespace

Game::Game(Mode mode, Difficulty difficulty, quint32 seed)
    : m_mode(mode), m_difficulty(difficulty), m_rng(seed),
      m_snake({kWidth / 2, kHeight / 2}, kStartLength, Direction::Right), m_food(kNowhere), m_bonus(kNowhere) {
    spawn(&m_food);
}

bool Game::contains(QPoint p) {
    return p.x() >= 0 && p.y() >= 0 && p.x() < kWidth && p.y() < kHeight;
}

int Game::multiplier() const {
    return std::clamp(1 + (m_snake.length() - kStartLength) / kLengthPerMultiplier, 1, kMaxMultiplier);
}

double Game::bonusRemaining() const {
    return double(m_bonusTicks) / kBonusLifetimeTicks;
}

QPoint Game::wrapped(QPoint p) const {
    return {(p.x() + kWidth) % kWidth, (p.y() + kHeight) % kHeight};
}

std::vector<Event> Game::tick() {
    std::vector<Event> events;
    if (m_phase != Phase::Playing || m_paused)
        return events;
    if (m_readyTicks > 0) {
        --m_readyTicks;
        return events;
    }
    if (m_bonusTicks > 0 && --m_bonusTicks == 0)
        events.push_back({Event::BonusExpired, m_bonus});
    if (++m_moveCounter < moveTicks())
        return events;
    m_moveCounter = 0;
    move(events);
    return events;
}

void Game::move(std::vector<Event> &events) {
    m_snake.takeTurn();
    QPoint head = m_snake.nextCell();
    if (!contains(head)) {
        // Classic: the step that leaves the field is the step that ends the
        // game. No grace beat, on purpose.
        if (m_mode == Mode::Classic) {
            die(Death::Wall, events);
            return;
        }
        head = wrapped(head);
    }
    const bool onFood = head == m_food;
    const bool onBonus = hasBonus() && head == m_bonus;
    const bool grow = onFood || onBonus;
    // A snake that is not growing may move onto the cell its tail is leaving.
    if (m_snake.occupies(head, !grow)) {
        die(Death::Self, events);
        return;
    }
    m_snake.step(head, grow);
    if (grow)
        eat(onBonus, events);
}

void Game::eat(bool bonus, std::vector<Event> &events) {
    const QPoint at = m_snake.head();
    if (bonus) {
        m_score += kBonusScore;
        m_bonusTicks = 0;
        m_bonus = kNowhere;
        events.push_back({Event::BonusEaten, at, kBonusScore, 1});
        return;
    }

    const int wasMoveTicks = moveTicks();
    const int factor = multiplier();
    const int points = kFoodScore * factor;
    m_score += points;
    ++m_foodsEaten;
    events.push_back({Event::Ate, at, points, factor});
    if (moveTicks() != wasMoveTicks)
        events.push_back({Event::SpeedUp, at});

    if (!spawn(&m_food)) {
        die(Death::Filled, events);
        return;
    }
    if (m_foodsEaten % kFoodsPerBonus != 0)
        return;
    QPoint cell;
    if (!spawn(&cell))
        return;
    m_bonus = cell;
    m_bonusTicks = kBonusLifetimeTicks;
    events.push_back({Event::BonusSpawned, cell});
}

bool Game::spawn(QPoint *cell) {
    std::vector<bool> taken(size_t(kWidth * kHeight), false);
    for (const QPoint &p : m_snake.body())
        taken[size_t(index(p))] = true;
    if (contains(m_food))
        taken[size_t(index(m_food))] = true;
    if (hasBonus())
        taken[size_t(index(m_bonus))] = true;

    std::vector<QPoint> free;
    free.reserve(taken.size());
    for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < kWidth; ++x) {
            if (!taken[size_t(index({x, y}))])
                free.push_back({x, y});
        }
    }
    if (free.empty())
        return false;
    *cell = free[m_rng.bounded(quint32(free.size()))];
    return true;
}

void Game::die(Death death, std::vector<Event> &events) {
    m_phase = Phase::GameOver;
    m_death = death;
    m_bonusTicks = 0;
    events.push_back({Event::GameOver, m_snake.head()});
}

void Game::placeSnake(const std::deque<QPoint> &body, Direction heading) {
    m_snake.reset(body, heading);
}

void Game::placeBonus(QPoint bonus, int ticks) {
    m_bonus = bonus;
    m_bonusTicks = ticks;
}
