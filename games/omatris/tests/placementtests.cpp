#include "placementtests.h"

#include <QRandomGenerator>
#include <QtTest>

#include <set>

#include "enginefixture.h"
#include "handling.h"
#include "placements.h"

using namespace EngineFixture;

namespace {

// A game whose falling piece is `type`, played with an instant soft drop as
// the search requires.
Game gameWith(PieceType type) {
    Game game(Mode::Zen, kSeed);
    game.setSoftDropFactor(Handling::kInstantSoftDrop);
    game.placePiece({type, 0, {Piece::spawnColumn(type, Board::kWidth), Game::kSpawnRow}});
    return game;
}

bool sameGame(const Game &a, const Game &b) {
    for (int i = 0; i < Board::kCellCount; ++i) {
        if (a.board().at(i) != b.board().at(i))
            return false;
    }
    return a.score() == b.score() && a.phase() == b.phase() && a.hasPiece() == b.hasPiece()
        && a.piece().cells() == b.piece().cells() && a.nextQueue() == b.nextQueue()
        && a.heldPiece() == b.heldPiece() && a.ticks() == b.ticks();
}

std::vector<PieceType> deal(Bag bag, int count) {
    std::vector<PieceType> pieces;
    for (int i = 0; i < count; ++i)
        pieces.push_back(bag.take());
    return pieces;
}

}  // namespace

void PlacementTests::callsRoundTripTheirTokens() {
    for (int i = 0; i < int(Call::Tick); ++i) {
        const QString token = Calls::token(Call(i));
        QCOMPARE(Calls::fromToken(token), std::optional<Call>(Call(i)));
    }
    QCOMPARE(Calls::token(Call::RotateCCW), QStringLiteral("CCW"));
    QVERIFY(!Calls::fromToken(QStringLiteral("t1")));
    QVERIFY(!Calls::fromToken(QStringLiteral("cw")));
}

void PlacementTests::callsDoWhatTheKeysDo() {
    Game game = gameWith(PieceType::T);
    const int x = game.piece().origin.x();
    Calls::apply(game, Call::Left);
    QCOMPARE(game.piece().origin.x(), x - 1);
    Calls::apply(game, Call::RotateCW);
    QCOMPARE(game.piece().rotation, 1);
    QCOMPARE(count(Calls::apply(game, Call::Hold), Event::Held), 1);
    QCOMPARE(count(Calls::apply(game, Call::HardDrop), Event::Locked), 1);
    const int ticks = game.ticks();
    Calls::apply(game, Call::Tick);
    QCOMPARE(game.ticks(), ticks + 1);
}

void PlacementTests::aCopyPlaysOnIdentically() {
    Game game(Mode::Marathon, kSeed);
    QRandomGenerator rng(7);
    for (int i = 0; i < 200; ++i)
        Calls::apply(game, Call(rng.bounded(int(Call::Tick) + 1)));
    Game copy = game;
    QVERIFY(sameGame(copy, game));
    for (int i = 0; i < 3000 && game.phase() == Phase::Playing; ++i) {
        // Mostly ticks, so pieces fall and lock as well as being pushed about.
        const Call call = rng.bounded(3) ? Call::Tick : Call(rng.bounded(int(Call::Tick)));
        Calls::apply(game, call);
        Calls::apply(copy, call);
        QVERIFY2(sameGame(copy, game), qPrintable(QStringLiteral("diverged at call %1").arg(i)));
    }
}

void PlacementTests::reseedingRedrawsOnlyWhatIsHidden() {
    const Bag original(kSeed);
    const std::vector<PieceType> dealt = deal(original, 28);
    bool differs = false;
    for (quint32 seed = 1; seed <= 5; ++seed) {
        Bag reseeded = original;
        reseeded.reseedHidden(Rules::kNextQueue, seed);
        const std::vector<PieceType> redealt = deal(reseeded, 28);
        // What the player can see stays; every bag is still all seven pieces.
        QVERIFY(std::equal(dealt.begin(), dealt.begin() + Rules::kNextQueue, redealt.begin()));
        for (int bag = 0; bag < 4; ++bag) {
            const std::set<PieceType> pieces(redealt.begin() + bag * kPieceCount,
                                             redealt.begin() + (bag + 1) * kPieceCount);
            QCOMPARE(int(pieces.size()), kPieceCount);
        }
        differs = differs || redealt != dealt;
    }
    QVERIFY(differs);

    Game game(Mode::Marathon, kSeed);
    Game copy = game;
    copy.reseedHidden(99);
    QCOMPARE(copy.nextQueue(), game.nextQueue());
    QVERIFY(sameGame(copy, game));
}

void PlacementTests::anEmptyBoardHasEveryDropAndNothingElse() {
    // Four orientations of a T, J or L, but an S, Z or I fills the same cells
    // turned twice, and an O is the same every way.
    const std::pair<PieceType, int> expected[] = {
        {PieceType::T, 34}, {PieceType::J, 34}, {PieceType::L, 34}, {PieceType::S, 17},
        {PieceType::Z, 17}, {PieceType::I, 17}, {PieceType::O, 9},
    };
    for (const auto &[type, landings] : expected) {
        const std::vector<Landing> found = Placements::find(gameWith(type), false);
        QCOMPARE(int(found.size()), landings);
        for (const Landing &landing : found) {
            QCOMPARE(Placements::bottomRow(landing.placement), kBottom);
            QVERIFY(!landing.hold);
            QCOMPARE(landing.lines, 0);
        }
    }
}

