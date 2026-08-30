#include "enginetests.h"

#include <QtTest>
#include <algorithm>
#include <deque>
#include <utility>
#include <vector>

#include "game.h"

namespace {

constexpr quint32 kSeed = 20260830u;
// Somewhere no scripted snake ever walks, so a scenario's food stays put.
constexpr QPoint kParked {1, 1};

void skipReady(Game &game) {
    for (int i = 0; i < Game::kReadyTicks; ++i)
        game.tick();
}

// One whole move, and everything it reported.
std::vector<Event> stepMove(Game &game) {
    std::vector<Event> all;
    const int ticks = game.moveTicks();
    for (int i = 0; i < ticks; ++i) {
        const std::vector<Event> events = game.tick();
        all.insert(all.end(), events.begin(), events.end());
    }
    return all;
}

bool has(const std::vector<Event> &events, Event::Type type) {
    return std::any_of(events.begin(), events.end(), [type](const Event &e) { return e.type == type; });
}

Event find(const std::vector<Event> &events, Event::Type type) {
    for (const Event &e : events) {
        if (e.type == type)
            return e;
    }
    return {Event::GameOver, {-1, -1}};
}

// A body of `length` cells laid out in a row, head first, heading right.
std::deque<QPoint> row(QPoint head, int length) {
    std::deque<QPoint> body;
    for (int i = 0; i < length; ++i)
        body.push_back(head - QPoint(i, 0));
    return body;
}

// A Hamiltonian path over the whole board: every row swept in the opposite
// direction to the one above it, so consecutive cells are always neighbours.
std::vector<QPoint> serpentine() {
    std::vector<QPoint> path;
    path.reserve(size_t(Game::kWidth * Game::kHeight));
    for (int y = 0; y < Game::kHeight; ++y) {
        for (int i = 0; i < Game::kWidth; ++i)
            path.push_back({y % 2 == 0 ? i : Game::kWidth - 1 - i, y});
    }
    return path;
}

// The first `count` cells of a path as a snake, head on the last of them.
std::deque<QPoint> bodyAlong(const std::vector<QPoint> &path, int count) {
    std::deque<QPoint> body;
    for (int i = 0; i < count; ++i)
        body.push_front(path[size_t(i)]);
    return body;
}

Direction headingFrom(QPoint from, QPoint to) {
    const QPoint step = to - from;
    if (step == QPoint(1, 0))
        return Direction::Right;
    if (step == QPoint(-1, 0))
        return Direction::Left;
    if (step == QPoint(0, -1))
        return Direction::Up;
    return Direction::Down;
}

}  // namespace

