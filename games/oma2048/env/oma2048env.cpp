#include "oma2048env.h"

#include <QJsonArray>

#include <QtAlgorithms>

using OmaGames::EnvStep;
using OmaGames::ObservationLayout;

namespace {

const auto kGame = QStringLiteral("oma2048");
constexpr int kCells = Board::kSize * Board::kSize;
// The largest tile a 4x4 board can hold: 2^17.
constexpr int kMaxTile = 131072;

// In signalNames() order.
enum Signal { Score, Merges, Highest, Empty, Won };

// A move as a replay call.
const char *const kTokens[] = {"L", "R", "U", "D"};

// A tile as the observation numbers it: its power of two, 0 for none.
quint8 power(int value) {
    return value > 0 ? quint8(qCountTrailingZeroBits(quint32(value))) : 0;
}

void writeBoard(const Board &board, quint8 *out) {
    for (int row = 0; row < Board::kSize; ++row) {
        for (int col = 0; col < Board::kSize; ++col)
            out[row * Board::kSize + col] = power(board.valueAt(row, col));
    }
}

std::optional<Direction> fromToken(const QString &token) {
    for (int i = 0; i < Oma2048Env::kActions; ++i) {
        if (token == QLatin1String(kTokens[i]))
            return Direction(i);
    }
    return std::nullopt;
}

}  // namespace

QJsonObject OmaGames::envGameSpec() {
    return {
        {QStringLiteral("game"), kGame},
        {QStringLiteral("rules_version"), Oma2048Env::kRulesVersion},
        {QStringLiteral("config"),
         QJsonObject{
             // The tile that ends the run when it first appears; 0 plays on
             // until no slide is left.
             {QStringLiteral("goal"), QJsonObject{{QStringLiteral("default"), 0}, {QStringLiteral("min"), 0},
                                                  {QStringLiteral("max"), kMaxTile}}},
         }},
    };
}

std::unique_ptr<OmaGames::Env> OmaGames::createEnv() {
    return std::make_unique<Oma2048Env>();
}

void Oma2048Env::configure(const QJsonObject &config) {
    m_config = config;
    m_goal = config.value(QStringLiteral("goal")).toInt();
    m_layout = ObservationLayout();
    const int size = Board::kSize;
    m_board = m_layout.add(QStringLiteral("board"), ObservationLayout::DType::U8, {size, size});
    // Per action, the board the slide leaves before the spawn; all zero for
    // a slide that changes nothing (masked).
    m_afterstates = m_layout.add(QStringLiteral("afterstates"), ObservationLayout::DType::U8, {kActions, size, size});
    m_gains = m_layout.add(QStringLiteral("gains"), ObservationLayout::DType::I32, {kActions},
                           {QStringLiteral("left"), QStringLiteral("right"), QStringLiteral("up"), QStringLiteral("down")});
    m_stats = m_layout.add(QStringLiteral("stats"), ObservationLayout::DType::I32, {4},
                           {QStringLiteral("score"), QStringLiteral("highest"), QStringLiteral("empty"),
                            QStringLiteral("moves")});
}

QStringList Oma2048Env::actionLabels() const {
    return {QStringLiteral("left"), QStringLiteral("right"), QStringLiteral("up"), QStringLiteral("down")};
}

QStringList Oma2048Env::signalNames() const {
    return {QStringLiteral("score"), QStringLiteral("merges"), QStringLiteral("highest"), QStringLiteral("empty"),
            QStringLiteral("won")};
}

void Oma2048Env::reset(quint32 seed) {
    m_game.emplace(seed);
    m_game->newGame();
    m_moves = 0;
    m_replay = OmaGames::Replay(kGame, kRulesVersion, m_config, seed);
}

