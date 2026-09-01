#include "gametests.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QtTest>

#include "game.h"

namespace {
struct Cell {
    int row;
    int col;
    int value;
};

QJsonObject stateJson(const QVector<Cell> &cells, int score = 0, bool won = false) {
    QJsonArray tiles;
    for (const Cell &cell : cells)
        tiles.append(QJsonObject{{QStringLiteral("row"), cell.row},
                                 {QStringLiteral("col"), cell.col},
                                 {QStringLiteral("value"), cell.value}});
    return {{QStringLiteral("tiles"), tiles},
            {QStringLiteral("score"), score},
            {QStringLiteral("won"), won}};
}

// Full board, alternating values, nothing adjacent equal: no move exists.
QVector<Cell> deadBoard() {
    QVector<Cell> cells;
    for (int row = 0; row < Board::kSize; ++row)
        for (int col = 0; col < Board::kSize; ++col)
            cells.append({row, col, (row + col) % 2 == 0 ? 2 : 4});
    return cells;
}

// One empty cell at (3,3); sliding row 3 right fills the board and leaves no
// adjacent pair whether the spawn at (3,0) is a 2 or a 4.
QVector<Cell> oneMoveFromOver() {
    return {{0, 0, 2},   {0, 1, 4},    {0, 2, 8},  {0, 3, 16},
            {1, 0, 32},  {1, 1, 64},   {1, 2, 128}, {1, 3, 256},
            {2, 0, 512}, {2, 1, 1024}, {2, 2, 2},  {2, 3, 4},
            {3, 0, 8},   {3, 1, 16},   {3, 2, 32}};
}
} // namespace

void GameTests::newGameStartsWithTwoTiles() {
    Game game(42);
    game.newGame();
    QCOMPARE(game.board().tiles().size(), 2);
    for (const Tile &tile : game.board().tiles())
        QVERIFY(tile.value == 2 || tile.value == 4);
    QCOMPARE(game.score(), 0);
    QVERIFY(!game.won());
    QVERIFY(!game.over());
    QVERIFY(!game.canUndo());
}

void GameTests::moveSpawnsExactlyOneNewTile() {
    Game game(42);
    QVERIFY(game.restore(stateJson({{0, 0, 8}})));
    QVERIFY(game.move(Direction::Right));
    QCOMPARE(game.board().tiles().size(), 2);
    QCOMPARE(game.board().valueAt(0, 3), 8);
}

void GameTests::rejectedMoveSpawnsNothing() {
    Game game(42);
    QVERIFY(game.restore(stateJson({{0, 3, 8}}, 50)));
    QVERIFY(!game.move(Direction::Right));
    QCOMPARE(game.board().tiles().size(), 1);
    QCOMPARE(game.score(), 50);
    QVERIFY(!game.canUndo());
}

void GameTests::spawnsAreDeterministicFromTheSeed() {
    Game a(7);
    Game b(7);
    a.newGame();
    b.newGame();
    const auto directions = {Direction::Left, Direction::Up, Direction::Right,
                             Direction::Down, Direction::Left, Direction::Up};
    for (Direction direction : directions) {
        QCOMPARE(a.move(direction), b.move(direction));
        QCOMPARE(a.toJson(), b.toJson());
    }

    Game c(8);
    c.newGame();
    QVERIFY(c.toJson() != a.toJson());
}

void GameTests::spawnsAreNineToOneAndSpreadOut() {
    Game game(1);
    int fours = 0;
    QSet<int> cells;
    for (int i = 0; i < 1000; ++i) {
        QVERIFY(game.restore(stateJson({{0, 0, 8}})));
        QVERIFY(game.move(Direction::Right));
        for (const Tile &tile : game.board().tiles()) {
            if (tile.value == 8)
                continue;
            QVERIFY(tile.value == 2 || tile.value == 4);
            if (tile.value == 4)
                ++fours;
            cells.insert(tile.row * Board::kSize + tile.col);
        }
    }
    QVERIFY(fours > 60 && fours < 140);
    QVERIFY(cells.size() > 10);
}

void GameTests::scoreAccumulatesAcrossMoves() {
    Game game(42);
    QVERIFY(game.restore(stateJson({{0, 0, 2}, {0, 1, 2}, {0, 2, 2}, {0, 3, 2}}, 100)));
    QVERIFY(game.move(Direction::Right));
    QCOMPARE(game.score(), 108);
}

void GameTests::winFiresOnceAndKeepGoingContinues() {
    Game game(42);
    QVERIFY(game.restore(stateJson({{0, 2, 1024}, {0, 3, 1024}}, 500)));
    game.keepGoing();
    QVERIFY(!game.keepPlaying());

    QVERIFY(game.move(Direction::Right));
    QVERIFY(game.won());
    QVERIFY(!game.keepPlaying());
    QCOMPARE(game.score(), 500 + 2048);
    QCOMPARE(game.board().valueAt(0, 3), 2048);

    game.keepGoing();
    QVERIFY(game.keepPlaying());
    QVERIFY(game.move(Direction::Left));
    QVERIFY(game.won());
    QVERIFY(game.keepPlaying());
}

