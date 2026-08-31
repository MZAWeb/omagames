#include "boardtests.h"

#include <QtTest>

#include "board.h"

namespace {
// Lays values out along `row` starting at column 0; a 0 leaves the cell empty.
void fillRow(Board &board, int row, std::initializer_list<int> values) {
    int col = 0;
    for (int value : values) {
        if (value != 0)
            board.put(row, col, value);
        ++col;
    }
}

QVector<int> rowValues(const Board &board, int row) {
    QVector<int> values;
    for (int col = 0; col < Board::kSize; ++col)
        values.append(board.valueAt(row, col));
    return values;
}

QVector<int> colValues(const Board &board, int col) {
    QVector<int> values;
    for (int row = 0; row < Board::kSize; ++row)
        values.append(board.valueAt(row, col));
    return values;
}
} // namespace

void BoardTests::tripleMergesClosestToTheEdge() {
    Board board;
    fillRow(board, 0, {2, 2, 2});
    QVERIFY(board.move(Direction::Right).moved);
    QCOMPARE(rowValues(board, 0), (QVector<int>{0, 0, 2, 4}));

    board.clear();
    fillRow(board, 0, {0, 2, 2, 2});
    QVERIFY(board.move(Direction::Left).moved);
    QCOMPARE(rowValues(board, 0), (QVector<int>{4, 2, 0, 0}));
}

void BoardTests::fourEqualTilesMergeIntoTwoPairs() {
    Board board;
    fillRow(board, 0, {2, 2, 2, 2});
    const MoveResult result = board.move(Direction::Right);
    QVERIFY(result.moved);
    QCOMPARE(rowValues(board, 0), (QVector<int>{0, 0, 4, 4}));
    QCOMPARE(result.scoreGained, 8);
}

void BoardTests::unequalNeighbourWorkedExamples() {
    Board board;
    fillRow(board, 0, {4, 2, 2});
    QVERIFY(board.move(Direction::Right).moved);
    QCOMPARE(rowValues(board, 0), (QVector<int>{0, 0, 4, 4}));

    board.clear();
    fillRow(board, 0, {4, 2, 2});
    QVERIFY(board.move(Direction::Left).moved);
    QCOMPARE(rowValues(board, 0), (QVector<int>{4, 4, 0, 0}));

    board.clear();
    fillRow(board, 0, {2, 2, 4});
    QVERIFY(board.move(Direction::Right).moved);
    QCOMPARE(rowValues(board, 0), (QVector<int>{0, 0, 4, 4}));

    board.clear();
    fillRow(board, 0, {2, 2, 4});
    QVERIFY(board.move(Direction::Left).moved);
    QCOMPARE(rowValues(board, 0), (QVector<int>{4, 4, 0, 0}));
}

void BoardTests::mergedTileNeverMergesAgain() {
    // The 4s make an 8 right next to an existing 8; the fresh 8 must stay.
    Board board;
    fillRow(board, 0, {4, 4, 8});
    QCOMPARE(board.move(Direction::Right).scoreGained, 8);
    QCOMPARE(rowValues(board, 0), (QVector<int>{0, 0, 8, 8}));

    board.clear();
    fillRow(board, 0, {8, 4, 4});
    QCOMPARE(board.move(Direction::Right).scoreGained, 8);
    QCOMPARE(rowValues(board, 0), (QVector<int>{0, 0, 8, 8}));
}

void BoardTests::everyDirectionSlidesAndMerges() {
    Board board;
    board.put(0, 1, 2);
    board.put(2, 1, 2);
    QVERIFY(board.move(Direction::Down).moved);
    QCOMPARE(colValues(board, 1), (QVector<int>{0, 0, 0, 4}));

    board.clear();
    board.put(1, 2, 2);
    board.put(3, 2, 2);
    QVERIFY(board.move(Direction::Up).moved);
    QCOMPARE(colValues(board, 2), (QVector<int>{4, 0, 0, 0}));

    board.clear();
    board.put(0, 0, 2);
    board.put(1, 1, 2);
    board.put(2, 2, 2);
    QVERIFY(board.move(Direction::Down).moved);
    QCOMPARE(rowValues(board, 3), (QVector<int>{2, 2, 2, 0}));
}

void BoardTests::slideKeepsTheTileId() {
    Board board;
    board.put(2, 1, 8);
    const int id = board.idAt(2, 1);
    QVERIFY(board.move(Direction::Right).moved);
    QCOMPARE(board.idAt(2, 3), id);
    QCOMPARE(board.tiles().size(), 1);
}

void BoardTests::mergeKeepsTheIdOfTheTileSlidOnto() {
    Board board;
    board.put(0, 3, 2);
    const int keptId = board.idAt(0, 3);
    board.put(0, 0, 2);
    QVERIFY(board.move(Direction::Right).moved);
    QCOMPARE(board.tiles().size(), 1);
    QCOMPARE(board.idAt(0, 3), keptId);
    QCOMPARE(board.valueAt(0, 3), 4);
}

void BoardTests::moveThatChangesNothingReportsIt() {
    Board board;
    board.put(0, 3, 2);
    board.put(1, 3, 4);
    board.put(2, 3, 2);
    const MoveResult result = board.move(Direction::Right);
    QVERIFY(!result.moved);
    QCOMPARE(result.scoreGained, 0);
    QCOMPARE(colValues(board, 3), (QVector<int>{2, 4, 2, 0}));
}

void BoardTests::scoringIsTheValueOfTheCreatedTile() {
    Board board;
    fillRow(board, 0, {8, 8});
    QCOMPARE(board.move(Direction::Left).scoreGained, 16);

    board.clear();
    fillRow(board, 0, {2, 2});
    fillRow(board, 1, {16, 16});
    QCOMPARE(board.move(Direction::Left).scoreGained, 36);
}

void BoardTests::canMoveMatchesTheBoard() {
    // Alternating values: full board, nothing adjacent is equal, no move.
    Board board;
    fillRow(board, 0, {2, 4, 2, 4});
    fillRow(board, 1, {4, 2, 4, 2});
    fillRow(board, 2, {2, 4, 2, 4});
    fillRow(board, 3, {4, 2, 4, 2});
    QVERIFY(!board.canMove(Direction::Left));
    QVERIFY(!board.canMove(Direction::Right));
    QVERIFY(!board.canMove(Direction::Up));
    QVERIFY(!board.canMove(Direction::Down));
    QVERIFY(!board.anyMoveAvailable());

    // A horizontal pair whose vertical neighbours differ opens exactly the
    // horizontal directions.
    fillRow(board, 3, {8, 8, 4, 2});
    QVERIFY(board.canMove(Direction::Left));
    QVERIFY(board.canMove(Direction::Right));
    QVERIFY(!board.canMove(Direction::Up));
    QVERIFY(!board.canMove(Direction::Down));
    QVERIFY(board.anyMoveAvailable());
}

void BoardTests::emptyCellsAndHighestValue() {
    Board board;
    QCOMPARE(board.emptyCells().size(), Board::kSize * Board::kSize);
    QCOMPARE(board.highestValue(), 0);

    board.put(1, 2, 64);
    board.put(3, 0, 8);
    QCOMPARE(board.emptyCells().size(), 14);
    QVERIFY(!board.emptyCells().contains(QPoint(2, 1)));
    QVERIFY(!board.emptyCells().contains(QPoint(0, 3)));
    QCOMPARE(board.highestValue(), 64);

    board.clear();
    QCOMPARE(board.tiles().size(), 0);
    QCOMPARE(board.emptyCells().size(), Board::kSize * Board::kSize);
}
