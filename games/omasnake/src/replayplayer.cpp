#include "replayplayer.h"

#include "choices.h"
#include "replay.h"

namespace {

const auto kGame = QStringLiteral("omasnake");

}  // namespace

std::optional<ReplayPlayer> ReplayPlayer::load(const QJsonObject &json, QString *error) {
    const auto replay = OmaGames::Replay::fromJson(json, error);
    if (!replay || !replay->playableBy(kGame, Rules::kVersion, error))
        return std::nullopt;
    ReplayPlayer player;
    const QString mode = replay->config().value(QStringLiteral("mode")).toString(Modes::id(Mode::Classic));
    if (!Modes::fromId(mode, &player.m_mode)) {
        *error = QStringLiteral("unknown walls \"%1\"").arg(mode);
        return std::nullopt;
    }
    const QString speed =
        replay->config().value(QStringLiteral("difficulty")).toString(Difficulties::id(Difficulty::Normal));
    if (!Difficulties::fromId(speed, &player.m_difficulty)) {
        *error = QStringLiteral("unknown speed \"%1\"").arg(speed);
        return std::nullopt;
    }
    player.m_seed = replay->seed();
    player.m_agent = replay->agent();
    for (const OmaGames::Replay::Step &step : replay->steps()) {
        player.m_calls.insert(player.m_calls.end(), size_t(step.ticks), std::nullopt);
        if (step.input.isEmpty())
            continue;
        const auto turn = directionFromToken(step.input);
        if (!turn) {
            *error = QStringLiteral("unknown turn \"%1\"").arg(step.input);
            return std::nullopt;
        }
        player.m_calls.push_back(turn);
    }
    return player;
}

std::optional<ReplayPlayer> ReplayPlayer::loadFile(const QString &path, QString *error) {
    const auto json = OmaGames::Replay::readFile(path, error);
    return json ? load(*json, error) : std::nullopt;
}

Game ReplayPlayer::deal() const {
    return Game(m_mode, m_difficulty, m_seed);
}

std::vector<Event> ReplayPlayer::tick(Game &game) {
    while (!done()) {
        const std::optional<Direction> call = m_calls[m_next++];
        if (!call)
            return game.tick();
        game.turn(*call);
    }
    return {};
}

std::vector<Event> ReplayPlayer::nextMove(Game &game) {
    std::vector<Event> events;
    const QPoint head = game.snake().head();
    while (!done() && game.phase() == Phase::Playing && game.snake().head() == head) {
        const std::vector<Event> made = tick(game);
        events.insert(events.end(), made.begin(), made.end());
    }
    return events;
}
