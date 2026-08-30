#include "snaketests.h"

#include <QtTest>

#include "scenario.h"

using namespace Scenario;

void SnakeTests::snakeStartsCentredAndKeepsItsHeading() {
    Game game(Mode::Classic, Difficulty::Normal, kSeed);
    QCOMPARE(game.phase(), Phase::Playing);
    QCOMPARE(game.length(), Game::kStartLength);
    QCOMPARE(game.snake().head(), QPoint(Game::kWidth / 2, Game::kHeight / 2));
    QCOMPARE(game.snake().heading(), Direction::Right);
    QCOMPARE(game.score(), 0);
    QCOMPARE(game.multiplier(), 1);
    QVERIFY(!game.hasBonus());
    QVERIFY(Game::contains(game.food()));
    QVERIFY(!game.snake().occupies(game.food()));

    skipReady(game);
    game.placeFood(kParked);
    const QPoint head = game.snake().head();
    for (int i = 0; i < 3; ++i)
        stepMove(game);
    QCOMPARE(game.snake().head(), head + QPoint(3, 0));
    QCOMPARE(game.length(), Game::kStartLength);
}

void SnakeTests::readyBeatHoldsTheSnakeStill() {
    Game game(Mode::Classic, Difficulty::Fast, kSeed);
    const QPoint head = game.snake().head();
    for (int i = 0; i < Game::kReadyTicks; ++i) {
        QVERIFY(game.ready());
        QVERIFY(game.tick().empty());
    }
    QVERIFY(!game.ready());
    QCOMPARE(game.snake().head(), head);
    game.placeFood(kParked);
    stepMove(game);
    QCOMPARE(game.snake().head(), head + QPoint(1, 0));
}

void SnakeTests::pauseHoldsTheSnakeStill() {
    Game game(Mode::Classic, Difficulty::Normal, kSeed);
    skipReady(game);
    game.placeFood(kParked);
    game.setPaused(true);
    QVERIFY(game.paused());
    const QPoint head = game.snake().head();
    for (int i = 0; i < 5 * game.moveTicks(); ++i)
        game.tick();
    QCOMPARE(game.snake().head(), head);
    game.setPaused(false);
    stepMove(game);
    QCOMPARE(game.snake().head(), head + QPoint(1, 0));
}

void SnakeTests::turnsAreTakenOneMoveAtATime() {
    Game game(Mode::Classic, Difficulty::Normal, kSeed);
    skipReady(game);
    game.placeFood(kParked);
    const QPoint head = game.snake().head();

    game.turn(Direction::Up);
    game.turn(Direction::Left);
    QCOMPARE(game.snake().queuedTurns(), 2);

    stepMove(game);
    QCOMPARE(game.snake().heading(), Direction::Up);
    QCOMPARE(game.snake().head(), head + QPoint(0, -1));
    QCOMPARE(game.snake().queuedTurns(), 1);

    stepMove(game);
    QCOMPARE(game.snake().heading(), Direction::Left);
    QCOMPARE(game.snake().head(), head + QPoint(-1, -1));
    QCOMPARE(game.snake().queuedTurns(), 0);
}

void SnakeTests::reversingIsIgnored() {
    Game game(Mode::Classic, Difficulty::Normal, kSeed);
    skipReady(game);
    game.placeFood(kParked);

    // Straight back onto its own neck, and the heading it already has.
    game.turn(Direction::Left);
    QCOMPARE(game.snake().queuedTurns(), 0);
    game.turn(Direction::Right);
    QCOMPARE(game.snake().queuedTurns(), 0);

    // A reversal of a turn already queued is just as impossible.
    game.turn(Direction::Up);
    game.turn(Direction::Down);
    QCOMPARE(game.snake().queuedTurns(), 1);

    const QPoint head = game.snake().head();
    stepMove(game);
    QCOMPARE(game.snake().head(), head + QPoint(0, -1));
}

void SnakeTests::thirdQueuedTurnIsDropped() {
    Game game(Mode::Classic, Difficulty::Normal, kSeed);
    skipReady(game);
    game.placeFood(kParked);
    game.turn(Direction::Up);
    game.turn(Direction::Left);
    game.turn(Direction::Down);
    QCOMPARE(game.snake().queuedTurns(), Snake::kMaxQueuedTurns);

    stepMove(game);
    stepMove(game);
    QCOMPARE(game.snake().heading(), Direction::Left);
    stepMove(game);
    QCOMPARE(game.snake().heading(), Direction::Left);
}

