#include "enginetests.h"

#include <QtTest>
#include <algorithm>

#include "game.h"

namespace {

constexpr quint32 kSeed = 20260830u;
constexpr int kBottom = Board::kHeight - 1;

QString shape(const PieceCells &cells) {
    QStringList parts;
    for (QPoint cell : cells)
        parts << QStringLiteral("%1,%2").arg(cell.x()).arg(cell.y());
    return parts.join(QLatin1Char(' '));
}

QString spawnShape(PieceType type, int rotation = 0) {
    return shape(Placement {type, rotation, {0, 0}}.cells());
}

QString offsets(const std::array<QPoint, Piece::kMaxKicks> &kicks) {
    QStringList parts;
    for (QPoint kick : kicks)
        parts << QStringLiteral("%1,%2").arg(kick.x()).arg(kick.y());
    return parts.join(QLatin1Char(' '));
}

int count(const std::vector<Event> &events, Event::Type type) {
    return int(std::count_if(events.begin(), events.end(), [type](const Event &e) { return e.type == type; }));
}

const Event *find(const std::vector<Event> &events, Event::Type type) {
    for (const Event &event : events) {
        if (event.type == type)
            return &event;
    }
    return nullptr;
}

// Fills a row solid except for the named columns.
void fillRow(Board &board, int y, std::initializer_list<int> gaps) {
    for (int x = 0; x < Board::kWidth; ++x) {
        if (std::find(gaps.begin(), gaps.end(), x) == gaps.end())
            board.set({x, y}, PieceType::L);
    }
}

void waitOutTheFlash(Game &game) {
    for (int i = 0; i < Rules::kClearDelayTicks; ++i)
        game.tick();
}

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

// Four rows open only at column 0, wiped by an I stood on end in that column.
std::vector<Event> tetrisAtTheLeftWall(Game &game) {
    for (int y = Board::kHeight - 4; y < Board::kHeight; ++y)
        fillRow(game.mutableBoard(), y, {0});
    game.placePiece({PieceType::I, 1, {-2, 0}});
    return game.hardDrop();
}

}  // namespace

void EngineTests::spawnOrientationsMatchTheGuideline() {
    QCOMPARE(spawnShape(PieceType::I), QStringLiteral("0,1 1,1 2,1 3,1"));
    QCOMPARE(spawnShape(PieceType::J), QStringLiteral("0,0 0,1 1,1 2,1"));
    QCOMPARE(spawnShape(PieceType::L), QStringLiteral("2,0 0,1 1,1 2,1"));
    QCOMPARE(spawnShape(PieceType::O), QStringLiteral("1,0 2,0 1,1 2,1"));
    QCOMPARE(spawnShape(PieceType::S), QStringLiteral("1,0 2,0 0,1 1,1"));
    QCOMPARE(spawnShape(PieceType::T), QStringLiteral("1,0 0,1 1,1 2,1"));
    QCOMPARE(spawnShape(PieceType::Z), QStringLiteral("0,0 1,0 1,1 2,1"));

    // Every piece enters centred, and I and O in a four-wide box.
    QCOMPARE(Piece::boxSize(PieceType::I), 4);
    QCOMPARE(Piece::boxSize(PieceType::O), 4);
    QCOMPARE(Piece::boxSize(PieceType::T), 3);
    for (int i = 0; i < kPieceCount; ++i)
        QCOMPARE(Piece::spawnColumn(PieceType(i), Board::kWidth), 3);
}

void EngineTests::rotationStatesCycleBothWays() {
    QCOMPARE(spawnShape(PieceType::T, 1), QStringLiteral("1,0 1,1 2,1 1,2"));
    QCOMPARE(spawnShape(PieceType::T, 2), QStringLiteral("0,1 1,1 2,1 1,2"));
    QCOMPARE(spawnShape(PieceType::T, 3), QStringLiteral("1,0 0,1 1,1 1,2"));
    QCOMPARE(spawnShape(PieceType::I, 1), QStringLiteral("2,0 2,1 2,2 2,3"));
    QCOMPARE(spawnShape(PieceType::I, 3), QStringLiteral("1,0 1,1 1,2 1,3"));
    QCOMPARE(spawnShape(PieceType::S, 1), QStringLiteral("1,0 1,1 2,1 2,2"));
    QCOMPARE(spawnShape(PieceType::Z, 3), QStringLiteral("1,0 0,1 1,1 0,2"));
    QCOMPARE(spawnShape(PieceType::J, 2), QStringLiteral("0,1 1,1 2,1 2,2"));
    QCOMPARE(spawnShape(PieceType::L, 3), QStringLiteral("0,0 1,0 1,1 1,2"));

    QCOMPARE(Piece::turn(0, -1), 3);
    QCOMPARE(Piece::turn(3, 1), 0);
    // Four turns either way is where it started, for every piece.
    for (int i = 0; i < kPieceCount; ++i) {
        const PieceType type = PieceType(i);
        Game game(Mode::Zen, kSeed);
        game.placePiece({type, 0, {3, 8}});
        for (int quarter = 0; quarter < 4; ++quarter)
            QVERIFY(game.rotate(1));
        QCOMPARE(game.piece().rotation, 0);
        QCOMPARE(shape(game.piece().cells()), shape(Placement {type, 0, {3, 8}}.cells()));
        for (int quarter = 0; quarter < 4; ++quarter)
            QVERIFY(game.rotate(-1));
        QCOMPARE(game.piece().rotation, 0);
    }
}

