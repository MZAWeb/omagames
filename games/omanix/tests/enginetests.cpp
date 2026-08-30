#include "enginetests.h"

#include <QtTest>

#include "game.h"

namespace {

constexpr quint32 kSeed = 20260830u;
constexpr int kInteriorHeight = Field::kDefaultHeight - 2 * Field::kBorder;

// Runs ticks until the player has moved `cells` times (or gives up).
void walk(Game &game, Direction direction, int cells, std::vector<Event> *events = nullptr) {
    game.setDirection(direction);
    for (int i = 0; i < cells; ++i) {
        for (int t = 0; t < game.params().playerPeriod; ++t) {
            const std::vector<Event> tickEvents = game.tick();
            if (events)
                events->insert(events->end(), tickEvents.begin(), tickEvents.end());
        }
    }
    game.releaseDirection(direction);
}

int count(const std::vector<Event> &events, Event::Type type) {
    return int(std::count_if(events.begin(), events.end(), [type](const Event &e) { return e.type == type; }));
}

const Event *find(const std::vector<Event> &events, Event::Type type) {
    for (const Event &e : events) {
        if (e.type == type)
            return &e;
    }
    return nullptr;
}

void skipIntro(Game &game) {
    for (int i = 0; i < Game::kLevelIntroTicks; ++i)
        game.tick();
}

// Walls a ball into a one-cell pocket so it stays exactly where a scenario
// put it. Its region is then that single cell, so any claim afterwards takes
// the whole rest of the field.
void fence(Game &game, QPoint ball) {
    Field &field = game.mutableField();
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx != 0 || dy != 0)
                field.set({ball.x() + dx, ball.y() + dy}, Cell::Claimed);
        }
    }
}

// A game with nothing that could interfere: no chasers, and one ball kept on
// the far side of a claimed wall down column `wallX`. Any cut left of the
// wall closes with no ball on that side, so it claims every interior column
// up to the wall: `(wallX - kBorder) * kInteriorHeight` cells. The marker starts
// bottom-centre.
Game quietGame(int wallX = 40) {
    Game game(Difficulty::Normal, kSeed);
    game.placeChasers({});
    Field &field = game.mutableField();
    for (int y = Field::kBorder; y < field.height() - Field::kBorder; ++y)
        field.set({wallX, y}, Cell::Claimed);
    game.placeBalls({{{wallX + 2, Field::kBorder}, {1, 1}}});
    game.placePlayer({field.width() / 2, field.height() - 1});
    skipIntro(game);
    return game;
}

// Cuts a full-height column at the marker's x, from the bottom frame up to the top.
std::vector<Event> cutColumn(Game &game) {
    std::vector<Event> events;
    walk(game, Direction::Up, game.field().height() - 1, &events);
    return events;
}

}  // namespace

void EngineTests::fieldStartsAsFrameAroundOpenSea() {
    Field field;
    QCOMPARE(field.width(), 64);
    QCOMPARE(field.height(), 40);
    QCOMPARE(field.interiorCells(), 62 * 38);
    QCOMPARE(field.claimedInterior(), 0);
    QCOMPARE(field.at({0, 0}), Cell::Claimed);
    QCOMPARE(field.at({1, 39}), Cell::Claimed);
    // One cell deep: the second ring in is already open sea.
    QCOMPARE(field.at({1, 1}), Cell::Open);
    QCOMPARE(field.at({62, 38}), Cell::Open);
    QVERIFY(field.isBorder({63, 20}));
    QVERIFY(!field.isBorder({1, 20}));
    QVERIFY(!field.contains({64, 0}));
    // The whole frame is edge, corners included, and nothing inside it is.
    QVERIFY(field.isEdge({0, 0}));
    QVERIFY(field.isEdge({30, 39}));
    QVERIFY(!field.isEdge({1, 1}));
}

void EngineTests::claimTakesTrailAndBallFreeRegions() {
    Field field(12, 10);
    // Vertical trail down column 5 splits the interior in two.
    for (int y = 1; y < 9; ++y)
        field.set({5, y}, Cell::Trail);
    const std::vector<int> claimed = field.claim({QPoint(8, 5)});
    // Trail (8) plus the left region: columns 1..4 × rows 1..8 = 32.
    QCOMPARE(int(claimed.size()), 8 + 32);
    QCOMPARE(field.at({3, 4}), Cell::Claimed);
    QCOMPARE(field.at({5, 4}), Cell::Claimed);
    QCOMPARE(field.at({8, 4}), Cell::Open);
    QCOMPARE(field.trailCells().size(), size_t(0));
}

