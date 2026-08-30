#include "playtests.h"

#include <QtTest>

#include "scenario.h"

using namespace Scenario;

void PlayTests::tapMovesOneCellAndHoldKeepsMoving() {
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

void PlayTests::marchingIntoOpenSeaCutsATrailUntilGround() {
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

void PlayTests::cannotReverseOntoTheTrail() {
    Game game = quietGame();
    walk(game, Direction::Up, 4);
    QVERIFY(game.player().onTrail);
    game.setDirection(Direction::Down);
    QCOMPARE(game.player().dir, Direction::Up);
    game.setDirection(Direction::Left);
    QCOMPARE(game.player().dir, Direction::Left);
}

void PlayTests::ballOnTrailCostsALifeAndWipesIt() {
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

void PlayTests::crossingOwnTrailCostsALife() {
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

void PlayTests::chaserContactCostsALifeAnywhere() {
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
