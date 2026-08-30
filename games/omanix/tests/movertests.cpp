#include "movertests.h"

#include <QtTest>

#include "scenario.h"

using namespace Scenario;

void MoverTests::ballReflectsOffWalls() {
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

void MoverTests::ballReflectsOffCorner() {
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

void MoverTests::ballNeverSqueezesBetweenDiagonalBlockers() {
    Field field;
    field.set({11, 10}, Cell::Claimed);
    field.set({10, 11}, Cell::Claimed);
    Ball ball {{10, 10}, {1, 1}};
    ball.step(field);
    QCOMPARE(ball.dir, QPoint(-1, -1));
    QCOMPARE(ball.pos, QPoint(9, 9));
}

void MoverTests::chaserCrawlsTheFrameClockwise() {
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

void MoverTests::chaserFollowsNewlyClaimedGround() {
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

void MoverTests::chaserReversesAtADeadEnd() {
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
    // Pacing it soon counts as a circuit, but the bar is all the ground there
    // is, so the chaser has nowhere else to be.
    for (int i = 0; i < 40; ++i) {
        chaser.step(field, rng);
        QCOMPARE(chaser.pos.y(), 6);
        QVERIFY(chaser.pos.x() >= 5 && chaser.pos.x() <= 8);
    }
}

void MoverTests::chaserBuriedByAClaimWalksBackToTheEdge() {
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

void MoverTests::chaserPicksJunctionsFromTheSeed() {
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

void MoverTests::chaserLeavesAClosedPocketForAnotherRegion() {
    Field field(20, 12);
    // Rows 1..8 claimed but for a 3×3 pocket of sea, far enough inside the
    // ground that the ring around it touches nothing else: a chaser on that
    // ring used to orbit it for the rest of the level.
    for (int y = 1; y <= 8; ++y) {
        for (int x = 1; x <= 18; ++x)
            field.set({x, y}, Cell::Claimed);
    }
    for (int y = 3; y <= 5; ++y) {
        for (int x = 8; x <= 10; ++x)
            field.set({x, y}, Cell::Open);
    }
    const auto onTheRing = [](QPoint p) {
        return p.x() >= 7 && p.x() <= 11 && p.y() >= 2 && p.y() <= 6;
    };
    QVERIFY(field.isEdge({7, 2}));
    QVERIFY(!field.isEdge({7, 7}));

    QRandomGenerator rng(kSeed);
    Chaser chaser {{8, 2}, {1, 0}};
    const int ring = 5 * 5 - 3 * 3;
    // One lap of the ring is a circuit, and that is that: it gives the pocket
    // up rather than orbiting it for the rest of the level.
    int steps = 0;
    while (onTheRing(chaser.pos) && steps < 2 * ring) {
        chaser.step(field, rng);
        ++steps;
    }
    QVERIFY(!onTheRing(chaser.pos));
    // Then it walks over the ground to the boundary of the sea below, which
    // is a region of its own: the pocket's ring touches nothing but the
    // pocket, so no amount of crawling would ever have taken it there.
    while (!field.isEdge(chaser.pos) && steps < 4 * ring) {
        chaser.step(field, rng);
        QCOMPARE(field.at(chaser.pos), Cell::Claimed);
        ++steps;
    }
    QVERIFY(field.isEdge(chaser.pos));
    QVERIFY(chaser.pos.y() >= 8);
}

void MoverTests::chaserForgetsItsTrackWhenAClaimMovesTheBoundary() {
    Game game = quietGame();
    game.placeChasers({{{5, 0}, {1, 0}}});
    for (int i = 0; i < 4 * game.params().chaserPeriod; ++i)
        game.tick();
    QVERIFY(!game.chasers().front().track.empty());
    const std::vector<Event> events = cutColumn(game);
    QVERIFY(find(events, Event::Claimed));
    QVERIFY(game.chasers().front().track.empty());
}