void EngineTests::claimKeepsEveryRegionWithABall() {
    Field field(12, 10);
    for (int y = 1; y < 9; ++y)
        field.set({5, y}, Cell::Trail);
    const std::vector<int> claimed = field.claim({QPoint(3, 3), QPoint(8, 5)});
    QCOMPARE(int(claimed.size()), 8);
    QCOMPARE(field.at({3, 4}), Cell::Open);
    QCOMPARE(field.at({8, 4}), Cell::Open);

    // Three regions, one ball: the two empty ones go.
    Field three(12, 10);
    for (int y = 1; y < 9; ++y) {
        three.set({4, y}, Cell::Trail);
        three.set({7, y}, Cell::Trail);
    }
    three.claim({QPoint(5, 5)});
    QCOMPARE(three.at({2, 5}), Cell::Claimed);
    QCOMPARE(three.at({5, 5}), Cell::Open);
    QCOMPARE(three.at({9, 5}), Cell::Claimed);
    QCOMPARE(three.claimedInterior(), 8 + 8 + 3 * 8 + 3 * 8);
}

void EngineTests::claimMergesWithExistingGround() {
    Field field(12, 10);
    // Ground already claimed on the left; a trail hugging it closes a pocket.
    for (int y = 2; y < 8; ++y)
        field.set({2, y}, Cell::Claimed);
    field.set({3, 2}, Cell::Trail);
    field.set({3, 3}, Cell::Trail);
    field.set({4, 3}, Cell::Trail);
    field.set({4, 2}, Cell::Trail);
    field.claim({QPoint(8, 6)});
    // The trail cells become ground; no pocket was enclosed (the trail touches
    // the top frame on both ends with nothing inside), so 6 + 4 claimed.
    QCOMPARE(field.claimedInterior(), 10);
    // Now a loop around (6,5): trail cells all around it.
    Field pocket(12, 10);
    const QPoint ring[] = {{5, 4}, {6, 4}, {7, 4}, {7, 5}, {7, 6}, {6, 6}, {5, 6}, {5, 5}};
    for (QPoint p : ring)
        pocket.set(p, Cell::Trail);
    pocket.claim({QPoint(2, 2)});
    QCOMPARE(pocket.at({6, 5}), Cell::Claimed);
    QCOMPARE(pocket.claimedInterior(), 9);
}

void EngineTests::ballReflectsOffWalls() {
    Field field;
    Ball ball {{3, 10}, {-1, 1}};
    ball.step(field);
    QCOMPARE(ball.pos, QPoint(2, 11));
    ball.step(field);
    QCOMPARE(ball.pos, QPoint(1, 12));
    ball.step(field);  // x would hit the frame at 0: flip dx, keep dy
    QCOMPARE(ball.dir, QPoint(1, 1));
    QCOMPARE(ball.pos, QPoint(2, 13));

    Ball floor {{20, 36}, {1, 1}};
    floor.step(field);
    QCOMPARE(floor.pos, QPoint(21, 37));
    floor.step(field);
    QCOMPARE(floor.pos, QPoint(22, 38));
    floor.step(field);
    QCOMPARE(floor.dir, QPoint(1, -1));
    QCOMPARE(floor.pos, QPoint(23, 37));

    // The trail is not a wall.
    field.set({24, 36}, Cell::Trail);
    floor.step(field);
    QCOMPARE(floor.pos, QPoint(24, 36));
}

void EngineTests::ballReflectsOffCorner() {
    Field field;
    // Exact corner of the interior: both axes flip.
    Ball ball {{1, 1}, {-1, -1}};
    ball.step(field);
    QCOMPARE(ball.dir, QPoint(1, 1));
    QCOMPARE(ball.pos, QPoint(2, 2));

    // A lone claimed cell on the diagonal, sides free: bounce straight back.
    Field lone;
    lone.set({11, 11}, Cell::Claimed);
    Ball diagonal {{10, 10}, {1, 1}};
    diagonal.step(lone);
    QCOMPARE(diagonal.dir, QPoint(-1, -1));
    QCOMPARE(diagonal.pos, QPoint(9, 9));
}

