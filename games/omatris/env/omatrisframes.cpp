// OmatrisEnv drawing a replay for a viewer with no engine of its own (a
// browser): the replay's calls played on a fresh game, and the well kept
// after the deal and after every piece that locks.
#include "omatrisenv.h"

#include <QJsonArray>

#include "calls.h"
#include "handling.h"
#include "modes.h"

namespace {

const auto kGame = QStringLiteral("omatris");

// A cell as one character: '.' for empty, else the piece that filled it.
QChar cell(PieceType type) {
    return type == PieceType::None ? QLatin1Char('.') : QLatin1Char("IJLOSTZ"[int(type)]);
}

QJsonObject frame(const Game &game, const PieceCells *placed) {
    QString board;
    board.reserve(Board::kCellCount);
    for (int i = 0; i < Board::kCellCount; ++i)
        board.append(cell(game.board().at(i)));
    QJsonArray queue;
    for (PieceType type : game.nextQueue())
        queue.append(QString(cell(type)));
    QJsonObject json {
        {QStringLiteral("board"), board},
        {QStringLiteral("score"), game.score()},
        {QStringLiteral("lines"), game.lines()},
        {QStringLiteral("level"), game.level()},
        {QStringLiteral("ticks"), game.ticks()},
        {QStringLiteral("queue"), queue},
        {QStringLiteral("hold"), QString(cell(game.heldPiece()))},
    };
    if (game.dealtRows() > 0) {
        json.insert(QStringLiteral("dealt_rows_left"), game.dealtRowsLeft());
        json.insert(QStringLiteral("difficulty"), game.difficulty());
    }
    // Where the piece that just locked went, for the viewer to light up.
    if (placed) {
        QJsonArray cells;
        for (QPoint p : *placed)
            cells.append(Board::index(p));
        json.insert(QStringLiteral("placed"), cells);
    }
    return json;
}

}  // namespace

std::optional<QJsonObject> OmatrisEnv::frames(const OmaGames::Replay &replay, QString *error) const {
    if (!replay.playableBy(kGame, kRulesVersion, error))
        return std::nullopt;
    Mode mode = Mode::Marathon;
    Modes::fromId(replay.config().value(QStringLiteral("mode")).toString(), &mode);
    Game game(mode, replay.seed());
    game.setSoftDropFactor(replay.config().value(QStringLiteral("soft_drop_factor")).toInt(Handling::kInstantSoftDrop));

    QJsonArray frames{frame(game, nullptr)};
    // Makes a call; when a piece locks, keeps the well as it is then.
    auto play = [&game, &frames](Call call) {
        const PieceCells before = game.piece().cells();
        for (const Event &event : Calls::apply(game, call)) {
            if (event.type == Event::Locked)
                frames.append(frame(game, &before));
        }
    };
    for (const OmaGames::Replay::Step &step : replay.steps()) {
        if (step.ticks > 0) {
            for (int i = 0; i < step.ticks && game.phase() == Phase::Playing; ++i)
                play(Call::Tick);
            continue;
        }
        const auto call = Calls::fromToken(step.input);
        if (!call) {
            *error = QStringLiteral("not an Omatris call: \"%1\"").arg(step.input);
            return std::nullopt;
        }
        play(*call);
    }
    return QJsonObject{
        {QStringLiteral("game"), kGame},
        {QStringLiteral("width"), Board::kWidth},
        {QStringLiteral("height"), Board::kHeight},
        {QStringLiteral("hidden_rows"), Board::kHiddenRows},
        {QStringLiteral("mode"), Modes::id(mode)},
        {QStringLiteral("agent"), replay.agent()},
        {QStringLiteral("ended"), game.phase() == Phase::Finished ? QStringLiteral("won")
                                  : game.phase() == Phase::GameOver ? QStringLiteral("topped out")
                                                                    : QStringLiteral("cut short")},
        {QStringLiteral("frames"), frames},
    };
}
