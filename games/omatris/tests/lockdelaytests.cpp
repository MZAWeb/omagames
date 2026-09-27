#include "lockdelaytests.h"

#include <QtTest>

#include "enginefixture.h"
#include "lockdelay.h"

using namespace EngineFixture;

namespace {

// An L wedged against the right wall under an overhang: turning it clockwise
// kicks it two rows up into the gap, where it hangs without ever reaching a
// row it has not been on, and turning it back drops it into the notch again.
// Real players find spots like this and spin in them.
constexpr int kNotchTop = Board::kHeight - 8;
const char *const kNotch[8] = {
    "........#.",
    "........#.",
    "..........",
    "..........",
    "........#.",
    "........#.",
    "........#.",
    "#########.",
};
const Placement kInTheNotch {PieceType::L, 3, {8, kNotchTop + 3}};

void buildNotch(Board &board) {
    for (int row = 0; row < 8; ++row) {
        for (int x = 0; x < Board::kWidth; ++x) {
            if (kNotch[row][x] == '#')
                board.set({x, kNotchTop + row}, PieceType::L);
        }
    }
}

}  // namespace

// The allowance on its own, rows and ticks only, with no board in sight.
void LockDelayTests::theDelayCountsRestingTicksAndSpendsResets() {
    LockDelay delay;
    delay.start(5);
    // Moves in mid-air before the piece has landed cost nothing.
    delay.moved(5, false);
    QCOMPARE(delay.resets(), 0);
    for (int i = 1; i < Rules::kLockDelayTicks; ++i)
        QVERIFY(!delay.rest());
    // Landed, a move buys the timer back, until the allowance runs out.
    for (int reset = 1; reset <= Rules::kMaxLockResets; ++reset) {
        delay.moved(5, true);
        QCOMPARE(delay.resets(), reset);
        QCOMPARE(delay.ticks(), 0);
    }
    delay.moved(5, true);
    QCOMPARE(delay.resets(), Rules::kMaxLockResets);
    for (int i = 1; i < Rules::kLockDelayTicks; ++i)
        QVERIFY(!delay.rest());
    QVERIFY(delay.rest());
    // Falling back to a row it has been on renews nothing; a new one does.
    delay.fell(5);
    QCOMPARE(delay.resets(), Rules::kMaxLockResets);
    delay.fell(6);
    QCOMPARE(delay.resets(), 0);
    QCOMPARE(delay.ticks(), 0);
}

void LockDelayTests::lockDelayResetsOnMoveAndCapsAtTheAllowance() {
    Game game(Mode::Zen, kSeed);
    game.placePiece({PieceType::O, 0, {3, Board::kHeight - 2}});
    // The piece has to land before a move counts against the allowance.
    QVERIFY(game.tick().empty());
    for (int reset = 0; reset < Rules::kMaxLockResets; ++reset) {
        for (int t = 0; t < Rules::kLockDelayTicks - 2; ++t)
            QVERIFY(game.tick().empty());
        QVERIFY(game.lockTicks() > 0);
        QVERIFY(reset % 2 == 0 ? game.moveLeft() : game.moveRight());
        QCOMPARE(game.lockResets(), reset + 1);
        QCOMPARE(game.lockTicks(), 0);
    }
    // One nudge past the allowance buys no more time: the timer keeps running.
    for (int t = 0; t < Rules::kLockDelayTicks - 1; ++t)
        QVERIFY(game.tick().empty());
    QVERIFY(game.moveLeft());
    QCOMPARE(game.lockResets(), Rules::kMaxLockResets);
    QVERIFY(game.lockTicks() > 0);
    QCOMPARE(count(game.tick(), Event::Locked), 1);
}