void EngineTests::ballNeverSqueezesBetweenDiagonalBlockers() {
    Field field;
    field.set({11, 10}, Cell::Claimed);
    field.set({10, 11}, Cell::Claimed);
    Ball ball {{10, 10}, {1, 1}};
    ball.step(field);
    QCOMPARE(ball.dir, QPoint(-1, -1));
    QCOMPARE(ball.pos, QPoint(9, 9));
}

void EngineTests::chaserCrawlsTheFrameClockwise() {
    Field field;
    QRandomGenerator rng(kSeed);
    Chaser chaser {{10, 0}, {1, 0}};
    // Along the top edge to the corner, then round it and down the right one.
    for (int i = 0; i < 53; ++i)
        chaser.step(field, rng);
    QCOMPARE(chaser.pos, QPoint(63, 0));
    chaser.step(field, rng);
    QCOMPARE(chaser.pos, QPoint(63, 1));
    QCOMPARE(chaser.dir, QPoint(0, 1));
    // It never leaves the ground.
    const int ring = 2 * (field.width() + field.height()) - 4;
    for (int i = 0; i < 3 * ring; ++i) {
        chaser.step(field, rng);
        QCOMPARE(field.at(chaser.pos), Cell::Claimed);
    }

    // The frame is one unbroken ring with nowhere to branch off to, so a
    // chaser never has a choice to make: it crawls every cell of the ring and
    // a full lap brings it home facing the way it set out.
    Chaser lap {{10, 0}, {1, 0}};
    std::vector<bool> seen(size_t(field.cellCount()), false);
    int distinct = 0;
    for (int i = 0; i < ring; ++i) {
        lap.step(field, rng);
        const int index = field.index(lap.pos);
        if (!seen[size_t(index)]) {
            seen[size_t(index)] = true;
            ++distinct;
        }
    }
    QCOMPARE(distinct, ring);
    QCOMPARE(lap.pos, QPoint(10, 0));
    QCOMPARE(lap.dir, QPoint(1, 0));
}

void EngineTests::chaserFollowsNewlyClaimedGround() {
    Field field;
    // A one-cell spur claimed downward from the top frame at column 30.
    for (int y = 1; y <= 5; ++y)
        field.set({30, y}, Cell::Claimed);
    // The foot of the spur is a junction, and this seed turns down it. (The
    // chaser used to take the spur every time, keeping the sea on its right;
    // now which way it goes is the seed's business.)
    QRandomGenerator rng(kSeed + 1);
    Chaser chaser {{28, 0}, {1, 0}};
    chaser.step(field, rng);
    QCOMPARE(chaser.pos, QPoint(29, 0));
    chaser.step(field, rng);
    QCOMPARE(chaser.pos, QPoint(30, 0));
    // Down the edge the claim added...
    for (int y = 1; y <= 5; ++y) {
        chaser.step(field, rng);
        QCOMPARE(chaser.pos, QPoint(30, y));
    }
    // ...turns around at the tip, with only one way back up...
    for (int y = 4; y >= 0; --y) {
        chaser.step(field, rng);
        QCOMPARE(chaser.pos, QPoint(30, y));
    }
    // ...and the frame carries it on, whichever way the junction sends it.
    chaser.step(field, rng);
    QCOMPARE(chaser.pos.y(), 0);
    QVERIFY(chaser.pos == QPoint(29, 0) || chaser.pos == QPoint(31, 0));
}

void EngineTests::chaserReversesAtADeadEnd() {
    Field field(20, 12);
    // An island bar out in the sea: every cell of it is edge and both ends
    // are dead ends, so a chaser can only pace it.
    for (int x = 5; x <= 8; ++x)
        field.set({x, 6}, Cell::Claimed);
    QRandomGenerator rng(kSeed);
    Chaser chaser {{6, 6}, {1, 0}};
    chaser.step(field, rng);
    QCOMPARE(chaser.pos, QPoint(7, 6));
    chaser.step(field, rng);
    QCOMPARE(chaser.pos, QPoint(8, 6));
    chaser.step(field, rng);
    QCOMPARE(chaser.pos, QPoint(7, 6));
    QCOMPARE(chaser.dir, QPoint(-1, 0));
    for (int i = 0; i < 40; ++i) {
        chaser.step(field, rng);
        QCOMPARE(chaser.pos.y(), 6);
        QVERIFY(chaser.pos.x() >= 5 && chaser.pos.x() <= 8);
    }
}

