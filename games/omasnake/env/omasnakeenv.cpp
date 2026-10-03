#include "omasnakeenv.h"

#include <QJsonArray>

#include <cmath>
#include <cstdlib>

#include "choices.h"

using OmaGames::EnvStep;
using OmaGames::ObservationLayout;

namespace {

const auto kGame = QStringLiteral("omasnake");

// What a grid cell holds, as the observation numbers it.
enum GridCode : quint8 { Empty, Body, Head, Tail, Food, Bonus };

// In signalNames() order.
enum Signal { Score, Ate, BonusEaten, Length, FoodDistance, Died, Filled };

QJsonObject choice(const QString &fallback, const QStringList &choices) {
    return {{QStringLiteral("default"), fallback}, {QStringLiteral("choices"), QJsonArray::fromStringList(choices)}};
}

QStringList modeIds() {
    QStringList ids;
    for (const ModeInfo &info : Modes::all())
        ids << info.id;
    return ids;
}

QStringList difficultyIds() {
    QStringList ids;
    for (const DifficultyInfo &info : Difficulties::all())
        ids << info.id;
    return ids;
}

// Steps from the head to the food, across an edge when the edges wrap.
int distance(const Game &game) {
    int dx = std::abs(game.snake().head().x() - game.food().x());
    int dy = std::abs(game.snake().head().y() - game.food().y());
    if (game.mode() == Mode::Wrap) {
        dx = std::min(dx, Game::kWidth - dx);
        dy = std::min(dy, Game::kHeight - dy);
    }
    return dx + dy;
}

Direction turnedLeft(Direction heading) {
    switch (heading) {
    case Direction::Up:
        return Direction::Left;
    case Direction::Left:
        return Direction::Down;
    case Direction::Down:
        return Direction::Right;
    case Direction::Right:
        break;
    }
    return Direction::Up;
}

}  // namespace

QJsonObject OmaGames::envGameSpec() {
    return {
        {QStringLiteral("game"), kGame},
        {QStringLiteral("rules_version"), OmasnakeEnv::kRulesVersion},
        {QStringLiteral("config"),
         QJsonObject{
             {QStringLiteral("mode"), choice(Modes::id(Mode::Classic), modeIds())},
             {QStringLiteral("difficulty"), choice(Difficulties::id(Difficulty::Normal), difficultyIds())},
             {QStringLiteral("actions"),
              choice(QStringLiteral("relative"), {QStringLiteral("relative"), QStringLiteral("absolute")})},
         }},
    };
}

std::unique_ptr<OmaGames::Env> OmaGames::createEnv() {
    return std::make_unique<OmasnakeEnv>();
}

void OmasnakeEnv::configure(const QJsonObject &config) {
    m_config = config;
    Modes::fromId(config.value(QStringLiteral("mode")).toString(), &m_mode);
    Difficulties::fromId(config.value(QStringLiteral("difficulty")).toString(), &m_difficulty);
    m_relative = config.value(QStringLiteral("actions")).toString() == QStringLiteral("relative");
    m_layout = ObservationLayout();
    m_grid = m_layout.add(QStringLiteral("grid"), ObservationLayout::DType::U8, {Game::kHeight, Game::kWidth});
    m_state = m_layout.add(QStringLiteral("state"), ObservationLayout::DType::I32, {12},
                           {QStringLiteral("head_x"), QStringLiteral("head_y"), QStringLiteral("heading"),
                            QStringLiteral("length"), QStringLiteral("food_x"), QStringLiteral("food_y"),
                            QStringLiteral("bonus_x"), QStringLiteral("bonus_y"), QStringLiteral("bonus_ticks"),
                            QStringLiteral("score"), QStringLiteral("multiplier"), QStringLiteral("move_ticks")});
}

QStringList OmasnakeEnv::actionLabels() const {
    if (m_relative)
        return {QStringLiteral("straight"), QStringLiteral("turn_left"), QStringLiteral("turn_right")};
    return {QStringLiteral("up"), QStringLiteral("down"), QStringLiteral("left"), QStringLiteral("right")};
}

QStringList OmasnakeEnv::signalNames() const {
    return {QStringLiteral("score"),         QStringLiteral("ate"),  QStringLiteral("bonus"),
            QStringLiteral("length"),        QStringLiteral("food_distance"),
            QStringLiteral("died"),          QStringLiteral("filled")};
}

