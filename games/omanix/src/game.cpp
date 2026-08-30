#include "game.h"

#include <algorithm>
#include <cstdlib>

namespace {

// Balls spawn away from the bottom edge, where the marker starts.
constexpr int kSpawnClearance = 10;

}  // namespace

Game::Game(Difficulty difficulty, quint32 seed)
    : m_difficulty(difficulty), m_rng(seed), m_params(Level::params(difficulty, Level::kFirstLevel)) {
    startLevel();
}

void Game::startLevel() {
    m_params = Level::params(m_difficulty, m_level);
    m_field.reset();
    m_levelTicks = 0;
    m_freezeTicks = 0;
    m_introTicks = kLevelIntroTicks;
    m_phase = Phase::Playing;
    spawnBalls();
    spawnChasers();
    respawnPlayer();
}

void Game::spawnBalls() {
    m_balls.clear();
    const int left = Field::kBorder;
    const int right = m_field.width() - Field::kBorder;
    const int top = Field::kBorder;
    const int bottom = m_field.height() - Field::kBorder - kSpawnClearance;
    while (int(m_balls.size()) < m_params.balls) {
        const QPoint pos {int(m_rng.bounded(left, right)), int(m_rng.bounded(top, bottom))};
        const bool taken = std::any_of(m_balls.begin(), m_balls.end(), [pos](const Ball &b) { return b.pos == pos; });
        if (taken)
            continue;
        const QPoint dir {m_rng.bounded(2) ? 1 : -1, m_rng.bounded(2) ? 1 : -1};
        m_balls.push_back({pos, dir});
    }
}

void Game::spawnChasers() {
    m_chasers.clear();
    // Spread along the top edge, the far side of the frame from where the
    // marker starts, all of them crawling clockwise.
    for (int i = 0; i < m_params.chasers; ++i) {
        const QPoint pos {int(m_rng.bounded(Field::kBorder, m_field.width() - Field::kBorder)), 0};
        m_chasers.push_back({pos, {1, 0}});
    }
}

// How far each claimed cell is from the nearest chaser, counted in the steps
// a chaser would actually take: chasers never leave the ground, so a cell
// across the sea is not near at all. Unreached cells come back as kUnreached.
std::vector<int> Game::chaserDistances() const {
    std::vector<int> distance(size_t(m_field.cellCount()), kUnreached);
    std::vector<int> queue;
    for (const Chaser &c : m_chasers) {
        if (!m_field.contains(c.pos) || m_field.at(c.pos) != Cell::Claimed)
            continue;
        const int start = m_field.index(c.pos);
        if (distance[size_t(start)] != kUnreached)
            continue;
        distance[size_t(start)] = 0;
        queue.push_back(start);
    }
    for (size_t head = 0; head < queue.size(); ++head) {
        const QPoint p = m_field.point(queue[head]);
        const int next = distance[size_t(queue[head])] + 1;
        for (QPoint step : {QPoint(1, 0), QPoint(0, 1), QPoint(-1, 0), QPoint(0, -1)}) {
            const QPoint n = p + step;
            if (!m_field.contains(n) || m_field.at(n) != Cell::Claimed)
                continue;
            const int index = m_field.index(n);
            if (distance[size_t(index)] != kUnreached)
                continue;
            distance[size_t(index)] = next;
            queue.push_back(index);
        }
    }
    return distance;
}

void Game::respawnPlayer() {
    // Bottom edge, on the cell the nearest chaser has the longest crawl to,
    // centre first on ties.
    const int y = m_field.height() - 1;
    const std::vector<int> distance = chaserDistances();
    const int centre = m_field.width() / 2;
    QPoint best {centre, y};
    int bestDistance = -1;
    for (int offset = 0; offset < m_field.width(); ++offset) {
        for (int x : {centre - offset, centre + offset}) {
            if (x < 0 || x >= m_field.width())
                continue;
            const int nearest = distance[size_t(m_field.index({x, y}))];
            if (nearest > bestDistance) {
                bestDistance = nearest;
                best = {x, y};
            }
        }
    }
    m_player = Player {};
    m_player.pos = best;
}

void Game::setDirection(Direction direction) {
    if (direction == Direction::None)
        return;
    m_player.held = direction;
    // Turning straight back onto the trail is never what the player meant.
    if (m_player.onTrail && direction == opposite(m_player.dir))
        return;
    m_player.dir = direction;
    m_player.queued = true;
}

void Game::releaseDirection(Direction direction) {
    if (m_player.held == direction)
        m_player.held = Direction::None;
}

void Game::setPaused(bool paused, std::vector<Event> *events) {
    if (m_paused == paused || m_phase != Phase::Playing)
        return;
    m_paused = paused;
    if (events)
        events->push_back({paused ? Event::Paused : Event::Resumed, {}, 0, 1, false, {}, {}});
}

void Game::nextLevel() {
    if (m_phase != Phase::LevelComplete)
        return;
    ++m_level;
    startLevel();
}

void Game::restartLevel() {
    if (m_phase != Phase::Playing)
        return;
    startLevel();
}

void Game::placePlayer(QPoint pos) {
    m_player = Player {};
    m_player.pos = pos;
}

void Game::placeBalls(const std::vector<Ball> &balls) {
    m_balls = balls;
}