void EngineTests::chaserBuriedByAClaimWalksBackToTheEdge() {
    Field field(20, 12);
    // A claim swallowed the sea around the chaser's stretch of frame.
    for (int y = 1; y <= 4; ++y) {
        for (int x = 1; x <= 18; ++x)
            field.set({x, y}, Cell::Claimed);
    }
    QRandomGenerator rng(kSeed);
    Chaser chaser {{9, 2}, {1, 0}};
    QVERIFY(!field.isEdge(chaser.pos));
    // Straight down to the nearest edge, one cell a step, then it hugs it.
    chaser.step(field, rng);
    QCOMPARE(chaser.pos, QPoint(9, 3));
    chaser.step(field, rng);
    QCOMPARE(chaser.pos, QPoint(9, 4));
    QVERIFY(field.isEdge(chaser.pos));
    chaser.step(field, rng);
    QCOMPARE(chaser.pos.y(), 4);
    QVERIFY(field.isEdge(chaser.pos));
}

void EngineTests::chaserPicksJunctionsFromTheSeed() {
    // A spur off the top frame gives the chaser a junction to decide at every
    // time it passes the spur's foot or climbs back to it.
    const auto crawl = [](quint32 seed) {
        Field field;
        for (int y = 1; y <= 8; ++y)
            field.set({30, y}, Cell::Claimed);
        QRandomGenerator rng(seed);
        Chaser chaser {{28, 0}, {1, 0}};
        QString path;
        for (int i = 0; i < 200; ++i) {
            chaser.step(field, rng);
            path += QStringLiteral("%1,%2;").arg(chaser.pos.x()).arg(chaser.pos.y());
        }
        return path;
    };
    QCOMPARE(crawl(kSeed), crawl(kSeed));
    QVERIFY(crawl(kSeed) != crawl(kSeed + 1));
}

void EngineTests::tapMovesOneCellAndHoldKeepsMoving() {
    Game game = quietGame();
    const QPoint start = game.player().pos;
    game.setDirection(Direction::Left);
    game.releaseDirection(Direction::Left);
    for (int i = 0; i < 3 * game.params().playerPeriod; ++i)
        game.tick();
    QCOMPARE(game.player().pos, start + QPoint(-1, 0));
    QCOMPARE(game.player().dir, Direction::None);

    game.setDirection(Direction::Left);
    for (int i = 0; i < 3 * game.params().playerPeriod; ++i)
        game.tick();
    QCOMPARE(game.player().pos, start + QPoint(-4, 0));
    game.releaseDirection(Direction::Left);
    for (int i = 0; i < 3 * game.params().playerPeriod; ++i)
        game.tick();
    QCOMPARE(game.player().pos, start + QPoint(-4, 0));

    // The frame edge stops it.
    walk(game, Direction::Down, 5);
    QCOMPARE(game.player().pos.y(), game.field().height() - 1);
}

void EngineTests::marchingIntoOpenSeaCutsATrailUntilGround() {
    Game game = quietGame();
    const int x = game.player().pos.x();
    std::vector<Event> events;
    game.setDirection(Direction::Up);
    for (int i = 0; i < 3 * game.params().playerPeriod; ++i) {
        const std::vector<Event> e = game.tick();
        events.insert(events.end(), e.begin(), e.end());
    }
    game.releaseDirection(Direction::Up);
    QCOMPARE(count(events, Event::TrailStarted), 1);
    QVERIFY(game.player().onTrail);
    QCOMPARE(game.field().at({x, game.field().height() - Field::kBorder - 1}), Cell::Trail);

    // Released, it keeps going through the sea, closes on the first cell of
    // the top frame and stops there.
    for (int i = 0; i < 40 * game.params().playerPeriod; ++i) {
        const std::vector<Event> e = game.tick();
        events.insert(events.end(), e.begin(), e.end());
    }
    QVERIFY(!game.player().onTrail);
    QCOMPARE(game.player().pos.y(), Field::kBorder - 1);
    QCOMPARE(game.player().dir, Direction::None);
    const Event *claimed = find(events, Event::Claimed);
    QVERIFY(claimed);
    // The column itself plus the whole ball-free region around it.
    const int interiorHeight = game.field().height() - 2 * Field::kBorder;
    QCOMPARE(interiorHeight, kInteriorHeight);
    QCOMPARE(int(claimed->cells.size()), interiorHeight * (40 - Field::kBorder));
    QCOMPARE(game.field().at({x, 10}), Cell::Claimed);
    QCOMPARE(claimed->at, game.player().pos);
    QCOMPARE(game.score(), claimed->points);
}

