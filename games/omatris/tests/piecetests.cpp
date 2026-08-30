#include "piecetests.h"

#include <QtTest>

#include "enginefixture.h"

using namespace EngineFixture;

namespace {

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

}  // namespace

void PieceTests::spawnOrientationsMatchTheGuideline() {
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

void PieceTests::rotationStatesCycleBothWays() {
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

void PieceTests::kickTablesMatchTheGuideline() {
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

void PieceTests::iPieceKicksOffAWall() {
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

void PieceTests::tPieceKicksAgainstBothWalls() {
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

void PieceTests::oPieceNeverMovesWhenTurned() {
    Game game(Mode::Zen, kSeed);
    game.placePiece({PieceType::O, 0, {3, 10}});
    const QString before = shape(game.piece().cells());
    for (int turn = 0; turn < 4; ++turn) {
        QVERIFY(game.rotate(1));
        QCOMPARE(shape(game.piece().cells()), before);
    }
}

// The hold and next boxes draw a piece on its own, without the empty rows and
// columns of the box it turns inside.
void PieceTests::spawnBoxTrimsAPieceToItsOwnCells() {
    const Piece::SpawnBox bar = Piece::spawnBox(PieceType::I);
    QCOMPARE(bar.width, 4);
    QCOMPARE(bar.height, 1);
    QCOMPARE(shape(bar.cells), QStringLiteral("0,0 1,0 2,0 3,0"));

    const Piece::SpawnBox block = Piece::spawnBox(PieceType::O);
    QCOMPARE(block.width, 2);
    QCOMPARE(block.height, 2);
    QCOMPARE(shape(block.cells), QStringLiteral("0,0 1,0 0,1 1,1"));

    const Piece::SpawnBox tee = Piece::spawnBox(PieceType::T);
    QCOMPARE(tee.width, 3);
    QCOMPARE(tee.height, 2);
    QCOMPARE(shape(tee.cells), QStringLiteral("1,0 0,1 1,1 2,1"));
}

void PieceTests::sevenBagDealsEveryPieceOncePerSeven() {
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

void PieceTests::sameSeedPlaysTheSameGame() {
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