void SnakeTests::eatingGrowsAndScores() {
    Game game(Mode::Classic, Difficulty::Normal, kSeed);
    skipReady(game);
    const QPoint bite = game.snake().head() + QPoint(1, 0);
    game.placeFood(bite);
    const std::vector<Event> events = stepMove(game);

    QCOMPARE(game.length(), Game::kStartLength + 1);
    QCOMPARE(game.score(), Game::kFoodScore);
    QCOMPARE(game.foodsEaten(), 1);
    const Event ate = find(events, Event::Ate);
    QCOMPARE(ate.at, bite);
    QCOMPARE(ate.points, Game::kFoodScore);
    QCOMPARE(ate.multiplier, 1);
    QVERIFY(!has(events, Event::SpeedUp));
    QVERIFY(game.food() != bite);
    QVERIFY(!game.snake().occupies(game.food()));
}

void SnakeTests::lengthMultiplierClimbsWithTheSnake() {
    Game game(Mode::Wrap, Difficulty::Normal, kSeed);
    const std::vector<QPoint> path = serpentine();
    const std::vector<std::pair<int, int>> ladder = {{4, 1},  {11, 1}, {12, 2}, {19, 2}, {20, 3},
                                                    {27, 3}, {28, 4}, {36, 5}, {60, 5}};
    for (const auto &[length, multiplier] : ladder) {
        game.placeSnake(bodyAlong(path, length), Direction::Right);
        QCOMPARE(game.length(), length);
        QCOMPARE(game.multiplier(), multiplier);
    }

    // The food that takes the snake to twelve already pays double.
    game.placeSnake(row({10, 5}, 11), Direction::Right);
    skipReady(game);
    game.placeFood({11, 5});
    const std::vector<Event> events = stepMove(game);
    QCOMPARE(game.length(), 12);
    QCOMPARE(find(events, Event::Ate).multiplier, 2);
    QCOMPARE(game.score(), 2 * Game::kFoodScore);
}

void SnakeTests::wallEndsTheGameWithNoGraceMove() {
    Game game(Mode::Classic, Difficulty::Normal, kSeed);
    skipReady(game);
    game.placeFood(kParked);
    game.placeSnake(row({Game::kWidth - 1, 5}, Game::kStartLength), Direction::Right);

    const std::vector<Event> events = stepMove(game);
    QCOMPARE(game.phase(), Phase::GameOver);
    QCOMPARE(game.death(), Death::Wall);
    QVERIFY(has(events, Event::GameOver));
    // The snake never leaves the field, and never gets a beat to turn away.
    QCOMPARE(game.snake().head(), QPoint(Game::kWidth - 1, 5));
    QCOMPARE(game.length(), Game::kStartLength);
}

void SnakeTests::wrapModeCarriesTheSnakeAcross() {
    Game game(Mode::Wrap, Difficulty::Normal, kSeed);
    skipReady(game);
    game.placeFood(kParked);

    game.placeSnake(row({Game::kWidth - 1, 5}, Game::kStartLength), Direction::Right);
    stepMove(game);
    QCOMPARE(game.phase(), Phase::Playing);
    QCOMPARE(game.snake().head(), QPoint(0, 5));

    game.placeSnake(row({10, 0}, Game::kStartLength), Direction::Right);
    game.turn(Direction::Up);
    stepMove(game);
    QCOMPARE(game.snake().head(), QPoint(10, Game::kHeight - 1));
}

void SnakeTests::runningIntoItselfEndsTheGame() {
    Game game(Mode::Wrap, Difficulty::Normal, kSeed);
    skipReady(game);
    game.placeFood(kParked);
    // A hook whose head turns back into the middle of its own body.
    game.placeSnake({{10, 10}, {10, 11}, {11, 11}, {11, 10}, {12, 10}}, Direction::Right);

    const std::vector<Event> events = stepMove(game);
    QCOMPARE(game.phase(), Phase::GameOver);
    QCOMPARE(game.death(), Death::Self);
    QVERIFY(has(events, Event::GameOver));
}

void SnakeTests::followingItsOwnTailIsLegal() {
    Game game(Mode::Wrap, Difficulty::Normal, kSeed);
    skipReady(game);
    game.placeFood(kParked);
    // The same hook, one cell shorter: the head lands where the tail leaves.
    game.placeSnake({{10, 10}, {10, 11}, {11, 11}, {11, 10}}, Direction::Right);

    stepMove(game);
    QCOMPARE(game.phase(), Phase::Playing);
    QCOMPARE(game.snake().head(), QPoint(11, 10));
}

void SnakeTests::gameOverFreezesEverything() {
    Game game(Mode::Classic, Difficulty::Normal, kSeed);
    skipReady(game);
    game.placeFood(kParked);
    game.placeSnake(row({Game::kWidth - 1, 5}, Game::kStartLength), Direction::Right);
    stepMove(game);
    QCOMPARE(game.phase(), Phase::GameOver);

    const QPoint head = game.snake().head();
    const int score = game.score();
    for (int i = 0; i < 100; ++i)
        QVERIFY(game.tick().empty());
    QCOMPARE(game.snake().head(), head);
    QCOMPARE(game.score(), score);
}
