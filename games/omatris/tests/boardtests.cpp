#include "boardtests.h"

#include <QtTest>

#include "enginefixture.h"

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

void BoardTests::gravityFollowsTheGuidelineCurve() {
    // Level 1 is one row a second; every level after that is quicker, until
    // the curve stops at level 20.
    Game game(Mode::Marathon, kSeed);
    const int row = game.piece().origin.y();
    for (int i = 0; i < Rules::kTicksPerSecond - 1; ++i)
        game.tick();
    QCOMPARE(game.piece().origin.y(), row);
    game.tick();
    QCOMPARE(game.piece().origin.y(), row + 1);

    for (int level = Rules::kFirstLevel; level < Rules::kMaxGravityLevel; ++level)
        QVERIFY(Rules::gravityPerTick(level + 1) > Rules::gravityPerTick(level));
    QVERIFY(Rules::gravityPerTick(15) >= Rules::kGravityUnit);
    QCOMPARE(Rules::gravityPerTick(Rules::kMaxGravityLevel + 10), Rules::gravityPerTick(Rules::kMaxGravityLevel));
}

void BoardTests::softDropIsTwentyTimesGravityAndPaysACell() {
    Game game(Mode::Marathon, kSeed);
    game.setSoftDrop(true);
    const int row = game.piece().origin.y();
    // Twenty times one row a second is a row every three ticks.
    for (int i = 0; i < 3; ++i)
        game.tick();
    QCOMPARE(game.piece().origin.y(), row + 1);
    QCOMPARE(game.score(), Rules::kSoftDropPoints);
    game.setSoftDrop(false);
    for (int i = 0; i < 3; ++i)
        game.tick();
    QCOMPARE(game.piece().origin.y(), row + 1);
    QCOMPARE(game.score(), Rules::kSoftDropPoints);
}

void BoardTests::hardDropPaysTwoACellAndLocksAtOnce() {
    Game game(Mode::Marathon, kSeed);
    game.placePiece({PieceType::O, 0, {3, Game::kSpawnRow}});
    const std::vector<Event> events = game.hardDrop();
    const int fallen = Board::kHeight - 2 - Game::kSpawnRow;
    QCOMPARE(game.score(), fallen * Rules::kHardDropPoints);
    QCOMPARE(count(events, Event::Locked), 1);
    QCOMPARE(find(events, Event::Locked)->cells.size(), size_t(4));
    QCOMPARE(game.board().at({4, kBottom}), PieceType::O);
    QCOMPARE(game.board().at({5, kBottom}), PieceType::O);
}

void BoardTests::lockDelayResetsOnMoveAndCapsAtTheAllowance() {
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

void BoardTests::spinningOnTheSpotCannotOutlastTheLockDelay() {
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

void BoardTests::shiftingAtAWallCannotOutlastTheLockDelay() {
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

void BoardTests::fallingToANewLowestRowRenewsTheAllowance() {
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

void BoardTests::holdSwapsOncePerPiece() {
    Game game(Mode::Marathon, kSeed);
    QCOMPARE(game.heldPiece(), PieceType::None);
    QVERIFY(game.holdAvailable());
    const PieceType first = game.piece().type;
    const PieceType queued = game.nextQueue().front();
    QCOMPARE(int(game.nextQueue().size()), Rules::kNextQueue);

    QCOMPARE(count(game.hold(), Event::Held), 1);
    QCOMPARE(game.heldPiece(), first);
    QCOMPARE(game.piece().type, queued);
    QVERIFY(!game.holdAvailable());
    // A second hold on the same piece does nothing at all.
    const PieceType current = game.piece().type;
    QVERIFY(game.hold().empty());
    QCOMPARE(game.piece().type, current);
    QCOMPARE(game.heldPiece(), first);

    game.hardDrop();
    QVERIFY(game.holdAvailable());
    const PieceType now = game.piece().type;
    game.hold();
    QCOMPARE(game.piece().type, first);
    QCOMPARE(game.heldPiece(), now);
    // A held piece comes back in its spawn orientation.
    QCOMPARE(game.piece().rotation, 0);
    QCOMPARE(game.piece().origin.y(), Game::kSpawnRow);
}

void BoardTests::ghostLandsOnTheStack() {
    Game game(Mode::Zen, kSeed);
    fillRow(game.mutableBoard(), kBottom, {});
    game.placePiece({PieceType::O, 0, {3, Game::kSpawnRow}});
    QCOMPARE(game.ghost().origin, QPoint(3, Board::kHeight - 3));
    QCOMPARE(game.ghost().type, PieceType::O);
    game.moveLeft();
    QCOMPARE(game.ghost().origin.x(), 2);
}

void BoardTests::lineClearFlashesThenCascades() {
    Game game(Mode::Marathon, kSeed);
    Board &board = game.mutableBoard();
    fillRow(board, kBottom, {0, 1, 2, 3});
    board.set({9, kBottom - 1}, PieceType::Z);
    game.placePiece({PieceType::I, 0, {0, Game::kSpawnRow}});
    const std::vector<Event> events = game.hardDrop();

    const Event *cleared = find(events, Event::LinesCleared);
    QVERIFY(cleared);
    QCOMPARE(int(cleared->rows.size()), 1);
    QCOMPARE(cleared->rows.front(), kBottom);
    QCOMPARE(cleared->clear.lines, 1);
    // The row stays on the board while it flashes.
    QCOMPARE(int(game.clearingRows().size()), 1);
    QVERIFY(!game.hasPiece());
    QCOMPARE(game.board().at({5, kBottom}), PieceType::L);
    waitOutTheFlash(game);
    QVERIFY(game.clearingRows().empty());
    QVERIFY(game.hasPiece());
    // Everything above the cleared row fell one.
    QCOMPARE(game.board().at({9, kBottom}), PieceType::Z);
    QCOMPARE(game.board().at({9, kBottom - 1}), PieceType::None);
    QCOMPARE(game.lines(), 1);
}

void BoardTests::blockOutEndsTheGame() {
    Game game(Mode::Marathon, kSeed);
    Board &board = game.mutableBoard();
    // The three columns every piece spawns over, blocked in both entry rows.
    for (int y = Game::kSpawnRow; y < Game::kSpawnRow + 2; ++y) {
        for (int x = 3; x <= 5; ++x)
            board.set({x, y}, PieceType::S);
    }
    game.placePiece({PieceType::O, 0, {3, 18}});
    const std::vector<Event> events = game.hardDrop();
    QCOMPARE(count(events, Event::TopOut), 1);
    QCOMPARE(game.phase(), Phase::GameOver);
    QVERIFY(!game.hasPiece());
    QVERIFY(game.tick().empty());
}

void BoardTests::lockOutEndsTheGame() {
    Game game(Mode::Marathon, kSeed);
    Board &board = game.mutableBoard();
    // A stack right up to the ceiling, one column short of clearing anything.
    for (int y = Board::kHiddenRows; y < Board::kHeight; ++y)
        fillRow(board, y, {9});
    game.placePiece({PieceType::O, 0, {3, Game::kSpawnRow}});
    const std::vector<Event> events = game.hardDrop();
    QCOMPARE(count(events, Event::Locked), 1);
    QCOMPARE(count(events, Event::TopOut), 1);
    QCOMPARE(game.phase(), Phase::GameOver);
    QCOMPARE(game.score(), 0);
}