void EngineTests::cannotReverseOntoTheTrail() {
    Game game = quietGame();
    walk(game, Direction::Up, 4);
    QVERIFY(game.player().onTrail);
    game.setDirection(Direction::Down);
    QCOMPARE(game.player().dir, Direction::Up);
    game.setDirection(Direction::Left);
    QCOMPARE(game.player().dir, Direction::Left);
}

void EngineTests::ballOnTrailCostsALifeAndWipesIt() {
    Game game = quietGame();
    const QPoint start = game.player().pos;
    std::vector<Event> events;
    walk(game, Direction::Up, 5, &events);
    QCOMPARE(count(events, Event::LifeLost), 0);
    // Five steps up from the frame leave five trail cells (rows 34..38).
    // Park a ball diagonally above the trail's top, heading into it.
    game.placeBalls({{{start.x() - 1, start.y() - 6}, {1, 1}}});
    for (int i = 0; i < 2 * game.params().ballPeriod && !find(events, Event::LifeLost); ++i) {
        const std::vector<Event> e = game.tick();
        events.insert(events.end(), e.begin(), e.end());
    }
    const Event *lost = find(events, Event::LifeLost);
    QVERIFY(lost);
    QCOMPARE(lost->reason, LifeLostReason::BallHitTrail);
    // Five trail cells from the walk, plus the one the marker kept cutting
    // before the ball arrived.
    QCOMPARE(int(lost->cells.size()), 6);
    QCOMPARE(game.lives(), Game::kStartLives - 1);
    QCOMPARE(game.field().trailCells().size(), size_t(0));
    QVERIFY(!game.player().onTrail);
    QCOMPARE(game.player().pos.y(), game.field().height() - 1);
    // Nothing moves during the respawn freeze, then play resumes.
    const QPoint ball = game.balls().front().pos;
    for (int i = 0; i < Game::kRespawnTicks; ++i)
        game.tick();
    QCOMPARE(game.balls().front().pos, ball);
    for (int i = 0; i < game.params().ballPeriod; ++i)
        game.tick();
    QVERIFY(game.balls().front().pos != ball);
}

void EngineTests::crossingOwnTrailCostsALife() {
    Game game = quietGame();
    std::vector<Event> events;
    walk(game, Direction::Up, 4, &events);
    walk(game, Direction::Left, 2, &events);
    walk(game, Direction::Down, 2, &events);
    walk(game, Direction::Right, 2, &events);
    const Event *lost = find(events, Event::LifeLost);
    QVERIFY(lost);
    QCOMPARE(lost->reason, LifeLostReason::SelfCross);
    QCOMPARE(game.lives(), Game::kStartLives - 1);
}

void EngineTests::chaserContactCostsALifeAnywhere() {
    Game game = quietGame();
    const QPoint start = game.player().pos;
    // Chaser one cell along the frame from the marker, crawling into it.
    game.placeChasers({{{start.x() - 1, start.y()}, {1, 0}}});
    std::vector<Event> events;
    for (int i = 0; i < game.params().chaserPeriod; ++i) {
        const std::vector<Event> e = game.tick();
        events.insert(events.end(), e.begin(), e.end());
    }
    const Event *lost = find(events, Event::LifeLost);
    QVERIFY(lost);
    QCOMPARE(lost->reason, LifeLostReason::ChaserHit);
    QCOMPARE(game.lives(), Game::kStartLives - 1);

    // And closing a cut on top of one is just as fatal. A lone claimed cell
    // out in the sea has no edge leading anywhere, so a chaser on it waits.
    Game island = quietGame();
    const QPoint from = island.player().pos;
    const QPoint perch {from.x(), from.y() - 4};
    island.mutableField().set(perch, Cell::Claimed);
    island.placeChasers({{perch, {1, 0}}});
    events.clear();
    walk(island, Direction::Up, 4, &events);
    lost = find(events, Event::LifeLost);
    QVERIFY(lost);
    QCOMPARE(lost->reason, LifeLostReason::ChaserHit);
}

