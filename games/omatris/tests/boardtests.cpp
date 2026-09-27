#include "boardtests.h"

#include <QtTest>

#include "enginefixture.h"

using namespace EngineFixture;

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

void BoardTests::softDropFollowsTheChosenFactor() {
    Game game(Mode::Marathon, kSeed);
    QCOMPARE(game.softDropFactor(), Rules::kSoftDropFactor);
    game.setSoftDropFactor(40);
    game.setSoftDrop(true);
    const int row = game.piece().origin.y();
    // Forty rows a second is two rows every three ticks.
    for (int i = 0; i < 3; ++i)
        game.tick();
    QCOMPARE(game.piece().origin.y(), row + 2);
    QCOMPARE(game.score(), 2 * Rules::kSoftDropPoints);
}

void BoardTests::instantSoftDropReachesTheFloorWithoutLocking() {
    Game game(Mode::Marathon, kSeed);
    game.setSoftDropFactor(0);
    game.setSoftDrop(true);
    const int row = game.piece().origin.y();
    const int floor = game.ghost().origin.y();
    const std::vector<Event> events = game.tick();
    QCOMPARE(game.piece().origin.y(), floor);
    QCOMPARE(game.score(), (floor - row) * Rules::kSoftDropPoints);
    // Unlike a hard drop the piece is still in play, its lock delay running.
    QCOMPARE(count(events, Event::Locked), 0);
    QVERIFY(game.hasPiece());
    QCOMPARE(game.lockTicks(), 1);
    QVERIFY(game.moveLeft());
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
