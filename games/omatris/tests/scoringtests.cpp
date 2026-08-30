#include "scoringtests.h"

#include <QtTest>

#include "bonuses.h"
#include "enginefixture.h"

using namespace EngineFixture;

namespace {

// Four rows open only at column 0, wiped by an I stood on end in that column.
std::vector<Event> tetrisAtTheLeftWall(Game &game) {
    for (int y = Board::kHeight - 4; y < Board::kHeight; ++y)
        fillRow(game.mutableBoard(), y, {0});
    game.placePiece({PieceType::I, 1, {-2, 0}});
    return game.hardDrop();
}

}  // namespace

void ScoringTests::scoringPaysTheGuidelineTable() {
    // The bar starts with its lowest cell on row kSpawnRow + 2 and lands on
    // the bottom row, whichever rows below it are already filled.
    const int fallen = kBottom - (Game::kSpawnRow + 2);
    for (int lines = 1; lines <= 4; ++lines) {
        Game game(Mode::Marathon, kSeed);
        Board &board = game.mutableBoard();
        for (int y = Board::kHeight - lines; y < Board::kHeight; ++y)
            fillRow(board, y, {0});
        game.placePiece({PieceType::I, 1, {-2, Game::kSpawnRow - 1}});
        const std::vector<Event> events = game.hardDrop();
        const Event *cleared = find(events, Event::LinesCleared);
        QVERIFY(cleared);
        QCOMPARE(cleared->clear.lines, lines);
        QCOMPARE(cleared->clear.spin, Spin::None);
        QCOMPARE(cleared->clear.combo, 0);
        QVERIFY(!cleared->clear.backToBack);
        QCOMPARE(cleared->clear.points, Rules::clearPoints(lines, Spin::None));
        QCOMPARE(game.score(), Rules::clearPoints(lines, Spin::None) + fallen * Rules::kHardDropPoints);
        QCOMPARE(game.lines(), lines);
    }
    QCOMPARE(Rules::clearPoints(1, Spin::None), 100);
    QCOMPARE(Rules::clearPoints(2, Spin::None), 300);
    QCOMPARE(Rules::clearPoints(3, Spin::None), 500);
    QCOMPARE(Rules::clearPoints(4, Spin::None), 800);
}

void ScoringTests::backToBackAndComboStack() {
    Game game(Mode::Zen, kSeed);
    const Event *first = find(tetrisAtTheLeftWall(game), Event::LinesCleared);
    QVERIFY(first);
    QVERIFY(!first->clear.backToBack);
    QCOMPARE(first->clear.combo, 0);
    QCOMPARE(first->clear.points, Rules::kTetris);
    QVERIFY(game.backToBack());
    waitOutTheFlash(game);

    // A second Tetris straight after: half again for the chain, plus the combo.
    const Event *second = find(tetrisAtTheLeftWall(game), Event::LinesCleared);
    QVERIFY(second);
    QVERIFY(second->clear.backToBack);
    QCOMPARE(second->clear.combo, 1);
    QCOMPARE(second->clear.points, Rules::kTetris * 3 / 2 + Rules::kComboStep);
    waitOutTheFlash(game);

    // A plain single keeps the combo going but breaks the chain.
    fillRow(game.mutableBoard(), kBottom, {0});
    game.placePiece({PieceType::I, 1, {-2, 0}});
    const Event *third = find(game.hardDrop(), Event::LinesCleared);
    QVERIFY(third);
    QVERIFY(!third->clear.backToBack);
    QCOMPARE(third->clear.combo, 2);
    QCOMPARE(third->clear.points, Rules::kSingle + 2 * Rules::kComboStep);
    QVERIFY(!game.backToBack());
    waitOutTheFlash(game);

    // Locking a piece with no clear ends the combo.
    game.placePiece({PieceType::O, 0, {3, 0}});
    const Event *locked = find(game.hardDrop(), Event::Locked);
    QVERIFY(locked);
    QCOMPARE(locked->clear.lines, 0);
    QCOMPARE(game.combo(), -1);
}

void ScoringTests::tSpinTripleScoresAsAFullSpin() {
    Game game(Mode::Zen, kSeed);
    Board &board = game.mutableBoard();
    // The standard triple slot: a three-deep well in column 3 with a notch to
    // its right in the middle row, and one block overhanging the entrance so
    // the only way in is the last kick of the table.
    fillRow(board, 21, {3});
    fillRow(board, 22, {3, 4});
    fillRow(board, 23, {3});
    board.set({3, 19}, PieceType::J);

    game.placePiece({PieceType::T, 0, {3, 19}});
    QVERIFY(game.rotate(1));
    QCOMPARE(game.piece().origin, QPoint(2, 21));
    const std::vector<Event> events = game.hardDrop();
    const Event *cleared = find(events, Event::LinesCleared);
    QVERIFY(cleared);
    QCOMPARE(cleared->clear.lines, 3);
    QCOMPARE(cleared->clear.spin, Spin::Full);
    QCOMPARE(cleared->clear.points, Rules::kTSpinTriple);
    QCOMPARE(game.score(), Rules::kTSpinTriple);
}