void EngineTests::levelCompletesAtGoal() {
    // Any cut left of a wall six columns short of the right frame leaves
    // only the ball's sliver beyond it, far past the goal.
    Game game = quietGame(Field::kDefaultWidth - Field::kBorder - 6);
    walk(game, Direction::Right, 10);
    const std::vector<Event> events = cutColumn(game);
    QVERIFY(game.claimedPercent() >= Game::kGoalPercent);
    QCOMPARE(game.phase(), Phase::LevelComplete);
    const Event *done = find(events, Event::LevelComplete);
    QVERIFY(done);
    QCOMPARE(done->points, Game::kStartLives * Game::kLifeBonus * 1);
    QCOMPARE(game.lastLevel().livesBonus, done->points);
    QVERIFY(game.lastLevel().ticks > 0);
    // Frozen until the next level is requested.
    QVERIFY(game.tick().empty());
    game.nextLevel();
    QCOMPARE(game.level(), 2);
    QCOMPARE(game.phase(), Phase::Playing);
    QVERIFY(game.inLevelIntro());
    QCOMPARE(game.claimedPercent(), 0.0);
    QCOMPARE(int(game.balls().size()), Level::params(Difficulty::Normal, 2).balls);
}

void EngineTests::bigCutMultipliesTheScore() {
    // A wall at column 32 leaves 31 of 62 columns to claim: 50% → ×4.
    Game huge = quietGame(32);
    walk(huge, Direction::Left, 1);
    std::vector<Event> events = cutColumn(huge);
    const Event *claim = find(events, Event::Claimed);
    QVERIFY(claim);
    QCOMPARE(int(claim->cells.size()), 31 * kInteriorHeight);
    QCOMPARE(claim->multiplier, Game::kHugeCutMultiplier);
    QCOMPARE(claim->points, int(claim->cells.size()) * Game::kHugeCutMultiplier);

    // A wall at column 12 leaves 11 of 62 columns: 17.7% → ×2.
    Game big = quietGame(12);
    walk(big, Direction::Left, big.player().pos.x() - 11);
    events = cutColumn(big);
    claim = find(events, Event::Claimed);
    QVERIFY(claim);
    QCOMPARE(int(claim->cells.size()), 11 * kInteriorHeight);
    QCOMPARE(claim->multiplier, Game::kBigCutMultiplier);
    QCOMPARE(big.score(), 11 * kInteriorHeight * Game::kBigCutMultiplier);

    // A wall at column 6 leaves 5 columns: 8.1%, plain.
    Game plain = quietGame(6);
    walk(plain, Direction::Left, plain.player().pos.x() - 5);
    events = cutColumn(plain);
    claim = find(events, Event::Claimed);
    QVERIFY(claim);
    QCOMPARE(claim->multiplier, 1);
    QCOMPARE(plain.score(), 5 * kInteriorHeight);
}

void EngineTests::closeCallPaysABonus() {
    Game game = quietGame();
    const QPoint start = game.player().pos;
    // Ball fenced exactly three cells from the column the marker will cut.
    const QPoint near {start.x() + Game::kCloseCallDistance, 5};
    game.placeBalls({{near, {1, -1}}});
    fence(game, near);
    std::vector<Event> events;
    walk(game, Direction::Up, game.field().height() - 1, &events);
    const Event *claim = find(events, Event::Claimed);
    QVERIFY(claim);
    QVERIFY(claim->closeCall);
    QCOMPARE(claim->points, int(claim->cells.size()) * claim->multiplier + Game::kCloseCallBonus);

    Game far = quietGame();
    events.clear();
    walk(far, Direction::Up, far.field().height() - 1, &events);
    claim = find(events, Event::Claimed);
    QVERIFY(claim);
    QVERIFY(!claim->closeCall);
}

void EngineTests::extraLifeEveryTenThousand() {
    Game game = quietGame();
    std::vector<Event> events;
    game.addScore(Game::kExtraLifeScore - 1, events);
    QCOMPARE(count(events, Event::ExtraLife), 0);
    game.addScore(1, events);
    QCOMPARE(count(events, Event::ExtraLife), 1);
    QCOMPARE(game.lives(), Game::kStartLives + 1);
    game.addScore(2 * Game::kExtraLifeScore, events);
    QCOMPARE(count(events, Event::ExtraLife), 3);
    QCOMPARE(game.lives(), Game::kStartLives + 3);
}

void EngineTests::gameOverAfterLastLife() {
    Game game = quietGame();
    std::vector<Event> events;
    for (int life = 0; life < Game::kStartLives; ++life) {
        walk(game, Direction::Up, 4, &events);
        walk(game, Direction::Left, 2, &events);
        walk(game, Direction::Down, 2, &events);
        walk(game, Direction::Right, 2, &events);
        for (int i = 0; i < Game::kRespawnTicks; ++i)
            game.tick();
    }
    QCOMPARE(count(events, Event::LifeLost), Game::kStartLives);
    QCOMPARE(count(events, Event::GameOver), 1);
    QCOMPARE(game.lives(), 0);
    QCOMPARE(game.phase(), Phase::GameOver);
    QVERIFY(game.tick().empty());
}

