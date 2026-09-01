#include "board.h"

namespace {
// The cell `offset` steps in from the destination edge of `line`. Walking
// offsets in order visits each line front-to-back, which is what makes the
// pair closest to that edge merge first.
QPoint cellAt(Direction direction, int line, int offset) {
    switch (direction) {
    case Direction::Left:
        return {offset, line};
    case Direction::Right:
        return {Board::kSize - 1 - offset, line};
    case Direction::Up:
        return {line, offset};
    case Direction::Down:
        return {line, Board::kSize - 1 - offset};
    }
    return {};
}
} // namespace

MoveResult Board::move(Direction direction) {
    MoveResult result;
    QVector<Tile> moved;
    moved.reserve(m_tiles.size());
    for (int line = 0; line < kSize; ++line) {
        int placedInLine = 0;
        int lastPlaced = -1; // index into `moved` of this line's last kept tile
        bool lastMerged = false;
        for (int offset = 0; offset < kSize; ++offset) {
            const QPoint cell = cellAt(direction, line, offset);
            const int index = indexAt(cell.y(), cell.x());
            if (index < 0)
                continue;
            Tile tile = m_tiles.at(index);
            if (lastPlaced >= 0 && !lastMerged && moved.at(lastPlaced).value == tile.value) {
                moved[lastPlaced].value *= 2;
                result.scoreGained += moved.at(lastPlaced).value;
                lastMerged = true;
                result.moved = true;
            } else {
                const QPoint target = cellAt(direction, line, placedInLine);
                if (target.y() != tile.row || target.x() != tile.col)
                    result.moved = true;
                tile.row = target.y();
                tile.col = target.x();
                lastPlaced = moved.size();
                moved.append(tile);
                lastMerged = false;
                ++placedInLine;
            }
        }
    }
    if (result.moved)
        m_tiles = moved;
    return result;
}

bool Board::canMove(Direction direction) const {
    Board scratch = *this;
    return scratch.move(direction).moved;
}

bool Board::anyMoveAvailable() const {
    return canMove(Direction::Left) || canMove(Direction::Right)
        || canMove(Direction::Up) || canMove(Direction::Down);
}

QVector<QPoint> Board::emptyCells() const {
    QVector<QPoint> cells;
    for (int row = 0; row < kSize; ++row)
        for (int col = 0; col < kSize; ++col)
            if (indexAt(row, col) < 0)
                cells.append(QPoint(col, row));
    return cells;
}

int Board::valueAt(int row, int col) const {
    const int index = indexAt(row, col);
    return index < 0 ? 0 : m_tiles.at(index).value;
}

int Board::idAt(int row, int col) const {
    const int index = indexAt(row, col);
    return index < 0 ? -1 : m_tiles.at(index).id;
}

int Board::highestValue() const {
    int highest = 0;
    for (const Tile &tile : m_tiles)
        highest = qMax(highest, tile.value);
    return highest;
}

void Board::put(int row, int col, int value) {
    const int index = indexAt(row, col);
    if (index >= 0)
        m_tiles.removeAt(index);
    m_tiles.append({m_nextId++, value, row, col});
}

void Board::clear() {
    // The id counter keeps running so tiles from consecutive games never share
    // an id a renderer might still be animating.
    m_tiles.clear();
}

int Board::indexAt(int row, int col) const {
    for (int i = 0; i < m_tiles.size(); ++i)
        if (m_tiles.at(i).row == row && m_tiles.at(i).col == col)
            return i;
    return -1;
}