void Game::placeChasers(const std::vector<Chaser> &chasers) {
    m_chasers = chasers;
}

void Game::addScore(int points, std::vector<Event> &events) {
    m_score += points;
    while (m_score >= m_nextExtraLife) {
        ++m_lives;
        m_nextExtraLife += kExtraLifeScore;
        events.push_back({Event::ExtraLife, {}, 0, 1, false, {}, {}});
    }
}

std::vector<Event> Game::tick() {
    std::vector<Event> events;
    if (m_phase != Phase::Playing || m_paused)
        return events;
    if (m_introTicks > 0) {
        --m_introTicks;
        return events;
    }
    ++m_ticks;
    ++m_levelTicks;
    if (m_freezeTicks > 0) {
        --m_freezeTicks;
        return events;
    }
    if (m_ticks % m_params.playerPeriod == 0)
        movePlayer(events);
    if (m_phase == Phase::Playing && m_ticks % m_params.ballPeriod == 0)
        moveBalls(events);
    if (m_phase == Phase::Playing && m_ticks % m_params.chaserPeriod == 0)
        moveChasers(events);
    return events;
}

void Game::movePlayer(std::vector<Event> &events) {
    Player &p = m_player;
    if (p.dir == Direction::None)
        return;
    if (!p.onTrail && !p.queued && p.held != p.dir) {
        p.dir = Direction::None;
        return;
    }
    p.queued = false;
    const QPoint target = p.pos + delta(p.dir);
    if (!m_field.contains(target)) {
        p.dir = Direction::None;
        return;
    }
    switch (m_field.at(target)) {
    case Cell::Trail:
        loseLife(LifeLostReason::SelfCross, events);
        return;
    case Cell::Open:
        if (!p.onTrail) {
            p.onTrail = true;
            events.push_back({Event::TrailStarted, {}, 0, 1, false, p.pos, {}});
        }
        m_field.set(target, Cell::Trail);
        p.pos = target;
        break;
    case Cell::Claimed:
        p.pos = target;
        if (p.onTrail) {
            p.onTrail = false;
            closeTrail(events);
        }
        break;
    }
    if (chaserAt(p.pos))
        loseLife(LifeLostReason::ChaserHit, events);
}

void Game::closeTrail(std::vector<Event> &events) {
    const std::vector<int> trail = m_field.trailCells();
    const bool closeCall = ballNearTrail(trail);

    std::vector<QPoint> ballPositions;
    for (const Ball &b : m_balls)
        ballPositions.push_back(b.pos);
    const std::vector<int> cells = m_field.claim(ballPositions);

    const double share = 100.0 * double(cells.size()) / m_field.interiorCells();
    int multiplier = 1;
    if (share >= kHugeCutPercent)
        multiplier = kHugeCutMultiplier;
    else if (share >= kBigCutPercent)
        multiplier = kBigCutMultiplier;
    const int points = int(cells.size()) * kCellScore * m_level * multiplier + (closeCall ? kCloseCallBonus : 0);

    Event claimed {Event::Claimed, cells, points, multiplier, closeCall, m_player.pos, {}};
    events.push_back(claimed);
    addScore(points, events);

    if (m_field.claimedPercent() >= kGoalPercent) {
        const int bonus = m_lives * kLifeBonus * m_level;
        m_lastLevel = {m_field.claimedPercent(), bonus, m_levelTicks};
        addScore(bonus, events);
        m_phase = Phase::LevelComplete;
        events.push_back({Event::LevelComplete, {}, bonus, 1, false, {}, {}});
    }
}

bool Game::trailThreatened() const {
    return m_player.onTrail && ballNearTrail(m_field.trailCells());
}

bool Game::ballNearTrail(const std::vector<int> &trail) const {
    for (const Ball &b : m_balls) {
        for (int i : trail) {
            const QPoint p = m_field.point(i);
            if (std::max(std::abs(p.x() - b.pos.x()), std::abs(p.y() - b.pos.y())) <= kCloseCallDistance)
                return true;
        }
    }
    return false;
}

void Game::moveBalls(std::vector<Event> &events) {
    for (Ball &b : m_balls) {
        b.step(m_field);
        if (m_field.at(b.pos) == Cell::Trail) {
            loseLife(LifeLostReason::BallHitTrail, events);
            return;
        }
    }
}

void Game::moveChasers(std::vector<Event> &events) {
    for (Chaser &c : m_chasers) {
        c.step(m_field, m_rng);
        if (c.pos == m_player.pos) {
            loseLife(LifeLostReason::ChaserHit, events);
            return;
        }
    }
}

bool Game::chaserAt(QPoint p) const {
    return std::any_of(m_chasers.begin(), m_chasers.end(), [p](const Chaser &c) { return c.pos == p; });
}

void Game::loseLife(LifeLostReason reason, std::vector<Event> &events) {
    std::vector<int> wiped = m_field.clearTrail();
    --m_lives;
    events.push_back({Event::LifeLost, std::move(wiped), 0, 1, false, m_player.pos, reason});
    if (m_lives <= 0) {
        m_phase = Phase::GameOver;
        events.push_back({Event::GameOver, {}, 0, 1, false, {}, {}});
        return;
    }
    respawnPlayer();
    m_freezeTicks = kRespawnTicks;
}