Direction OmasnakeEnv::headingFor(int action) const {
    if (!m_relative)
        return Direction(action);
    const Direction heading = m_game->snake().heading();
    switch (action) {
    case TurnLeft:
        return turnedLeft(heading);
    case TurnRight:
        return opposite(turnedLeft(heading));
    default:
        return heading;
    }
}

void OmasnakeEnv::reset(quint32 seed) {
    m_game.emplace(m_mode, m_difficulty, seed);
    m_replay = OmaGames::Replay(kGame, kRulesVersion, m_config, seed);
}

EnvStep OmasnakeEnv::step(int action) {
    const Direction wanted = headingFor(action);
    if (wanted != m_game->snake().heading()) {
        m_game->turn(wanted);
        m_replay.input(directionToken(wanted));
    }
    const int score = m_game->score();
    const QPoint head = m_game->snake().head();
    EnvStep result;
    auto &signal = result.signalValues;
    // Until the head has moved: whatever ticks the speed and the opening
    // beat put between this decision and the next.
    while (m_game->phase() == Phase::Playing && m_game->snake().head() == head) {
        for (const Event &event : m_game->tick()) {
            if (event.type == Event::Ate)
                ++signal[Ate];
            else if (event.type == Event::BonusEaten)
                ++signal[BonusEaten];
        }
        m_replay.advance(1);
        ++result.ticks;
    }
    result.terminated = m_game->phase() == Phase::GameOver;
    result.reward = m_game->score() - score;
    signal[Score] = result.reward;
    signal[Length] = m_game->length();
    signal[FoodDistance] = result.terminated ? 0 : distance(*m_game);
    signal[Died] = result.terminated && m_game->death() != Death::Filled;
    signal[Filled] = result.terminated && m_game->death() == Death::Filled;
    return result;
}

void OmasnakeEnv::observe(std::byte *buffer) const {
    quint8 *grid = m_layout.at<quint8>(buffer, m_grid);
    auto put = [grid](QPoint p, GridCode code) {
        if (Game::contains(p))
            grid[p.y() * Game::kWidth + p.x()] = code;
    };
    put(m_game->food(), Food);
    if (m_game->hasBonus())
        put(m_game->bonus(), Bonus);
    for (const QPoint &cell : m_game->snake().body())
        put(cell, Body);
    put(m_game->snake().tail(), Tail);
    put(m_game->snake().head(), Head);

    const bool bonus = m_game->hasBonus();
    const qint32 state[] = {
        m_game->snake().head().x(),
        m_game->snake().head().y(),
        int(m_game->snake().heading()),
        m_game->length(),
        m_game->food().x(),
        m_game->food().y(),
        bonus ? m_game->bonus().x() : -1,
        bonus ? m_game->bonus().y() : -1,
        bonus ? int(std::lround(m_game->bonusRemaining() * Game::kBonusLifetimeTicks)) : 0,
        m_game->score(),
        m_game->multiplier(),
        m_game->moveTicks(),
    };
    std::copy(std::begin(state), std::end(state), m_layout.at<qint32>(buffer, m_state));
}

void OmasnakeEnv::actionMask(quint8 *mask) const {
    std::fill(mask, mask + actionCount(), quint8(1));
    // Turning back onto the neck is no move at all: the snake ignores it.
    if (!m_relative)
        mask[int(opposite(m_game->snake().heading()))] = 0;
}

std::unique_ptr<OmaGames::Env> OmasnakeEnv::clone(bool reseedHidden, quint32 seed) const {
    auto copy = std::make_unique<OmasnakeEnv>(*this);
    if (reseedHidden) {
        // Where the dots land next is no longer the seed's to say.
        copy->m_game->reseedHidden(seed);
        copy->m_replay = OmaGames::Replay();
    }
    return copy;
}

QJsonObject OmasnakeEnv::info() const {
    static const QString kDeaths[] = {QStringLiteral("wall"), QStringLiteral("self"), QStringLiteral("filled")};
    QJsonObject json{
        {QStringLiteral("mode"), Modes::id(m_mode)},
        {QStringLiteral("difficulty"), Difficulties::id(m_difficulty)},
        {QStringLiteral("score"), m_game->score()},
        {QStringLiteral("length"), m_game->length()},
        {QStringLiteral("foods"), m_game->foodsEaten()},
        {QStringLiteral("phase"),
         m_game->phase() == Phase::Playing ? QStringLiteral("playing") : QStringLiteral("game_over")},
    };
    if (m_game->phase() == Phase::GameOver)
        json.insert(QStringLiteral("death"), kDeaths[int(m_game->death())]);
    return json;
}
