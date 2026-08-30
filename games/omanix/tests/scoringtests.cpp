#include "scoringtests.h"

#include <QtTest>

#include "scenario.h"

using namespace Scenario;

void ScoringTests::levelCompletesAtGoal() {
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

void ScoringTests::bigCutMultipliesTheScore() {
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

void ScoringTests::closeCallPaysABonus() {
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

void ScoringTests::extraLifeEveryTenThousand() {
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

void ScoringTests::gameOverAfterLastLife() {
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

void ScoringTests::restartLevelKeepsScoreAndLives() {
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

void ScoringTests::difficultyParametersRamp() {
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
void ScoringTests::theThreeDifficultiesFormALadder() {
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

void ScoringTests::sameSeedSameEvents() {
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

void ScoringTests::levelStartsWithAnIntroFreeze() {
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

void ScoringTests::respawnLandsWhereChasersMustCrawlFurthest() {
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

void ScoringTests::trailThreatenedWhileABallIsNear() {
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