void ScoringTests::tSpinMiniIsToldFromAFullOne() {
    Game game(Mode::Zen, kSeed);
    // Three corners filled but only one of the two the T points at: a mini.
    game.mutableBoard().set({8, 23}, PieceType::S);
    game.placePiece({PieceType::T, 0, {7, 21}});
    QVERIFY(game.rotate(-1));
    QCOMPARE(game.piece().origin, QPoint(8, 21));
    const Event *locked = find(game.hardDrop(), Event::Locked);
    QVERIFY(locked);
    QCOMPARE(locked->clear.spin, Spin::Mini);
    QCOMPARE(locked->clear.lines, 0);
    QCOMPARE(locked->clear.points, Rules::kTSpinMini);

    // The same corner, one line to wipe: a mini single.
    Game single(Mode::Zen, kSeed);
    fillRow(single.mutableBoard(), 23, {9});
    single.placePiece({PieceType::T, 0, {7, 21}});
    QVERIFY(single.rotate(-1));
    const Event *cleared = find(single.hardDrop(), Event::LinesCleared);
    QVERIFY(cleared);
    QCOMPARE(cleared->clear.spin, Spin::Mini);
    QCOMPARE(cleared->clear.points, Rules::kTSpinMiniSingle);

    // Dropping a T into the same hole without turning is no spin at all.
    Game dropped(Mode::Zen, kSeed);
    dropped.mutableBoard().set({8, 23}, PieceType::S);
    dropped.placePiece({PieceType::T, 3, {8, 21}});
    const Event *plain = find(dropped.hardDrop(), Event::Locked);
    QVERIFY(plain);
    QCOMPARE(plain->clear.spin, Spin::None);
    QCOMPARE(plain->clear.points, 0);
}

void ScoringTests::levelRisesEveryTenLines() {
    Game game(Mode::Marathon, kSeed);
    QCOMPARE(game.level(), Rules::kFirstLevel);
    for (int tetris = 0; tetris < 3; ++tetris) {
        const std::vector<Event> events = tetrisAtTheLeftWall(game);
        const Event *up = find(events, Event::LevelUp);
        if (tetris < 2) {
            QCOMPARE(game.level(), Rules::kFirstLevel);
            QVERIFY(!up);
        } else {
            QCOMPARE(game.level(), Rules::kFirstLevel + 1);
            QVERIFY(up);
            QCOMPARE(up->level, Rules::kFirstLevel + 1);
        }
        waitOutTheFlash(game);
    }
    QCOMPARE(game.lines(), 12);
    QCOMPARE(game.gravityLevel(), 2);
}

void ScoringTests::sprintFinishesAtFortyLinesAndSprintZenNeverRamp() {
    Game sprint(Mode::Sprint, kSeed);
    QCOMPARE(sprint.lineGoal(), Rules::kSprintLines);
    QCOMPARE(sprint.linesLeft(), Rules::kSprintLines);
    std::vector<Event> last;
    for (int tetris = 0; tetris < 10; ++tetris) {
        last = tetrisAtTheLeftWall(sprint);
        waitOutTheFlash(sprint);
    }
    QCOMPARE(sprint.lines(), Rules::kSprintLines);
    QCOMPARE(sprint.linesLeft(), 0);
    QCOMPARE(sprint.phase(), Phase::Finished);
    QCOMPARE(count(last, Event::Finished), 1);
    QVERIFY(sprint.elapsedMs() > 0);
    // Four levels up, and gravity still at the first level's pace.
    QCOMPARE(sprint.level(), Rules::kFirstLevel + 4);
    QCOMPARE(sprint.gravityLevel(), Rules::kFirstLevel);
    // Nothing moves after the finish.
    const int ticks = sprint.ticks();
    sprint.tick();
    QCOMPARE(sprint.ticks(), ticks);

    Game zen(Mode::Zen, kSeed);
    for (int tetris = 0; tetris < 3; ++tetris) {
        tetrisAtTheLeftWall(zen);
        waitOutTheFlash(zen);
    }
    QCOMPARE(zen.level(), Rules::kFirstLevel + 1);
    QCOMPARE(zen.gravityLevel(), Rules::kFirstLevel);
    QCOMPARE(zen.phase(), Phase::Playing);
}

// The popups a clear earns are what the player is told about it, and a mini
// T-spin has to read differently from a full one.
void ScoringTests::popupsNameTheClearAndTheStreaks() {
    QVERIFY(Bonuses::texts({1, Spin::None, false, -1, 0}).isEmpty());
    QVERIFY(Bonuses::texts({3, Spin::None, false, -1, 0}).isEmpty());
    QCOMPARE(Bonuses::texts({4, Spin::None, false, -1, 0}), QStringList {QStringLiteral("Tetris")});
    QCOMPARE(Bonuses::texts({0, Spin::Mini, false, -1, 0}), QStringList {QStringLiteral("T-Spin Mini")});
    QCOMPARE(Bonuses::texts({2, Spin::Mini, false, -1, 0}),
             QStringList {QStringLiteral("T-Spin Mini Double")});
    QCOMPARE(Bonuses::texts({3, Spin::Full, false, -1, 0}), QStringList {QStringLiteral("T-Spin Triple")});
    // The streaks rise after the name, in the order they are earned.
    QCOMPARE(Bonuses::texts({4, Spin::None, true, 2, 0}),
             QStringList({QStringLiteral("Tetris"), QStringLiteral("Back-to-Back"),
                          QStringLiteral("Combo x2")}));
    // The first clear of a chain is no combo at all.
    QCOMPARE(Bonuses::texts({1, Spin::None, false, 0, 0}), QStringList {});
}