void EngineTests::snakeStartsCentredAndKeepsItsHeading() {
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

void EngineTests::readyBeatHoldsTheSnakeStill() {
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

void EngineTests::pauseHoldsTheSnakeStill() {
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

void EngineTests::turnsAreTakenOneMoveAtATime() {
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

void EngineTests::reversingIsIgnored() {
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

void EngineTests::thirdQueuedTurnIsDropped() {
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

void EngineTests::eatingGrowsAndScores() {
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

void EngineTests::lengthMultiplierClimbsWithTheSnake() {
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

void EngineTests::wallEndsTheGameWithNoGraceMove() {
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

void EngineTests::wrapModeCarriesTheSnakeAcross() {
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

void EngineTests::runningIntoItselfEndsTheGame() {
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

void EngineTests::followingItsOwnTailIsLegal() {
    Game game(Mode::Wrap, Difficulty::Normal, kSeed);
    skipReady(game);
    game.placeFood(kParked);
    // The same hook, one cell shorter: the head lands where the tail leaves.
    game.placeSnake({{10, 10}, {10, 11}, {11, 11}, {11, 10}}, Direction::Right);

    stepMove(game);
    QCOMPARE(game.phase(), Phase::Playing);
    QCOMPARE(game.snake().head(), QPoint(11, 10));
}

void EngineTests::gameOverFreezesEverything() {
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

void EngineTests::foodNeverLandsOnTheSnake() {
    // A snake filling all but seven cells: a dot placed carelessly would
    // land on it almost every time.
    const std::vector<QPoint> path = serpentine();
    const int filled = int(path.size()) - 7;
    for (quint32 seed = 1; seed <= 200; ++seed) {
        Game game(Mode::Wrap, Difficulty::Fast, seed);
        skipReady(game);
        game.placeSnake(bodyAlong(path, filled), headingFrom(path[size_t(filled - 1)], path[size_t(filled)]));
        game.placeFood(path[size_t(filled)]);
        stepMove(game);

        QCOMPARE(game.length(), filled + 1);
        QVERIFY(Game::contains(game.food()));
        QVERIFY(!game.snake().occupies(game.food()));
    }
}

void EngineTests::fillingTheBoardEndsThePerfectGame() {
    const std::vector<QPoint> path = serpentine();
    const int filled = int(path.size()) - 1;
    Game game(Mode::Wrap, Difficulty::Normal, kSeed);
    skipReady(game);
    game.placeSnake(bodyAlong(path, filled), headingFrom(path[size_t(filled - 1)], path[size_t(filled)]));
    game.placeFood(path[size_t(filled)]);

    const std::vector<Event> events = stepMove(game);
    QCOMPARE(game.length(), Game::kWidth * Game::kHeight);
    QVERIFY(has(events, Event::Ate));
    QCOMPARE(game.phase(), Phase::GameOver);
    QCOMPARE(game.death(), Death::Filled);
}

void EngineTests::bonusFollowsEveryFifthFood() {
    Game game(Mode::Wrap, Difficulty::Normal, kSeed);
    skipReady(game);
    game.placeSnake(row({3, 12}, Game::kStartLength), Direction::Right);

    std::vector<Event> last;
    for (int i = 0; i < Game::kFoodsPerBonus; ++i) {
        QCOMPARE(game.hasBonus(), false);
        game.placeFood(game.snake().head() + QPoint(1, 0));
        last = stepMove(game);
    }
    QCOMPARE(game.foodsEaten(), Game::kFoodsPerBonus);
    QVERIFY(has(last, Event::BonusSpawned));
    // Normal shaves a tick off the move period every five foods.
    QVERIFY(has(last, Event::SpeedUp));
    QVERIFY(game.hasBonus());
    QCOMPARE(game.bonusRemaining(), 1.0);
    QVERIFY(Game::contains(game.bonus()));
    QVERIFY(!game.snake().occupies(game.bonus()));
    QVERIFY(game.bonus() != game.food());
}

void EngineTests::bonusExpiresAfterItsLifetime() {
    Game game(Mode::Wrap, Difficulty::Normal, kSeed);
    skipReady(game);
    // A lap of an empty row, with the dots well out of the snake's way.
    game.placeSnake(row({3, 2}, Game::kStartLength), Direction::Right);
    game.placeFood({0, 0});
    game.placeBonus({5, 20}, Game::kBonusLifetimeTicks);

    std::vector<Event> events;
    for (int i = 0; i < Game::kBonusLifetimeTicks - 1; ++i) {
        const std::vector<Event> tickEvents = game.tick();
        events.insert(events.end(), tickEvents.begin(), tickEvents.end());
    }
    QVERIFY(game.hasBonus());
    QVERIFY(game.bonusRemaining() > 0.0);
    QVERIFY(game.bonusRemaining() < 0.01);
    QVERIFY(!has(events, Event::BonusExpired));

    const std::vector<Event> last = game.tick();
    QVERIFY(has(last, Event::BonusExpired));
    QCOMPARE(find(last, Event::BonusExpired).at, QPoint(5, 20));
    QVERIFY(!game.hasBonus());
    QCOMPARE(game.bonusRemaining(), 0.0);
    QCOMPARE(game.score(), 0);
}

void EngineTests::bonusEatenScoresItsFlatFifty() {
    Game game(Mode::Wrap, Difficulty::Normal, kSeed);
    skipReady(game);
    game.placeSnake(row({10, 8}, Game::kStartLength), Direction::Right);
    game.placeFood(kParked);
    game.placeBonus({11, 8}, Game::kBonusLifetimeTicks);

    const std::vector<Event> events = stepMove(game);
    QVERIFY(has(events, Event::BonusEaten));
    QCOMPARE(find(events, Event::BonusEaten).points, Game::kBonusScore);
    QCOMPARE(game.score(), Game::kBonusScore);
    QCOMPARE(game.length(), Game::kStartLength + 1);
    QCOMPARE(game.foodsEaten(), 0);
    QVERIFY(!game.hasBonus());
}

void EngineTests::speedRampsWithFoodEaten() {
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

void EngineTests::theThreeDifficultiesFormALadder() {
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

void EngineTests::sameSeedSameGame() {
    // The snake is parked beside every dot the generator produces, so the run
    // is all randomness and no luck: same seed, same forty dots.
    const auto play = [](quint32 seed) {
        Game game(Mode::Wrap, Difficulty::Normal, seed);
        skipReady(game);
        std::vector<QPoint> dots;
        for (int i = 0; i < 40; ++i) {
            const QPoint dot = game.food();
            dots.push_back(dot);
            std::deque<QPoint> body;
            for (int c = 0; c < Game::kStartLength; ++c)
                body.push_back({(dot.x() - 1 - c + Game::kWidth) % Game::kWidth, dot.y()});
            game.placeSnake(body, Direction::Right);
            for (const Event &event : stepMove(game)) {
                if (event.type == Event::BonusSpawned)
                    dots.push_back(event.at);
            }
        }
        return std::make_pair(game.score(), dots);
    };

    const std::pair<int, std::vector<QPoint>> first = play(kSeed);
    QCOMPARE(first.first, 40 * Game::kFoodScore);
    QCOMPARE(int(first.second.size()), 40 + 40 / Game::kFoodsPerBonus);
    QVERIFY(play(kSeed) == first);
    QVERIFY(play(kSeed + 1) != first);
}
