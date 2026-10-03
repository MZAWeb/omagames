#include "replayplayer.h"

#include <QFile>
#include <QJsonDocument>

#include <algorithm>

#include "modes.h"
#include "replay.h"

namespace {

const auto kGame = QStringLiteral("omatris");

// Calls that only change what the next tick does: they take no beat of
// their own whatever comes before them.
bool settingOnly(Call call) {
    return call == Call::SoftDropOn || call == Call::SoftDropOff;
}

}  // namespace

std::optional<ReplayPlayer> ReplayPlayer::load(const QJsonObject &json, QString *error) {
    const auto replay = OmaGames::Replay::fromJson(json, error);
    if (!replay)
        return std::nullopt;
    if (replay->game() != kGame) {
        *error = QStringLiteral("this is a replay of %1, not Omatris").arg(replay->game());
        return std::nullopt;
    }
    if (replay->rulesVersion() != Rules::kVersion) {
        *error = QStringLiteral("recorded under rules version %1; this Omatris plays version %2")
                     .arg(replay->rulesVersion())
                     .arg(Rules::kVersion);
        return std::nullopt;
    }
    ReplayPlayer player;
    const QString mode = replay->config().value(QStringLiteral("mode")).toString(Modes::id(Mode::Marathon));
    if (!Modes::fromId(mode, &player.m_mode)) {
        *error = QStringLiteral("unknown mode \"%1\"").arg(mode);
        return std::nullopt;
    }
    player.m_seed = replay->seed();
    player.m_softDropFactor = replay->config().value(QStringLiteral("soft_drop_factor")).toInt(Rules::kSoftDropFactor);
    player.m_agent = replay->agent();
    for (const OmaGames::Replay::Step &step : replay->steps()) {
        player.m_calls.insert(player.m_calls.end(), size_t(step.ticks), Call::Tick);
        if (step.input.isEmpty())
            continue;
        const auto call = Calls::fromToken(step.input);
        if (!call) {
            *error = QStringLiteral("unknown input \"%1\"").arg(step.input);
            return std::nullopt;
        }
        player.m_calls.push_back(*call);
    }
    return player;
}

std::optional<ReplayPlayer> ReplayPlayer::loadFile(const QString &path, QString *error) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        *error = QStringLiteral("cannot read %1: %2").arg(path, file.errorString());
        return std::nullopt;
    }
    QJsonParseError parse;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parse);
    if (!doc.isObject()) {
        *error = QStringLiteral("%1 is not JSON: %2").arg(path, parse.errorString());
        return std::nullopt;
    }
    return load(doc.object(), error);
}

Game ReplayPlayer::deal() const {
    Game game(m_mode, m_seed);
    game.setSoftDropFactor(m_softDropFactor);
    return game;
}

bool ReplayPlayer::isBeat(size_t index) const {
    const Call call = m_calls[index];
    if (call == Call::Tick)
        return true;
    if (settingOnly(call))
        return false;
    // A lock or a hold is what a viewer follows a game by: each gets a beat,
    // so the next piece is seen at the top before anything moves it.
    if (call == Call::HardDrop || call == Call::Hold)
        return true;
    // The first input after time has passed rides along with that tick's
    // frame; one straight after another input needs a frame of its own.
    for (size_t i = index; i-- > 0;) {
        if (m_calls[i] == Call::Tick)
            return false;
        if (!settingOnly(m_calls[i]))
            return true;
    }
    return false;
}

std::vector<Event> ReplayPlayer::beat(Game &game) {
    std::vector<Event> events;
    while (!done()) {
        const size_t index = m_next++;
        const std::vector<Event> made = Calls::apply(game, m_calls[index]);
        events.insert(events.end(), made.begin(), made.end());
        if (isBeat(index))
            break;
    }
    return events;
}

std::vector<Event> ReplayPlayer::nextPiece(Game &game) {
    std::vector<Event> events;
    const auto locked = [&events]() {
        return std::any_of(events.begin(), events.end(), [](const Event &e) { return e.type == Event::Locked; });
    };
    while (!done() && !locked()) {
        const std::vector<Event> made = beat(game);
        events.insert(events.end(), made.begin(), made.end());
    }
    return events;
}
