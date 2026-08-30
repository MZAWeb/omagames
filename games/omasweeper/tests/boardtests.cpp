#include "boardtests.h"

#include <QtTest>

#include "board.h"

namespace {

Board boardWith(int width, int height, const std::vector<int> &mines) {
    Board board(width, height, int(mines.size()));
    board.placeMines(mines);
    return board;
}

int chebyshev(QPoint a, QPoint b) { return std::max(std::abs(a.x() - b.x()), std::abs(a.y() - b.y())); }

}  // namespace

void BoardTests::numbersCountAdjacentMines() {
    // Mines in two opposite corners of a 3×3.
    const Board board = boardWith(3, 3, {0, 8});
    QCOMPARE(board.mineCount(), 2);
    QCOMPARE(board.safeCount(), 7);
    QCOMPARE(int(board.cell(QPoint(1, 1)).adjacent), 2);
    QCOMPARE(int(board.cell(QPoint(1, 0)).adjacent), 1);
    QCOMPARE(int(board.cell(QPoint(2, 0)).adjacent), 0);
    QCOMPARE(int(board.cell(QPoint(0, 2)).adjacent), 0);
    QCOMPARE(int(board.cell(QPoint(2, 1)).adjacent), 1);
    QVERIFY(board.cell(0).mine);
    QCOMPARE(board.mines(), (std::vector<int>{0, 8}));
    QCOMPARE(board.neighbours(0), (std::vector<int>{1, 3, 4}));
    QCOMPARE(int(board.neighbours(4).size()), 8);
}

void BoardTests::zeroCascadesOutwards() {
    Board board = boardWith(5, 5, {24});
    const MoveResult result = board.reveal(QPoint(0, 0));
    QCOMPARE(int(result.revealed.size()), 24);
    QCOMPARE(result.revealed.front(), 0);
    QVERIFY(result.won());
    QCOMPARE(board.status(), Status::Won);
    QCOMPARE(board.revealedCount(), 24);
    // Breadth-first: distance from the click never decreases along the list.
    int distance = 0;
    for (int i : result.revealed) {
        const int d = chebyshev(board.point(i), QPoint(0, 0));
        QVERIFY(d >= distance);
        distance = d;
    }
    QCOMPARE(board.cell(24).state, CellState::Hidden);

    // A number stops the ripple.
    Board small = boardWith(3, 3, {0});
    QCOMPARE(int(small.reveal(QPoint(1, 1)).revealed.size()), 1);
    QCOMPARE(small.status(), Status::Playing);
}

void BoardTests::firstRevealPlacesMinesAroundASafeZero() {
    Board board(9, 9, 10, 7u);
    QVERIFY(!board.minesPlaced());
    QCOMPARE(board.status(), Status::Ready);
    const MoveResult result = board.reveal(QPoint(4, 4));
    QVERIFY(board.minesPlaced());
    QCOMPARE(board.status(), Status::Playing);
    QCOMPARE(board.mineCount(), 10);
    QCOMPARE(int(board.mines().size()), 10);
    QVERIFY(!result.lost());
    QCOMPARE(int(board.cell(QPoint(4, 4)).adjacent), 0);
    for (int n : board.neighbours(board.index(QPoint(4, 4))))
        QVERIFY(!board.cell(n).mine);
    QVERIFY(int(result.revealed.size()) >= 9);

    // Too small for a 3×3 opening: only the click itself is spared.
    Board tiny(2, 2, 3, 1u);
    QVERIFY(!tiny.reveal(QPoint(0, 0)).lost());
    QCOMPARE(tiny.mineCount(), 3);
}

void BoardTests::revealingAMineLosesAndReportsWrongFlags() {
    Board board = boardWith(3, 3, {0, 8});
    board.reveal(QPoint(1, 1));
    board.toggleFlag(QPoint(2, 0));
    board.toggleFlag(QPoint(8 % 3, 8 / 3));
    const MoveResult result = board.reveal(QPoint(0, 0));
    QVERIFY(result.lost());
    QCOMPARE(result.exploded, 0);
    QCOMPARE(result.wrongFlags, (std::vector<int>{2}));
    QCOMPARE(board.status(), Status::Lost);
    QVERIFY(result.revealed.empty());
}

void BoardTests::flagsToggleAndTheCounterGoesNegative() {
    Board board = boardWith(3, 3, {4});
    QCOMPARE(board.remainingMines(), 1);
    QCOMPARE(board.toggleFlag(QPoint(0, 0)).toggled, 0);
    QCOMPARE(board.cell(0).state, CellState::Flagged);
    board.toggleFlag(QPoint(1, 0));
    QCOMPARE(board.flagCount(), 2);
    QCOMPARE(board.remainingMines(), -1);
    // Flagging before the first reveal does not start the game.
    QCOMPARE(board.status(), Status::Ready);
    QCOMPARE(board.toggleFlag(QPoint(0, 0)).toggled, 0);
    QCOMPARE(board.cell(0).state, CellState::Hidden);
    QCOMPARE(board.remainingMines(), 0);
    QVERIFY(!board.toggleFlag(QPoint(9, 9)).changed());
}

void BoardTests::flaggedAndRevealedCellsIgnoreReveal() {
    Board board = boardWith(3, 3, {0});
    board.toggleFlag(QPoint(0, 0));
    QVERIFY(!board.reveal(QPoint(0, 0)).changed());
    QCOMPARE(board.status(), Status::Ready);
    board.reveal(QPoint(1, 1));
    QVERIFY(!board.reveal(QPoint(1, 1)).changed());
    QVERIFY(!board.toggleFlag(QPoint(1, 1)).changed());
    QVERIFY(!board.reveal(QPoint(-1, 0)).changed());
}

