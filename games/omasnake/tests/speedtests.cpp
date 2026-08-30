#include "speedtests.h"

#include <QtTest>

#include "scenario.h"

using namespace Scenario;

void SpeedTests::speedRampsWithFoodEaten() {
    QCOMPARE(Rules::moveTicks(Difficulty::Normal, 0), 9);
    QCOMPARE(Rules::moveTicks(Difficulty::Normal, 4), 9);
    QCOMPARE(Rules::moveTicks(Difficulty::Normal, 5), 8);
    QCOMPARE(Rules::moveTicks(Difficulty::Normal, 19), 6);
    QCOMPARE(Rules::moveTicks(Difficulty::Normal, 20), 5);
    QCOMPARE(Rules::moveTicks(Difficulty::Normal, 500), 5);

    Game game(Mode::Wrap, Difficulty::Slow, kSeed);
    QCOMPARE(game.moveTicks(), Rules::params(Difficulty::Slow).startMoveTicks);
    QCOMPARE(game.cellsPerSecond(), double(Rules::kTicksPerSecond) / game.moveTicks());
    skipReady(game);
    game.placeSnake(row({3, 12}, Game::kStartLength), Direction::Right);
    const int perStep = Rules::params(Difficulty::Slow).foodsPerSpeedUp;
    std::vector<Event> last;
    for (int i = 0; i < perStep; ++i) {
        game.placeFood(game.snake().head() + QPoint(1, 0));
        last = stepMove(game);
    }
    QVERIFY(has(last, Event::SpeedUp));
    QCOMPARE(game.moveTicks(), Rules::params(Difficulty::Slow).startMoveTicks - 1);
}

void SpeedTests::theThreeDifficultiesFormALadder() {
    const SpeedParams slow = Rules::params(Difficulty::Slow);
    const SpeedParams normal = Rules::params(Difficulty::Normal);
    const SpeedParams fast = Rules::params(Difficulty::Fast);
    QVERIFY(slow.startMoveTicks > normal.startMoveTicks);
    QVERIFY(normal.startMoveTicks > fast.startMoveTicks);
    QVERIFY(slow.minMoveTicks > normal.minMoveTicks);
    QVERIFY(normal.minMoveTicks > fast.minMoveTicks);
    for (int i = 0; i < kDifficultyCount; ++i) {
        const SpeedParams params = Rules::params(Difficulty(i));
        QVERIFY(params.minMoveTicks > 0);
        QVERIFY(params.minMoveTicks < params.startMoveTicks);
        QVERIFY(params.foodsPerSpeedUp > 0);
    }
}