void PlacementTests::everyLandingIsWhereItsCallsLeadIt() {
    // A ragged stack with an overhang, so tucks are possible.
    Game game = gameWith(PieceType::L);
    Board &board = game.mutableBoard();
    fillRow(board, kBottom, {2, 7});
    fillRow(board, kBottom - 1, {1, 2, 3, 6, 7, 8, 9});
    fillRow(board, kBottom - 2, {0, 1, 2, 3, 6, 7, 8, 9});
    board.set({6, kBottom - 3}, PieceType::Z);
    const std::vector<Landing> landings = Placements::find(game, true);
    QVERIFY(landings.size() > 34);
    for (const Landing &landing : landings) {
        Game played = game;
        for (Call call : landing.calls)
            Calls::apply(played, call);
        QCOMPARE(played.piece().cells(), landing.placement.cells());
        QCOMPARE(played.ghost().origin, played.piece().origin);
        QCOMPARE(played.spin(), landing.spin);
        const Event *locked = find(played.hardDrop(), Event::Locked);
        QVERIFY(locked);
        QCOMPARE(locked->clear.lines, landing.lines);
    }
    // A plain drop is always one of them.
    const auto dropped = Placements::drop(game, false, 0, 0);
    QVERIFY(dropped);
    QVERIFY(std::any_of(landings.begin(), landings.end(), [&dropped](const Landing &l) {
        return !l.hold && l.placement.cells() == dropped->placement.cells();
    }));
}

void PlacementTests::aTSlotIsFoundAsASpin() {
    // A T-spin double slot under an overhang at (3, 21): no straight drop
    // reaches it, only a turn at the bottom.
    Game game = gameWith(PieceType::T);
    Board &board = game.mutableBoard();
    fillRow(board, kBottom, {4});
    fillRow(board, kBottom - 1, {3, 4, 5});
    board.set({3, kBottom - 2}, PieceType::J);
    const std::vector<Landing> landings = Placements::find(game, false);
    const auto spin = std::find_if(landings.begin(), landings.end(), [](const Landing &l) {
        return l.placement.rotation == 2 && l.placement.origin == QPoint(3, kBottom - 2);
    });
    QVERIFY(spin != landings.end());
    QCOMPARE(spin->spin, Spin::Full);
    QCOMPARE(spin->lines, 2);
    // Dropped straight, the same T stops on the overhang.
    const auto straight = Placements::drop(game, false, 2, 3);
    QVERIFY(straight);
    QVERIFY(straight->placement.origin.y() < kBottom - 2);

    for (Call call : spin->calls)
        Calls::apply(game, call);
    const int before = game.score();
    game.hardDrop();
    QCOMPARE(game.score() - before, Rules::kTSpinDouble * game.level());
}

void PlacementTests::theHeldPieceLandsToo() {
    Game game = gameWith(PieceType::T);
    const PieceType next = game.nextQueue().front();
    const std::vector<Landing> landings = Placements::find(game, true);
    const auto held = std::count_if(landings.begin(), landings.end(), [](const Landing &l) { return l.hold; });
    QVERIFY(held > 0);
    for (const Landing &landing : landings) {
        QCOMPARE(landing.placement.type, landing.hold ? next : PieceType::T);
        QCOMPARE(landing.calls.front() == Call::Hold, landing.hold);
    }
    QCOMPARE(Placements::find(game, false).size(), landings.size() - size_t(held));

    game.hold();
    for (const Landing &landing : Placements::find(game, true))
        QVERIFY(!landing.hold);  // once per piece
}

void PlacementTests::dropReachesTheColumnItNames() {
    const Game game = gameWith(PieceType::I);
    const auto flat = Placements::drop(game, false, 0, 0);
    QVERIFY(flat);
    QCOMPARE(Placements::leftColumn(flat->placement), 0);
    QCOMPARE(Placements::bottomRow(flat->placement), kBottom);
    const auto upright = Placements::drop(game, false, 3, 9);
    QVERIFY(upright);
    QCOMPARE(Placements::leftColumn(upright->placement), 9);
    QCOMPARE(upright->placement.rotation, 3);
    QVERIFY(!Placements::drop(game, false, 0, 7));  // four wide does not start at 7
    const auto held = Placements::drop(game, true, 0, 0);
    QVERIFY(held);
    QCOMPARE(held->placement.type, game.nextQueue().front());
}

void PlacementTests::anAfterstateHasItsLinesCleared() {
    Game game = gameWith(PieceType::I);
    fillRow(game.mutableBoard(), kBottom, {0, 1, 2, 3});
    game.mutableBoard().set({9, kBottom - 1}, PieceType::Z);
    const auto flat = Placements::drop(game, false, 0, 0);
    QVERIFY(flat);
    QCOMPARE(flat->lines, 1);
    // The bottom row went, and the block above it fell into its place.
    QCOMPARE(flat->afterstate.at(QPoint(9, kBottom)), PieceType::Z);
    QCOMPARE(flat->afterstate.at(QPoint(0, kBottom)), PieceType::None);
    QCOMPARE(game.board().at(QPoint(9, kBottom)), PieceType::L);  // the game itself is untouched
}

void PlacementTests::aLandingThatLocksOutSaysSo() {
    // The visible well is full to the top but for one column an O cannot
    // fit, so wherever it rests is above the field.
    Game game = gameWith(PieceType::O);
    for (int y = Board::kHiddenRows; y < Board::kHeight; ++y)
        fillRow(game.mutableBoard(), y, {9});
    const std::vector<Landing> landings = Placements::find(game, false);
    QVERIFY(!landings.empty());
    for (const Landing &landing : landings)
        QVERIFY(landing.toppedOut);
}
