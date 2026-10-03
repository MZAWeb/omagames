#include "omatrisenv.h"

#include <QJsonArray>

#include "boardmetrics.h"
#include "handling.h"
#include "modes.h"

using OmaGames::EnvStep;

namespace {

const auto kGame = QStringLiteral("omatris");
const auto kSoftDropFactorKey = QStringLiteral("soft_drop_factor");

// In signalNames() order.
enum Signal { Score, Lines, Pieces, TSpin, Holes, MaxHeight, Bumpiness, ToppedOut };

QJsonObject choice(const QString &fallback, const QStringList &choices) {
    return {{QStringLiteral("default"), fallback}, {QStringLiteral("choices"), QJsonArray::fromStringList(choices)}};
}

QJsonObject range(int fallback, int min, int max) {
    return {{QStringLiteral("default"), fallback}, {QStringLiteral("min"), min}, {QStringLiteral("max"), max}};
}

QStringList modeIds() {
    QStringList ids;
    for (int i = 0; i < kModeCount; ++i)
        ids << Modes::id(Mode(i));
    return ids;
}

}  // namespace

QJsonObject OmaGames::envGameSpec() {
    return {
        {QStringLiteral("game"), kGame},
        {QStringLiteral("rules_version"), OmatrisEnv::kRulesVersion},
        {QStringLiteral("config"),
         QJsonObject{
             {QStringLiteral("mode"), choice(Modes::id(Mode::Marathon), modeIds())},
             {QStringLiteral("actions"),
              choice(QStringLiteral("placement"),
                     {QStringLiteral("placement"), QStringLiteral("drop"), QStringLiteral("raw")})},
             {QStringLiteral("hold"), QJsonObject{{QStringLiteral("default"), true}}},
             // How many landings the placement space shows. A piece rarely
             // has a hundred, and the afterstates are 240 bytes each.
             {QStringLiteral("candidates"), range(128, 1, 512)},
             {QStringLiteral("frame_skip"), range(1, 1, 60)},
             // Key presses a second when placing (placement, drop): 0 is
             // infinitely fast, so gravity never acts on a piece in motion;
             // 10 is a fast human, for whom high levels are hard.
             {QStringLiteral("input_rate"), range(0, 0, 60)},
         }},
    };
}

std::unique_ptr<OmaGames::Env> OmaGames::createEnv() {
    return std::make_unique<OmatrisEnv>();
}

void OmatrisEnv::configure(const QJsonObject &config) {
    m_config = config;
    Modes::fromId(config.value(QStringLiteral("mode")).toString(), &m_mode);
    const QString space = config.value(QStringLiteral("actions")).toString();
    m_space = space == QStringLiteral("raw") ? Space::Raw
            : space == QStringLiteral("drop") ? Space::Drop
                                              : Space::Placement;
    m_holdAllowed = config.value(QStringLiteral("hold")).toBool();
    m_candidates = config.value(QStringLiteral("candidates")).toInt();
    m_frameSkip = config.value(QStringLiteral("frame_skip")).toInt();
    const int rate = config.value(QStringLiteral("input_rate")).toInt();
    m_ticksPerInput = rate > 0 ? (Rules::kTicksPerSecond + rate / 2) / rate : 0;
    m_observation.emplace(m_space == Space::Placement ? m_candidates : 0);
}

int OmatrisEnv::actionCount() const {
    switch (m_space) {
    case Space::Placement:
        return m_candidates;
    case Space::Drop:
        return kDropActions;
    case Space::Raw:
        break;
    }
    return kRawActions;
}

QStringList OmatrisEnv::actionLabels() const {
    if (m_space == Space::Raw) {
        return {QStringLiteral("none"), QStringLiteral("left"), QStringLiteral("right"),
                QStringLiteral("rotate_cw"), QStringLiteral("rotate_ccw"), QStringLiteral("soft_drop"),
                QStringLiteral("hard_drop"), QStringLiteral("hold")};
    }
    if (m_space == Space::Placement)
        return {};
    QStringList labels;
    for (int action = 0; action < kDropActions; ++action) {
        const int column = action % Board::kWidth;
        const int rotation = action / Board::kWidth % Piece::kStates;
        labels << QStringLiteral("%1r%2c%3").arg(action >= kDropActions / 2 ? QStringLiteral("hold_") : QString())
                      .arg(rotation)
                      .arg(column);
    }
    return labels;
}

QStringList OmatrisEnv::signalNames() const {
    return {QStringLiteral("score"),     QStringLiteral("lines"),      QStringLiteral("pieces"),
            QStringLiteral("tspin"),     QStringLiteral("holes"),      QStringLiteral("max_height"),
            QStringLiteral("bumpiness"), QStringLiteral("topped_out")};
}

int OmatrisEnv::dropAction(bool hold, int rotation, int column) {
    return (hold ? kDropActions / 2 : 0) + rotation * Board::kWidth + column;
}

void OmatrisEnv::reset(quint32 seed) {
    m_game.emplace(m_mode, seed);
    m_game->setSoftDropFactor(Handling::kInstantSoftDrop);
    QJsonObject played = m_config;
    played.insert(kSoftDropFactorKey, Handling::kInstantSoftDrop);
    m_replay = OmaGames::Replay(kGame, kRulesVersion, played, seed);
    offerActions();
}

std::vector<Event> OmatrisEnv::call(Call made, int *ticks) {
    if (made == Call::Tick) {
        m_replay.advance(1);
        ++*ticks;
    } else {
        m_replay.input(Calls::token(made));
    }
    return Calls::apply(*m_game, made);
}