void EngineTests::kickTablesMatchTheGuideline() {
    // The tables exactly as the guideline prints them: x right, y *up*, in the
    // order 0->R, 0->L, R->2, R->0, 2->L, 2->R, L->0, L->2.
    const int published[2][8][Piece::kMaxKicks][2] = {
        {{{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}},
         {{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}},
         {{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}},
         {{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}},
         {{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}},
         {{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}},
         {{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}},
         {{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}}},
        {{{0, 0}, {-2, 0}, {1, 0}, {-2, -1}, {1, 2}},
         {{0, 0}, {-1, 0}, {2, 0}, {-1, 2}, {2, -1}},
         {{0, 0}, {-1, 0}, {2, 0}, {-1, 2}, {2, -1}},
         {{0, 0}, {2, 0}, {-1, 0}, {2, 1}, {-1, -2}},
         {{0, 0}, {2, 0}, {-1, 0}, {2, 1}, {-1, -2}},
         {{0, 0}, {1, 0}, {-2, 0}, {1, -2}, {-2, 1}},
         {{0, 0}, {1, 0}, {-2, 0}, {1, -2}, {-2, 1}},
         {{0, 0}, {-2, 0}, {1, 0}, {-2, -1}, {1, 2}}},
    };
    const int fromState[8] = {0, 0, 1, 1, 2, 2, 3, 3};
    const int toState[8] = {1, 3, 2, 0, 3, 1, 0, 2};
    const PieceType types[2] = {PieceType::T, PieceType::I};

    for (int table = 0; table < 2; ++table) {
        for (int row = 0; row < 8; ++row) {
            std::array<QPoint, Piece::kMaxKicks> expected;
            for (int k = 0; k < Piece::kMaxKicks; ++k)
                expected[size_t(k)] = QPoint(published[table][row][k][0], -published[table][row][k][1]);
            QCOMPARE(offsets(Piece::kicks(types[table], fromState[row], toState[row])), offsets(expected));
        }
    }
    // The O piece has no kicks at all.
    QCOMPARE(offsets(Piece::kicks(PieceType::O, 0, 1)), QStringLiteral("0,0 0,0 0,0 0,0 0,0"));
}

void EngineTests::iPieceKicksOffAWall() {
    Game game(Mode::Zen, kSeed);
    // Upright in the leftmost column, laid flat: turning in place would put
    // two cells off the board, so the third kick shifts it two to the right.
    game.placePiece({PieceType::I, 1, {-2, 10}});
    QCOMPARE(shape(game.piece().cells()), QStringLiteral("0,10 0,11 0,12 0,13"));
    QVERIFY(game.rotate(1));
    QCOMPARE(game.piece().origin, QPoint(0, 10));
    QCOMPARE(shape(game.piece().cells()), QStringLiteral("0,12 1,12 2,12 3,12"));

    // The same against the right wall: one to the left is enough.
    game.placePiece({PieceType::I, 1, {7, 10}});
    QCOMPARE(shape(game.piece().cells()), QStringLiteral("9,10 9,11 9,12 9,13"));
    QVERIFY(game.rotate(1));
    QCOMPARE(shape(game.piece().cells()), QStringLiteral("6,12 7,12 8,12 9,12"));

    // And off the floor: flat on the bottom row, standing up needs the last
    // kick in the table, which lifts it two rows.
    game.placePiece({PieceType::I, 0, {3, Board::kHeight - 2}});
    QCOMPARE(shape(game.piece().cells()), QStringLiteral("3,23 4,23 5,23 6,23"));
    QVERIFY(game.rotate(1));
    QCOMPARE(shape(game.piece().cells()), QStringLiteral("6,20 6,21 6,22 6,23"));
}

