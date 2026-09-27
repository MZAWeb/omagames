#include "difficultytests.h"

#include <QtTest>

#include "challenge.h"
#include "difficulty.h"
#include "enginefixture.h"

using namespace EngineFixture;

namespace {

constexpr int kSeedsTried = 300;
constexpr int kTop = kBottom - 1;

// Two dealt rows with open gaps at the left: the bottom one missing columns
// 0 and 1, the one over it columns 0 to 3. Nothing covers either.
Board twoOpenRows() {
    Board board;
    fillRow(board, kBottom, {0, 1});
    fillRow(board, kTop, {0, 1, 2, 3});
    return board;
}

const std::vector<int> kDealt {kTop, kBottom};

void put(Board &board, std::initializer_list<QPoint> cells) {
    for (QPoint cell : cells)
        board.set(cell, PieceType::T);
}

}  // namespace

void DifficultyTests::measureFindsTheRowsThatHaveToGo() {
    Board board = twoOpenRows();
    Difficulty::Features open = Difficulty::measure(board, kDealt);
    QCOMPARE(open.rowsToClear, 2);
    QCOMPARE(open.cellsToFill, 6);
    QCOMPARE(open.buried, 0);
    QCOMPARE(open.height, 2);

    // A block two rows up over column 0 buries both gaps under it, and its
    // own row has to go before they can be filled: nine more cells to fill.
    put(board, {{0, kTop - 2}});
    const Difficulty::Features buried = Difficulty::measure(board, kDealt);
    QCOMPARE(buried.rowsToClear, 3);
    QCOMPARE(buried.cellsToFill, 6 + 9);
    // The two dealt gaps under it and the empty cell of the row between.
    QCOMPARE(buried.buried, 2);
    QCOMPARE(buried.height, 4);
}

// The healthy move: pieces over the solid columns, clear of every gap. The
// dealt rows are no further from done, so the rating must not climb.
void DifficultyTests::buildingBesideTheGapsCostsNothing() {
    Board board = twoOpenRows();
    const int before = Difficulty::rate(board, kDealt);
    put(board, {{6, kTop - 1}, {7, kTop - 1}, {8, kTop - 1}, {9, kTop - 1}});
    put(board, {{6, kTop - 2}, {7, kTop - 2}, {8, kTop - 2}, {9, kTop - 2}});
    QCOMPARE(Difficulty::rate(board, kDealt), before);
}

void DifficultyTests::fillingAGapHelpsAndBuryingOneHurts() {
    const Board start = twoOpenRows();
    const int before = Difficulty::rate(start, kDealt);

    // An O dropped into the gap fills four of the six cells.
    Board filledIn = start;
    put(filledIn, {{0, kTop}, {1, kTop}, {0, kBottom}, {1, kBottom}});
    QVERIFY(Difficulty::rate(filledIn, kDealt) < before);

    // The same four cells laid flat across the top of the gap instead: a
    // misplaced I. It fills nothing and seals the gap under a new row.
    Board sealed = start;
    put(sealed, {{0, kTop - 1}, {1, kTop - 1}, {2, kTop - 1}, {3, kTop - 1}});
    QVERIFY(Difficulty::rate(sealed, kDealt) >= before + 10);
}

void DifficultyTests::ratingStaysBetweenOneAndAHundred() {
    // Nothing dealt left: done, whatever else is on the board.
    Board board = twoOpenRows();
    QCOMPARE(Difficulty::rate(board, {}), Difficulty::kMin);
    // One cell short of a clear is as easy as it gets.
    Board almost;
    fillRow(almost, kBottom, {9});
    QCOMPARE(Difficulty::rate(almost, {kBottom}), Difficulty::kMin);
    // Every visible row dealt, each with a gap buried under the row above.
    Board hopeless;
    std::vector<int> everyRow;
    for (int y = Board::kHiddenRows; y < Board::kHeight; ++y) {
        fillRow(hopeless, y, {y % 2 == 0 ? 2 : 7});
        everyRow.push_back(y);
    }
    QCOMPARE(Difficulty::rate(hopeless, everyRow), Difficulty::kMax);
}

// A deal rates somewhere in the middle, rarely at either end, so there is
// room to see it get better or worse.
void DifficultyTests::freshDealsSpreadAcrossTheScale() {
    int lowest = Difficulty::kMax;
    int highest = Difficulty::kMin;
    for (quint32 seed = 0; seed < kSeedsTried; ++seed) {
        Board board;
        const int rating = Difficulty::rate(board, Challenge::build(board, seed));
        lowest = std::min(lowest, rating);
        highest = std::max(highest, rating);
    }
    QVERIFY(lowest > Difficulty::kMin && lowest < 20);
    QVERIFY(highest > 60 && highest < Difficulty::kMax);
}

void DifficultyTests::theGameRatesEverySettledBoard() {
    QCOMPARE(Game(Mode::Zen, kSeed).difficulty(), 0);

    Game game(Mode::Challenge, kSeed);
    QCOMPARE(game.difficulty(), Difficulty::rate(game.board(), game.dealtStack()));
    QVERIFY(game.difficulty() > Difficulty::kMin);
    for (int i = 0; i < 4 && game.phase() == Phase::Playing; ++i) {
        game.hardDrop();
        waitOutTheFlash(game);
        QCOMPARE(game.difficulty(), Difficulty::rate(game.board(), game.dealtStack()));
    }

    // Cleared out, it rates as done.
    Game cleared(Mode::Challenge, kSeed);
    for (int y = Board::kHeight - cleared.dealtRows(); y < Board::kHeight; ++y)
        fillRow(cleared.mutableBoard(), y, {0});
    while (cleared.phase() == Phase::Playing) {
        cleared.placePiece({PieceType::I, 1, {-2, 0}});
        cleared.hardDrop();
        waitOutTheFlash(cleared);
    }
    QCOMPARE(cleared.phase(), Phase::Finished);
    QCOMPARE(cleared.difficulty(), Difficulty::kMin);
}