void EngineTests::restartLevelKeepsScoreAndLives() {
    Game game = quietGame();
    walk(game, Direction::Left, 5);
    cutColumn(game);
    const int score = game.score();
    QVERIFY(score > 0);
    // Everything left of the wall is ground now; cut into the open side.
    walk(game, Direction::Right, 45 - game.player().pos.x());
    walk(game, Direction::Down, 3);
    QVERIFY(game.player().onTrail);
    game.restartLevel();
    QCOMPARE(game.score(), score);
    QCOMPARE(game.lives(), Game::kStartLives);
    QCOMPARE(game.level(), 1);
    QCOMPARE(game.claimedPercent(), 0.0);
    QVERIFY(!game.player().onTrail);
    QCOMPARE(game.levelTicks(), 0);
}

void EngineTests::difficultyParametersRamp() {
    QCOMPARE(Level::params(Difficulty::Easy, 1).balls, 2);
    QCOMPARE(Level::params(Difficulty::Normal, 1).balls, 3);
    QCOMPARE(Level::params(Difficulty::Hard, 1).balls, 4);
    QCOMPARE(Level::params(Difficulty::Easy, 1).chasers, 1);
    QCOMPARE(Level::params(Difficulty::Normal, 1).chasers, 2);
    QCOMPARE(Level::params(Difficulty::Hard, 1).chasers, 3);
    // Each level adds a ball; speed and chasers ramp per difficulty.
    QCOMPARE(Level::params(Difficulty::Normal, 4).balls, 6);
    QVERIFY(Level::params(Difficulty::Normal, 5).ballPeriod < Level::params(Difficulty::Normal, 1).ballPeriod);
    QCOMPARE(Level::params(Difficulty::Normal, 4).chasers, 2);
    QCOMPARE(Level::params(Difficulty::Normal, 5).chasers, 3);
    QCOMPARE(Level::params(Difficulty::Hard, 3).chasers, 4);
    QCOMPARE(Level::params(Difficulty::Easy, 5).chasers, 1);
    QCOMPARE(Level::params(Difficulty::Easy, 6).chasers, 2);
    // And everything is capped.
    QCOMPARE(Level::params(Difficulty::Hard, 40).balls, Level::kMaxBalls);
    QCOMPARE(Level::params(Difficulty::Hard, 40).chasers, Level::kMaxChasers);
    QCOMPARE(Level::params(Difficulty::Hard, 40).ballPeriod, 3);
    // The starting positions follow the parameters.
    Game game(Difficulty::Hard, kSeed);
    skipIntro(game);
    QCOMPARE(int(game.balls().size()), 4);
    QCOMPARE(int(game.chasers().size()), 3);
    for (const Ball &b : game.balls())
        QCOMPARE(game.field().at(b.pos), Cell::Open);
    for (const Chaser &c : game.chasers())
        QVERIFY(game.field().isEdge(c.pos));
}

// Ball speed, ball count and chaser count all have to climb from Easy to
// Hard, and Normal has to be the small step off Easy rather than Hard with
// the edges filed off.
void EngineTests::theThreeDifficultiesFormALadder() {
    for (int level : {1, 5}) {
        const LevelParams easy = Level::params(Difficulty::Easy, level);
        const LevelParams normal = Level::params(Difficulty::Normal, level);
        const LevelParams hard = Level::params(Difficulty::Hard, level);
        // Speed is ticks per move, so faster is a shorter period.
        QVERIFY(easy.ballPeriod > normal.ballPeriod);
        QVERIFY(normal.ballPeriod > hard.ballPeriod);
        QVERIFY(easy.balls < normal.balls);
        QVERIFY(normal.balls < hard.balls);
        QVERIFY(easy.chasers < normal.chasers);
        QVERIFY(normal.chasers < hard.chasers);
    }
    const LevelParams easy = Level::params(Difficulty::Easy, Level::kFirstLevel);
    const LevelParams normal = Level::params(Difficulty::Normal, Level::kFirstLevel);
    const LevelParams hard = Level::params(Difficulty::Hard, Level::kFirstLevel);
    QVERIFY(easy.ballPeriod - normal.ballPeriod < normal.ballPeriod - hard.ballPeriod);
    QVERIFY(normal.chasers - easy.chasers <= hard.chasers - normal.chasers);
    QVERIFY(easy.chaserPeriod > normal.chaserPeriod);
    QVERIFY(normal.chaserPeriod > hard.chaserPeriod);
}