void OmatrisEnv::land(const Landing &landing, std::vector<Event> &events, int *ticks) {
    auto keep = [&events](std::vector<Event> more) { events.insert(events.end(), more.begin(), more.end()); };
    for (Call made : landing.calls)
        keep(call(made, ticks));
    keep(call(Call::HardDrop, ticks));
    // A clear flashes before the next piece appears; nothing is decided
    // during it, so it is part of the placement.
    while (m_game->phase() == Phase::Playing && !m_game->hasPiece())
        keep(call(Call::Tick, ticks));
}

EnvStep OmatrisEnv::step(int action) {
    const int score = m_game->score();
    const int lines = m_game->lines();
    std::vector<Event> events;
    int ticks = 0;
    auto keep = [&events](std::vector<Event> more) { events.insert(events.end(), more.begin(), more.end()); };

    switch (m_space) {
    case Space::Placement:
        land(m_landings[size_t(action)], events, &ticks);
        break;
    case Space::Drop:
        land(*m_drops[size_t(action)], events, &ticks);
        break;
    case Space::Raw: {
        static const Call kCalls[] = {Call::Tick, Call::Left, Call::Right, Call::RotateCW, Call::RotateCCW,
                                      Call::SoftDropOn, Call::HardDrop, Call::Hold};
        if (action != None)
            keep(call(kCalls[action], &ticks));
        for (int i = 0; i < m_frameSkip; ++i)
            keep(call(Call::Tick, &ticks));
        if (action == SoftDrop)
            keep(call(Call::SoftDropOff, &ticks));
        break;
    }
    }

    EnvStep result;
    result.terminated = m_game->phase() != Phase::Playing;
    result.reward = m_game->score() - score;
    result.ticks = ticks;
    auto &signal = result.signalValues;
    signal[Score] = result.reward;
    signal[Lines] = m_game->lines() - lines;
    for (const Event &event : events) {
        if (event.type == Event::Locked) {
            ++signal[Pieces];
            signal[TSpin] = std::max(signal[TSpin], double(event.clear.spin));
        }
        if (event.type == Event::TopOut)
            signal[ToppedOut] = 1;
    }
    const BoardMetrics metrics = BoardMetrics::of(m_game->board());
    signal[Holes] = metrics.holes;
    signal[MaxHeight] = metrics.maxHeight;
    signal[Bumpiness] = metrics.bumpiness;
    offerActions();
    return result;
}

void OmatrisEnv::offerActions() {
    m_landings.clear();
    m_drops.clear();
    m_unoffered = 0;
    if (m_space == Space::Placement) {
        m_landings = Placements::find(*m_game, m_holdAllowed, m_ticksPerInput);
        m_unoffered = std::max(0, int(m_landings.size()) - m_candidates);
        if (m_unoffered > 0)
            m_landings.resize(size_t(m_candidates));
    } else if (m_space == Space::Drop) {
        m_drops.resize(size_t(kDropActions));
        for (int hold = 0; hold < (m_holdAllowed ? 2 : 1); ++hold) {
            for (int rotation = 0; rotation < Piece::kStates; ++rotation) {
                for (int column = 0; column < Board::kWidth; ++column) {
                    m_drops[size_t(dropAction(hold, rotation, column))] =
                        Placements::drop(*m_game, hold, rotation, column, m_ticksPerInput);
                }
            }
        }
    }
}

void OmatrisEnv::observe(std::byte *buffer) const {
    m_observation->write(buffer, *m_game, m_landings);
}

void OmatrisEnv::actionMask(quint8 *mask) const {
    switch (m_space) {
    case Space::Placement:
        for (int i = 0; i < m_candidates; ++i)
            mask[i] = i < int(m_landings.size()) ? 1 : 0;
        break;
    case Space::Drop:
        for (int i = 0; i < kDropActions; ++i)
            mask[i] = m_drops[size_t(i)].has_value() ? 1 : 0;
        break;
    case Space::Raw:
        std::fill(mask, mask + kRawActions, quint8(1));
        mask[Hold] = m_holdAllowed && m_game->hasPiece() && m_game->holdAvailable() ? 1 : 0;
        break;
    }
}

std::unique_ptr<OmaGames::Env> OmatrisEnv::clone(bool reseedHidden, quint32 seed) const {
    auto copy = std::make_unique<OmatrisEnv>(*this);
    if (reseedHidden) {
        // The landings stay valid: they only ever reach the visible queue.
        // The replay does not: the seed no longer says what comes next.
        copy->m_game->reseedHidden(seed);
        copy->m_replay = OmaGames::Replay();
    }
    return copy;
}

QJsonObject OmatrisEnv::info() const {
    const QString phase = m_game->phase() == Phase::Playing ? QStringLiteral("playing")
                        : m_game->phase() == Phase::Finished ? QStringLiteral("finished")
                                                             : QStringLiteral("game_over");
    return {
        {QStringLiteral("mode"), Modes::id(m_mode)},
        {QStringLiteral("phase"), phase},
        {QStringLiteral("score"), m_game->score()},
        {QStringLiteral("lines"), m_game->lines()},
        {QStringLiteral("level"), m_game->level()},
        {QStringLiteral("ticks"), m_game->ticks()},
        {QStringLiteral("landings"), int(m_landings.size())},
        {QStringLiteral("landings_not_offered"), m_unoffered},
    };
}
