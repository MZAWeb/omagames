#include "difficultytests.h"

#include <QtTest>

#include "challenge.h"
#include "difficulty.h"
#include "enginefixture.h"

using namespace EngineFixture;

namespace {

constexpr int kSeedsTried = 300;

Difficulty::Features someMess() {
    Difficulty::Features features;
    features.holes = 6;
    features.cover = 12;
    features.bumpiness = 5;
    features.height = 7;
    features.rowsLeft = 6;
    return features;
}

}  // namespace

void DifficultyTests::measureReadsTheBoard() {
    Board board;
    // Column 0: two blocks over two empty cells. Column 1: one block on the
    // floor. The rest empty.
    board.set({0, kBottom - 3}, PieceType::L);
    board.set({0, kBottom - 2}, PieceType::L);
    board.set({1, kBottom}, PieceType::L);
    const Difficulty::Features features = Difficulty::measure(board, {kBottom - 1, kBottom});
    QCOMPARE(features.holes, 2);
    QCOMPARE(features.cover, 4);
    QCOMPARE(features.height, 4);
    // 4 -> 1 -> 0, then flat.
    QCOMPARE(features.bumpiness, 4);
    QCOMPARE(features.rowsLeft, 2);
}

void DifficultyTests::everyFeaturePushesTheRatingUp() {
    const Difficulty::Features base = someMess();
    const int rating = Difficulty::rate(base);
    auto worse = [&](int Difficulty::Features::*field, int by) {
        Difficulty::Features changed = base;
        changed.*field += by;
        return Difficulty::rate(changed);
    };
    QVERIFY(worse(&Difficulty::Features::holes, 3) > rating);
    QVERIFY(worse(&Difficulty::Features::cover, 5) > rating);
    QVERIFY(worse(&Difficulty::Features::bumpiness, 6) > rating);
    QVERIFY(worse(&Difficulty::Features::rowsLeft, 3) > rating);
    // Height is free while there is room, and costly once there is not.
    QCOMPARE(worse(&Difficulty::Features::height, 2), rating);
    QVERIFY(worse(&Difficulty::Features::height, 10) > rating + 20);
}

void DifficultyTests::ratingStaysBetweenOneAndAHundred() {
    // Nothing dealt left: done, whatever else is on the board.
    Difficulty::Features done = someMess();
    done.rowsLeft = 0;
    QCOMPARE(Difficulty::rate(done), Difficulty::kMin);
    Difficulty::Features trivial;
    trivial.rowsLeft = 1;
    // One clean row to go is as easy as it gets.
    QCOMPARE(Difficulty::rate(trivial), Difficulty::kMin);
    Difficulty::Features hopeless = someMess();
    hopeless.holes = 60;
    hopeless.cover = 400;
    hopeless.height = Board::kVisibleHeight;
    QCOMPARE(Difficulty::rate(hopeless), Difficulty::kMax);
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
    QVERIFY(lowest > Difficulty::kMin && lowest < 30);
    QVERIFY(highest > 60 && highest < 90);
}

void DifficultyTests::theGameRatesEverySettledBoard() {
    QCOMPARE(Game(Mode::Zen, kSeed).difficulty(), 0);

    Game game(Mode::Challenge, kSeed);
    QCOMPARE(game.difficulty(), Difficulty::rate(game.board(), game.dealtStack()));
    QVERIFY(game.difficulty() > Difficulty::kMin);
    // Pieces piled into the middle only make it worse, one rating per piece.
    int previous = game.difficulty();
    for (int i = 0; i < 4 && game.phase() == Phase::Playing; ++i) {
        game.hardDrop();
        waitOutTheFlash(game);
        QCOMPARE(game.difficulty(), Difficulty::rate(game.board(), game.dealtStack()));
        QVERIFY(game.difficulty() >= previous);
        previous = game.difficulty();
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