void EngineTests::tPieceKicksAgainstBothWalls() {
    Game game(Mode::Zen, kSeed);
    // Flush against the left wall pointing right; turning it face-down would
    // push a cell off the board, so the first kick steps it one to the right.
    game.placePiece({PieceType::T, 1, {-1, 10}});
    QCOMPARE(shape(game.piece().cells()), QStringLiteral("0,10 0,11 1,11 0,12"));
    QVERIFY(game.rotate(1));
    QCOMPARE(game.piece().origin, QPoint(0, 10));
    QCOMPARE(shape(game.piece().cells()), QStringLiteral("0,11 1,11 2,11 1,12"));

    // Mirrored against the right wall, turning the other way.
    game.placePiece({PieceType::T, 3, {8, 10}});
    QCOMPARE(shape(game.piece().cells()), QStringLiteral("9,10 8,11 9,11 9,12"));
    QVERIFY(game.rotate(-1));
    QCOMPARE(shape(game.piece().cells()), QStringLiteral("7,11 8,11 9,11 8,12"));

    // On the floor the T climbs out diagonally: the third kick.
    fillRow(game.mutableBoard(), kBottom, {});
    game.placePiece({PieceType::T, 0, {3, Board::kHeight - 3}});
    QVERIFY(game.rotate(1));
    QCOMPARE(game.piece().origin, QPoint(2, Board::kHeight - 4));
    QCOMPARE(shape(game.piece().cells()), QStringLiteral("3,20 3,21 4,21 3,22"));
}

void EngineTests::oPieceNeverMovesWhenTurned() {
    Game game(Mode::Zen, kSeed);
    game.placePiece({PieceType::O, 0, {3, 10}});
    const QString before = shape(game.piece().cells());
    for (int turn = 0; turn < 4; ++turn) {
        QVERIFY(game.rotate(1));
        QCOMPARE(shape(game.piece().cells()), before);
    }
}

void EngineTests::sevenBagDealsEveryPieceOncePerSeven() {
    Bag bag(kSeed);
    int totals[kPieceCount] = {};
    for (int round = 0; round < 100; ++round) {
        int seen[kPieceCount] = {};
        for (int i = 0; i < kPieceCount; ++i) {
            const PieceType piece = bag.take();
            ++seen[int(piece)];
            ++totals[int(piece)];
        }
        for (int i = 0; i < kPieceCount; ++i)
            QCOMPARE(seen[i], 1);
    }
    for (int i = 0; i < kPieceCount; ++i)
        QCOMPARE(totals[i], 100);

    // The queue can always be read ahead without disturbing the deal.
    Bag peeked(kSeed);
    const PieceType first = peeked.peek(0);
    const PieceType third = peeked.peek(2);
    QCOMPARE(peeked.take(), first);
    peeked.take();
    QCOMPARE(peeked.take(), third);
}

void EngineTests::sameSeedPlaysTheSameGame() {
    Game a(Mode::Marathon, kSeed);
    Game b(Mode::Marathon, kSeed);
    Game other(Mode::Marathon, kSeed + 1);
    QStringList first;
    QStringList second;
    QStringList different;
    for (int i = 0; i < 30; ++i) {
        first << shape(a.piece().cells());
        second << shape(b.piece().cells());
        different << shape(other.piece().cells());
        a.hardDrop();
        b.hardDrop();
        other.hardDrop();
        waitOutTheFlash(a);
        waitOutTheFlash(b);
        waitOutTheFlash(other);
    }
    QCOMPARE(first, second);
    QCOMPARE(a.score(), b.score());
    QVERIFY(first != different);
}

void EngineTests::gravityFollowsTheGuidelineCurve() {
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

void EngineTests::softDropIsTwentyTimesGravityAndPaysACell() {
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

void EngineTests::hardDropPaysTwoACellAndLocksAtOnce() {
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

void EngineTests::lockDelayResetsOnMoveAndCapsAtTheAllowance() {
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

void EngineTests::spinningOnTheSpotCannotOutlastTheLockDelay() {
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

void EngineTests::shiftingAtAWallCannotOutlastTheLockDelay() {
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

void EngineTests::fallingToANewLowestRowRenewsTheAllowance() {
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

void EngineTests::holdSwapsOncePerPiece() {
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

void EngineTests::ghostLandsOnTheStack() {
    Game game(Mode::Zen, kSeed);
    fillRow(game.mutableBoard(), kBottom, {});
    game.placePiece({PieceType::O, 0, {3, Game::kSpawnRow}});
    QCOMPARE(game.ghost().origin, QPoint(3, Board::kHeight - 3));
    QCOMPARE(game.ghost().type, PieceType::O);
    game.moveLeft();
    QCOMPARE(game.ghost().origin.x(), 2);
}

void EngineTests::lineClearFlashesThenCascades() {
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

void EngineTests::scoringPaysTheGuidelineTable() {
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

void EngineTests::backToBackAndComboStack() {
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

void EngineTests::tSpinTripleScoresAsAFullSpin() {
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

void EngineTests::tSpinMiniIsToldFromAFullOne() {
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

void EngineTests::levelRisesEveryTenLines() {
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

void EngineTests::sprintFinishesAtFortyLinesAndSprintZenNeverRamp() {
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

void EngineTests::blockOutEndsTheGame() {
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

void EngineTests::lockOutEndsTheGame() {
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
