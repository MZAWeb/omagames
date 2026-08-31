#include "game.h"

#include <QJsonArray>

Game::Game(quint32 seed) : m_rng(seed) {}

void Game::newGame() {
    m_board.clear();
    m_score = 0;
    m_won = false;
    m_keepPlaying = false;
    m_over = false;
    m_undo.reset();
    spawn();
    spawn();
}

bool Game::move(Direction direction) {
    if (m_over)
        return false;
    const Snapshot before{m_board, m_score, m_won, m_keepPlaying};
    const MoveResult result = m_board.move(direction);
    if (!result.moved)
        return false;
    m_undo = before;
    m_score += result.scoreGained;
    if (!m_won && m_board.highestValue() >= kWinValue)
        m_won = true;
    spawn();
    m_over = !m_board.anyMoveAvailable();
    return true;
}

bool Game::undo() {
    if (!canUndo())
        return false;
    m_board = m_undo->board;
    m_score = m_undo->score;
    m_won = m_undo->won;
    m_keepPlaying = m_undo->keepPlaying;
    m_undo.reset();
    return true;
}

QJsonObject Game::toJson() const {
    QJsonArray tiles;
    for (const Tile &tile : m_board.tiles()) {
        tiles.append(QJsonObject{{QStringLiteral("row"), tile.row},
                                 {QStringLiteral("col"), tile.col},
                                 {QStringLiteral("value"), tile.value}});
    }
    return {{QStringLiteral("tiles"), tiles},
            {QStringLiteral("score"), m_score},
            {QStringLiteral("won"), m_won}};
}

bool Game::restore(const QJsonObject &json) {
    const QJsonValue tilesValue = json.value(QLatin1String("tiles"));
    const int score = json.value(QLatin1String("score")).toInt(-1);
    if (!tilesValue.isArray() || score < 0)
        return false;
    Board board;
    const QJsonArray tiles = tilesValue.toArray();
    for (const QJsonValue &entry : tiles) {
        const QJsonObject tile = entry.toObject();
        const int row = tile.value(QLatin1String("row")).toInt(-1);
        const int col = tile.value(QLatin1String("col")).toInt(-1);
        const int value = tile.value(QLatin1String("value")).toInt(0);
        if (row < 0 || row >= Board::kSize || col < 0 || col >= Board::kSize)
            return false;
        if (value < 2 || (value & (value - 1)) != 0)
            return false;
        if (board.valueAt(row, col) != 0)
            return false;
        board.put(row, col, value);
    }
    m_board = board;
    m_score = score;
    m_won = json.value(QLatin1String("won")).toBool(false);
    // The win overlay was already shown before the relaunch, so a restored
    // winning run goes straight back to playing.
    m_keepPlaying = m_won;
    m_over = !m_board.anyMoveAvailable();
    m_undo.reset();
    return true;
}

void Game::spawn() {
    const QVector<QPoint> cells = m_board.emptyCells();
    if (cells.isEmpty())
        return;
    const QPoint cell = cells.at(int(m_rng.bounded(quint32(cells.size()))));
    const int value = m_rng.bounded(10) == 0 ? 4 : 2;
    m_board.put(cell.y(), cell.x(), value);
}