void EngineTests::sameSeedSameEvents() {
    auto play = [](quint32 seed) {
        Game game(Difficulty::Normal, seed);
        skipIntro(game);
        std::vector<Event> events;
        const Direction plan[] = {Direction::Up, Direction::Up, Direction::Left, Direction::Up, Direction::Right};
        for (Direction d : plan)
            walk(game, d, 7, &events);
        for (int i = 0; i < 600; ++i) {
            const std::vector<Event> e = game.tick();
            events.insert(events.end(), e.begin(), e.end());
        }
        QString log;
        for (const Event &e : events)
            log += QStringLiteral("%1:%2:%3;").arg(int(e.type)).arg(e.points).arg(int(e.cells.size()));
        log += QStringLiteral("balls=");
        for (const Ball &b : game.balls())
            log += QStringLiteral("%1,%2;").arg(b.pos.x()).arg(b.pos.y());
        return log;
    };
    QCOMPARE(play(kSeed), play(kSeed));
    QVERIFY(play(kSeed) != play(kSeed + 1));
}

void EngineTests::levelStartsWithAnIntroFreeze() {
    Game game(Difficulty::Normal, kSeed);
    QVERIFY(game.inLevelIntro());
    const QPoint ball = game.balls().front().pos;
    game.setDirection(Direction::Up);
    for (int i = 0; i < Game::kLevelIntroTicks; ++i) {
        QVERIFY(game.inLevelIntro());
        QVERIFY(game.tick().empty());
    }
    QVERIFY(!game.inLevelIntro());
    QCOMPARE(game.balls().front().pos, ball);
    QCOMPARE(game.levelTicks(), 0);
    // The key pressed during the intro takes effect right after it.
    for (int i = 0; i < game.params().playerPeriod; ++i)
        game.tick();
    QCOMPARE(game.player().pos.y(), game.field().height() - 2);
    // A restart announces the level again.
    game.restartLevel();
    QVERIFY(game.inLevelIntro());
}

void EngineTests::respawnLandsWhereChasersMustCrawlFurthest() {
    Game game = quietGame();
    // One chaser in the bottom-left corner. Chasers walk the ground, so what
    // counts is the crawl along it: the far end of the bottom edge.
    game.placeChasers({{{0, game.field().height() - 1}, {1, 0}}});
    std::vector<Event> events;
    walk(game, Direction::Up, 4, &events);
    walk(game, Direction::Left, 2, &events);
    walk(game, Direction::Down, 2, &events);
    walk(game, Direction::Right, 2, &events);
    QVERIFY(find(events, Event::LifeLost));
    QCOMPARE(game.player().pos, QPoint(game.field().width() - 1, game.field().height() - 1));

    // With no chaser at all every cell is equally safe, so the marker comes
    // back to the middle of the bottom edge.
    Game empty = quietGame();
    events.clear();
    walk(empty, Direction::Up, 4, &events);
    walk(empty, Direction::Left, 2, &events);
    walk(empty, Direction::Down, 2, &events);
    walk(empty, Direction::Right, 2, &events);
    QVERIFY(find(events, Event::LifeLost));
    QCOMPARE(empty.player().pos, QPoint(empty.field().width() / 2, empty.field().height() - 1));
}

void EngineTests::trailThreatenedWhileABallIsNear() {
    Game game = quietGame();
    QVERIFY(!game.trailThreatened());
    const QPoint start = game.player().pos;
    const QPoint near {start.x() + Game::kCloseCallDistance, start.y() - 6};
    game.placeBalls({{near, {1, -1}}});
    fence(game, near);
    walk(game, Direction::Up, 2);
    QVERIFY(game.player().onTrail);
    // Trail top at row 37 is 4 rows from the ball: not yet.
    QVERIFY(!game.trailThreatened());
    walk(game, Direction::Up, 1);
    QVERIFY(game.trailThreatened());
    // Closing ends the threat with the trail.
    walk(game, Direction::Left, 1);
    QVERIFY(game.player().onTrail);
    walk(game, Direction::Down, 3);
    QVERIFY(!game.player().onTrail);
    QVERIFY(!game.trailThreatened());
}