void GameTests::fullBoardWithAPairIsNotOver() {
    Game game(42);
    QVector<Cell> cells = deadBoard();
    cells[13].value = 8; // (3,1)
    cells[12].value = 8; // (3,0): a horizontal pair among unequal columns
    QVERIFY(game.restore(stateJson(cells)));
    QVERIFY(!game.over());
}

void GameTests::fullBoardWithoutPairsIsOver() {
    Game game(42);
    QVERIFY(game.restore(stateJson(deadBoard(), 300)));
    QVERIFY(game.over());
    QVERIFY(!game.move(Direction::Left));
    QCOMPARE(game.score(), 300);
    QVERIFY(!game.canUndo());
}

void GameTests::moveIntoGameOverEndsTheRun() {
    Game game(42);
    QVERIFY(game.restore(stateJson(oneMoveFromOver())));
    QVERIFY(!game.over());
    QVERIFY(game.move(Direction::Right));
    QVERIFY(game.over());
    QCOMPARE(game.board().emptyCells().size(), 0);
    QVERIFY(!game.canUndo());
    QVERIFY(!game.undo());
}

void GameTests::undoIsUnavailableBeforeTheFirstMove() {
    Game game(42);
    game.newGame();
    QVERIFY(!game.canUndo());
    QVERIFY(!game.undo());
}

void GameTests::undoRevertsBoardScoreAndSpawn() {
    Game game(42);
    QVERIFY(game.restore(stateJson({{0, 0, 2}, {0, 1, 2}}, 10)));
    QVERIFY(game.move(Direction::Right));
    QCOMPARE(game.score(), 14);
    QCOMPARE(game.board().tiles().size(), 2); // the merged 4 plus the spawn

    QVERIFY(game.canUndo());
    QVERIFY(game.undo());
    QCOMPARE(game.board().tiles().size(), 2);
    QCOMPARE(game.board().valueAt(0, 0), 2);
    QCOMPARE(game.board().valueAt(0, 1), 2);
    QCOMPARE(game.score(), 10);
}

void GameTests::undoIsUnavailableTwiceInARow() {
    Game game(42);
    QVERIFY(game.restore(stateJson({{0, 0, 2}, {0, 1, 2}})));
    QVERIFY(game.move(Direction::Right));
    QVERIFY(game.undo());
    QVERIFY(!game.canUndo());
    QVERIFY(!game.undo());

    QVERIFY(game.move(Direction::Left));
    QVERIFY(game.canUndo());
}

void GameTests::undoRevertsTheWinningMove() {
    Game game(42);
    QVERIFY(game.restore(stateJson({{0, 2, 1024}, {0, 3, 1024}}, 500)));
    QVERIFY(game.move(Direction::Right));
    QVERIFY(game.won());
    QVERIFY(game.undo());
    QVERIFY(!game.won());
    QCOMPARE(game.score(), 500);
    QCOMPARE(game.board().valueAt(0, 2), 1024);
    QCOMPARE(game.board().valueAt(0, 3), 1024);
}

void GameTests::jsonRoundTripsTheRun() {
    Game game(42);
    game.newGame();
    QVERIFY(game.move(Direction::Left) || game.move(Direction::Up)
            || game.move(Direction::Right) || game.move(Direction::Down));

    Game other(1234);
    QVERIFY(other.restore(game.toJson()));
    QCOMPARE(other.toJson(), game.toJson());
    QCOMPARE(other.score(), game.score());
    QCOMPARE(other.won(), game.won());
    QCOMPARE(other.over(), game.over());
}

void GameTests::restoreLeavesNothingToUndo() {
    Game game(42);
    game.newGame();
    QVERIFY(game.move(Direction::Left) || game.move(Direction::Up)
            || game.move(Direction::Right) || game.move(Direction::Down));
    QVERIFY(game.canUndo());

    QVERIFY(game.restore(game.toJson()));
    QVERIFY(!game.canUndo());
    QVERIFY(!game.undo());
}

void GameTests::restoredWinDoesNotReshowTheOverlay() {
    Game game(42);
    QVERIFY(game.restore(stateJson({{0, 0, 2048}, {1, 1, 2}}, 20000, true)));
    QVERIFY(game.won());
    QVERIFY(game.keepPlaying());
}

void GameTests::restoreRejectsMalformedState() {
    Game game(42);
    QVERIFY(game.restore(stateJson({{2, 2, 32}}, 64)));

    QVERIFY(!game.restore(QJsonObject{{QStringLiteral("score"), 5}}));
    QVERIFY(!game.restore(stateJson({{0, 0, 2}}, -1)));
    QVERIFY(!game.restore(stateJson({{4, 0, 2}})));
    QVERIFY(!game.restore(stateJson({{0, -1, 2}})));
    QVERIFY(!game.restore(stateJson({{0, 0, 3}})));
    QVERIFY(!game.restore(stateJson({{0, 0, 2}, {0, 0, 4}})));

    // A failed restore leaves the game exactly as it was.
    QCOMPARE(game.board().tiles().size(), 1);
    QCOMPARE(game.board().valueAt(2, 2), 32);
    QCOMPARE(game.score(), 64);
}
