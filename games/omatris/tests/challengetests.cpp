#include "challengetests.h"

#include <QtTest>

#include "challenge.h"
#include "dealtstack.h"
#include "enginefixture.h"

using namespace EngineFixture;

namespace {

constexpr int kSeedsTried = 300;

int cellsIn(const Board &board, int y) {
    int cells = 0;
    for (int x = 0; x < Board::kWidth; ++x)
        cells += board.at({x, y}) != PieceType::None ? 1 : 0;
    return cells;
}

// Every dealt row solid but for column 0, so each I stood on end there
// clears the bottom four of whatever is left.
void openOnlyTheLeftColumn(Game &game) {
    for (int y = Board::kHeight - game.dealtRowsLeft(); y < Board::kHeight; ++y)
        fillRow(game.mutableBoard(), y, {0});
}

std::vector<Event> iDownTheLeftWall(Game &game) {
    game.placePiece({PieceType::I, 1, {-2, 0}});
    return game.hardDrop();
}

}  // namespace

// A fifth to a half of the well, from the floor up, no row full or empty, and
// nothing but whole pieces: the stack could have been played.
void ChallengeTests::dealtStackLooksPlayed() {
    for (quint32 seed = 0; seed < kSeedsTried; ++seed) {
        Board board;
        const std::vector<int> rows = Challenge::build(board, seed);
        const int count = int(rows.size());
        QVERIFY(count >= Challenge::kMinRows && count <= Challenge::kMaxRows);
        QCOMPARE(rows.back(), kBottom);
        int filled = 0;
        for (int i = 0; i < count; ++i) {
            QCOMPARE(rows[size_t(i)], Board::kHeight - count + i);
            const int cells = cellsIn(board, rows[size_t(i)]);
            QVERIFY(cells > 0 && cells < Board::kWidth);
            filled += cells;
        }
        for (int y = 0; y < Board::kHeight - count; ++y)
            QCOMPARE(cellsIn(board, y), 0);
        QCOMPARE(filled % 4, 0);
        // Tough, not token: at least half of what it covers is filled.
        QVERIFY(filled * 2 >= count * Board::kWidth);
    }
    // The dealt sizes span the range rather than sitting at one end of it.
    int smallest = Challenge::kMaxRows;
    int largest = Challenge::kMinRows;
    for (quint32 seed = 0; seed < kSeedsTried; ++seed) {
        Board board;
        const int count = int(Challenge::build(board, seed).size());
        smallest = std::min(smallest, count);
        largest = std::max(largest, count);
    }
    QCOMPARE(smallest, Challenge::kMinRows);
    QCOMPARE(largest, Challenge::kMaxRows);
}

void ChallengeTests::theSameSeedDealsTheSameMess() {
    Game first(Mode::Challenge, kSeed);
    Game second(Mode::Challenge, kSeed);
    for (int i = 0; i < Board::kCellCount; ++i)
        QCOMPARE(first.board().at(i), second.board().at(i));
    QVERIFY(!first.board().empty());
    QCOMPARE(first.dealtRows(), first.dealtRowsLeft());
    QVERIFY(first.hasPiece());
    QCOMPARE(first.phase(), Phase::Playing);

    // Only Challenge deals one.
    QVERIFY(Game(Mode::Zen, kSeed).board().empty());
    QCOMPARE(Game(Mode::Zen, kSeed).dealtRows(), 0);
}

void ChallengeTests::clearedRowsLeaveAndTheRestFall() {
    const std::vector<int> tracked {18, 19, 20, 21, 22, 23};
    // 23 and 20 go; 22 and 21 fall one, past 23; 19 and 18 fall two.
    QCOMPARE(Challenge::afterClear(tracked, {20, 23}), (std::vector<int> {20, 21, 22, 23}));
    // A row cleared above them all moves none of them.
    QCOMPARE(Challenge::afterClear(tracked, {10}), tracked);
    QCOMPARE(Challenge::afterClear(tracked, tracked), std::vector<int> {});
}

void ChallengeTests::theStackCountsFlashingRowsAsGone() {
    Board board;
    DealtStack stack(board, kSeed);
    const int dealt = stack.dealt();
    QCOMPARE(stack.rowsLeft({}), dealt);
    QCOMPARE(stack.rowsLeft({kBottom, 0}), dealt - 1);
    QVERIFY(stack.difficulty() > 1);
    // Cleared for real, the bottom row goes and the rest fall onto the floor.
    stack.clear({kBottom});
    QCOMPARE(stack.rowsLeft({}), dealt - 1);
    QCOMPARE(stack.rows().back(), kBottom);
    QCOMPARE(stack.dealt(), dealt);
}

void ChallengeTests::clearingEveryDealtRowFinishesAtZenPace() {
    Game game(Mode::Challenge, kSeed);
    openOnlyTheLeftColumn(game);
    int left = game.dealtRows();
    while (left > 4) {
        const std::vector<Event> events = iDownTheLeftWall(game);
        QCOMPARE(count(events, Event::Finished), 0);
        left -= 4;
        QCOMPARE(game.dealtRowsLeft(), left);
        // The rows stay marked until the flash is over, then the rest fall.
        QCOMPARE(int(game.dealtStack().size()), left + 4);
        waitOutTheFlash(game);
        QCOMPARE(game.phase(), Phase::Playing);
        QCOMPARE(int(game.dealtStack().size()), left);
        QCOMPARE(game.dealtStack().back(), kBottom);
    }
    const std::vector<Event> events = iDownTheLeftWall(game);
    QCOMPARE(count(events, Event::Finished), 1);
    QCOMPARE(game.dealtRowsLeft(), 0);
    QCOMPARE(game.phase(), Phase::Finished);
    QCOMPARE(game.gravityLevel(), Rules::kFirstLevel);
}

// Lines the player builds over the mess are still lines, but they are not the
// mess: the dealt rows stay to be cleared.
void ChallengeTests::aClearAboveTheStackDoesNotCount() {
    Game game(Mode::Challenge, kSeed);
    const int dealt = game.dealtRows();
    const int above = Board::kHeight - dealt - 1;
    fillRow(game.mutableBoard(), above, {0, 1, 2, 3});
    game.placePiece({PieceType::I, 0, {0, above - 1}});
    const std::vector<Event> events = game.hardDrop();
    QCOMPARE(count(events, Event::LinesCleared), 1);
    QCOMPARE(game.lines(), 1);
    QCOMPARE(game.dealtRowsLeft(), dealt);
    waitOutTheFlash(game);
    QCOMPARE(game.phase(), Phase::Playing);
}