EnvStep Oma2048Env::step(int action) {
    const int score = m_game->score();
    const int tiles = int(m_game->board().tiles().size());
    const bool wonBefore = m_game->won();
    m_game->move(Direction(action));
    m_replay.input(QLatin1String(kTokens[action]));
    ++m_moves;

    EnvStep result;
    auto &signal = result.signalValues;
    result.reward = m_game->score() - score;
    result.terminated = m_game->over() || reachedGoal();
    signal[Score] = result.reward;
    // Each merge takes a tile away and the spawn adds one.
    signal[Merges] = tiles + 1 - int(m_game->board().tiles().size());
    signal[Highest] = m_game->board().highestValue();
    signal[Empty] = int(m_game->board().emptyCells().size());
    signal[Won] = m_game->won() && !wonBefore;
    return result;
}

void Oma2048Env::observe(std::byte *buffer) const {
    const Board &board = m_game->board();
    writeBoard(board, m_layout.at<quint8>(buffer, m_board));
    quint8 *after = m_layout.at<quint8>(buffer, m_afterstates);
    qint32 *gains = m_layout.at<qint32>(buffer, m_gains);
    for (int action = 0; action < kActions; ++action) {
        Board slid = board;
        const MoveResult moved = slid.move(Direction(action));
        if (!moved.moved)
            continue;
        writeBoard(slid, after + action * kCells);
        gains[action] = moved.scoreGained;
    }
    const qint32 stats[] = {m_game->score(), board.highestValue(), qint32(board.emptyCells().size()), m_moves};
    std::copy(std::begin(stats), std::end(stats), m_layout.at<qint32>(buffer, m_stats));
}

void Oma2048Env::actionMask(quint8 *mask) const {
    for (int action = 0; action < kActions; ++action)
        mask[action] = !m_game->over() && m_game->board().canMove(Direction(action)) ? 1 : 0;
}

std::unique_ptr<OmaGames::Env> Oma2048Env::clone(bool reseedHidden, quint32 seed) const {
    auto copy = std::make_unique<Oma2048Env>(*this);
    if (reseedHidden) {
        // Where the next tiles spawn is no longer the seed's to say.
        copy->m_game->reseedHidden(seed);
        copy->m_replay = OmaGames::Replay();
    }
    return copy;
}

QJsonObject Oma2048Env::info() const {
    const QString phase = reachedGoal() ? QStringLiteral("finished")
                        : m_game->over() ? QStringLiteral("game_over") : QStringLiteral("playing");
    return {
        {QStringLiteral("score"), m_game->score()},
        {QStringLiteral("highest"), m_game->board().highestValue()},
        {QStringLiteral("moves"), m_moves},
        {QStringLiteral("won"), m_game->won()},
        {QStringLiteral("phase"), phase},
    };
}

std::optional<QJsonObject> Oma2048Env::frames(const OmaGames::Replay &replay, QString *error) const {
    if (!replay.playableBy(kGame, kRulesVersion, error))
        return std::nullopt;
    Game game(replay.seed());
    game.newGame();
    auto frame = [&game](const QString &move) {
        QJsonArray cells;
        for (int row = 0; row < Board::kSize; ++row) {
            for (int col = 0; col < Board::kSize; ++col)
                cells.append(game.board().valueAt(row, col));
        }
        return QJsonObject{{QStringLiteral("board"), cells}, {QStringLiteral("score"), game.score()},
                           {QStringLiteral("highest"), game.board().highestValue()},
                           {QStringLiteral("move"), move}};
    };
    QJsonArray frames{frame(QString())};
    for (const OmaGames::Replay::Step &step : replay.steps()) {
        if (step.ticks > 0)
            continue;  // a 2048 replay has no time between moves
        const auto direction = fromToken(step.input);
        if (!direction) {
            *error = QStringLiteral("not an Oma2048 move: \"%1\"").arg(step.input);
            return std::nullopt;
        }
        game.move(*direction);
        frames.append(frame(step.input));
    }
    return QJsonObject{
        {QStringLiteral("game"), kGame},
        {QStringLiteral("size"), Board::kSize},
        {QStringLiteral("agent"), replay.agent()},
        {QStringLiteral("ended"), game.over() ? QStringLiteral("no moves left") : QStringLiteral("cut short")},
        {QStringLiteral("frames"), frames},
    };
}