void BoardTests::chordRevealsAroundASatisfiedNumber() {
    Board board = boardWith(3, 3, {0});
    board.reveal(QPoint(1, 1));
    board.toggleFlag(QPoint(0, 0));
    const MoveResult result = board.chord(QPoint(1, 1));
    QCOMPARE(int(result.revealed.size()), 7);
    QVERIFY(result.won());
    QCOMPARE(board.flagCount(), 1);
}

void BoardTests::chordWithAWrongFlagLoses() {
    Board board = boardWith(3, 3, {0});
    board.reveal(QPoint(1, 1));
    board.toggleFlag(QPoint(1, 0));
    const MoveResult result = board.chord(QPoint(1, 1));
    QVERIFY(result.lost());
    QCOMPARE(result.exploded, 0);
    QCOMPARE(result.wrongFlags, (std::vector<int>{1}));
}

void BoardTests::chordNeedsMatchingFlags() {
    Board board = boardWith(3, 3, {0, 8});
    board.reveal(QPoint(1, 1));
    QVERIFY(!board.chord(QPoint(1, 1)).changed());
    board.toggleFlag(QPoint(0, 0));
    QVERIFY(!board.chord(QPoint(1, 1)).changed());
    // Chording a hidden cell or a zero does nothing either.
    QVERIFY(!board.chord(QPoint(2, 0)).changed());
    board.reveal(QPoint(2, 0));
    QVERIFY(!board.chord(QPoint(2, 0)).changed());
    board.toggleFlag(QPoint(2, 2));
    // (2, 0) was a zero and already rippled into (1, 0) and (2, 1).
    QCOMPARE(int(board.chord(QPoint(1, 1)).revealed.size()), 3);
    QCOMPARE(board.status(), Status::Won);
}

void BoardTests::winningNeedsNoFlags() {
    Board board = boardWith(3, 3, {4});
    for (int i = 0; i < 9; ++i) {
        if (i != 4)
            board.reveal(board.point(i));
    }
    QCOMPARE(board.status(), Status::Won);
    QCOMPARE(board.flagCount(), 0);
    QCOMPARE(board.remainingMines(), 1);
    QCOMPARE(board.revealedCount(), board.safeCount());
}

void BoardTests::nothingMovesAfterTheGameEnds() {
    Board board = boardWith(3, 3, {0});
    board.reveal(QPoint(0, 0));
    QCOMPARE(board.status(), Status::Lost);
    QVERIFY(!board.reveal(QPoint(2, 2)).changed());
    QVERIFY(!board.toggleFlag(QPoint(2, 2)).changed());
    QCOMPARE(board.reveal(QPoint(2, 2)).status, Status::Lost);
}

void BoardTests::mineCountIsClamped() {
    QCOMPARE(Board(3, 3, 50).mineCount(), 8);
    QCOMPARE(Board(3, 3, -2).mineCount(), 0);
    QCOMPARE(Board(0, 200, 1).width(), Board::kMinSize);
    QCOMPARE(Board(0, 200, 1).height(), Board::kMaxSize);
    Board none(3, 3, 0);
    QVERIFY(none.reveal(QPoint(1, 1)).won());
}

void BoardTests::sameSeedSameMines() {
    Board a(16, 16, 40, 99u);
    Board b(16, 16, 40, 99u);
    Board c(16, 16, 40, 100u);
    a.reveal(QPoint(8, 8));
    b.reveal(QPoint(8, 8));
    c.reveal(QPoint(8, 8));
    QCOMPARE(a.mines(), b.mines());
    QVERIFY(a.mines() != c.mines());
}

void BoardTests::jsonRoundTrip() {
    Board board = boardWith(4, 3, {0, 11});
    board.reveal(QPoint(3, 0));
    board.toggleFlag(QPoint(0, 0));
    board.toggleFlag(QPoint(0, 1));
    const std::optional<Board> copy = Board::fromJson(board.toJson());
    QVERIFY(copy.has_value());
    QCOMPARE(copy->width(), 4);
    QCOMPARE(copy->height(), 3);
    QCOMPARE(copy->mines(), board.mines());
    QCOMPARE(copy->status(), Status::Playing);
    QCOMPARE(copy->revealedCount(), board.revealedCount());
    QCOMPARE(copy->flagCount(), 2);
    for (int i = 0; i < board.cellCount(); ++i) {
        QCOMPARE(copy->cell(i).state, board.cell(i).state);
        QCOMPARE(copy->cell(i).adjacent, board.cell(i).adjacent);
    }
    // A fresh board keeps its seed and its unplaced mines.
    const std::optional<Board> fresh = Board::fromJson(Board(9, 9, 10, 42u).toJson());
    QVERIFY(fresh.has_value());
    QVERIFY(!fresh->minesPlaced());
    QCOMPARE(fresh->seed(), 42u);
    QCOMPARE(fresh->mineCount(), 10);
}

void BoardTests::jsonRejectsGarbage() {
    QVERIFY(!Board::fromJson(QJsonObject()).has_value());
    QJsonObject json = boardWith(3, 3, {0}).toJson();
    json[QStringLiteral("cells")] = QStringLiteral("...");
    QVERIFY(!Board::fromJson(json).has_value());
    json = boardWith(3, 3, {0}).toJson();
    json[QStringLiteral("status")] = 9;
    QVERIFY(!Board::fromJson(json).has_value());
}