void LockDelayTests::spinningOnTheSpotCannotOutlastTheLockDelay() {
    Game game(Mode::Zen, kSeed);
    buildNotch(game.mutableBoard());
    game.placePiece(kInTheNotch);
    // Only the ticks spent resting count against the timer, so those are the
    // ones the allowance is measured in.
    int resting = 0;
    bool locked = false;
    for (int spin = 0; spin < 200 && !locked; ++spin) {
        QVERIFY(game.rotate(1));
        // Hanging in the gap the timer pauses; it must not rewind.
        const int hanging = game.lockTicks();
        QVERIFY(game.tick().empty());
        QCOMPARE(game.lockTicks(), hanging);
        const int spent = game.lockResets();
        QVERIFY(game.rotate(-1));
        QVERIFY(game.lockResets() <= Rules::kMaxLockResets);
        // Once the allowance is gone, dropping back in buys no more time.
        if (spent == Rules::kMaxLockResets)
            QCOMPARE(game.lockTicks(), hanging);
        ++resting;
        locked = count(game.tick(), Event::Locked) == 1;
    }
    // However long the keys are hammered, the piece is down within the
    // allowance and one last lock delay.
    QVERIFY(locked);
    QVERIFY(resting <= (Rules::kMaxLockResets + 1) * Rules::kLockDelayTicks);
}

void LockDelayTests::shiftingAtAWallCannotOutlastTheLockDelay() {
    Game game(Mode::Zen, kSeed);
    game.placePiece({PieceType::O, 0, {-1, Board::kHeight - 2}});
    int ticks = 0;
    bool locked = false;
    for (int nudge = 0; nudge < 200 && !locked; ++nudge) {
        // A shift the wall refuses is not a move and buys nothing at all.
        const int spent = game.lockResets();
        QVERIFY(!game.moveLeft());
        QCOMPARE(game.lockResets(), spent);
        QVERIFY(game.moveRight());
        QVERIFY(game.moveLeft());
        QVERIFY(game.lockResets() <= Rules::kMaxLockResets);
        ++ticks;
        locked = count(game.tick(), Event::Locked) == 1;
    }
    QVERIFY(locked);
    QVERIFY(ticks <= (Rules::kMaxLockResets + 1) * Rules::kLockDelayTicks);
}

void LockDelayTests::fallingToANewLowestRowRenewsTheAllowance() {
    Game game(Mode::Zen, kSeed);
    // A ledge over the left half of the floor, open ground on the right.
    fillRow(game.mutableBoard(), kBottom, {5, 6, 7, 8, 9});
    game.placePiece({PieceType::O, 0, {2, kBottom - 2}});
    QVERIFY(game.tick().empty());
    for (int reset = 0; reset < Rules::kMaxLockResets; ++reset)
        QVERIFY(reset % 2 == 0 ? game.moveLeft() : game.moveRight());
    QCOMPARE(game.lockResets(), Rules::kMaxLockResets);

    // Walk off the ledge and fall: a row the piece has never been on hands it
    // the whole allowance again.
    while (game.ghost().origin.y() == game.piece().origin.y())
        QVERIFY(game.moveRight());
    const int ledge = game.piece().origin.y();
    game.setSoftDrop(true);
    while (game.piece().origin.y() == ledge)
        QVERIFY(game.tick().empty());
    game.setSoftDrop(false);
    // The tick it landed on has already started the timer over again.
    QCOMPARE(game.lockResets(), 0);
    QCOMPARE(game.lockTicks(), 1);
    for (int reset = 0; reset < Rules::kMaxLockResets; ++reset) {
        QVERIFY(reset % 2 == 0 ? game.moveRight() : game.moveLeft());
        QCOMPARE(game.lockResets(), reset + 1);
        QCOMPARE(game.lockTicks(), 0);
    }
}

void LockDelayTests::aSlideToTheWallIsChargedAsOneMove() {
    Game game(Mode::Zen, kSeed);
    game.placePiece({PieceType::O, 0, {3, Board::kHeight - 2}});
    QVERIFY(game.tick().empty());
    QVERIFY(game.slide(-1));
    QCOMPARE(game.piece().origin.x(), -1);
    QCOMPARE(game.lockResets(), 1);
    // Already at the wall there is nowhere to go, and nothing is spent.
    QVERIFY(!game.slide(-1));
    QCOMPARE(game.lockResets(), 1);
    QVERIFY(game.slide(1));
    QCOMPARE(game.piece().origin.x(), Board::kWidth - 3);
    QCOMPARE(game.lockResets(), 2);
}
