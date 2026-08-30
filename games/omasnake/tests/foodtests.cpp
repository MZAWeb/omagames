#include "foodtests.h"

#include <QtTest>

#include "scenario.h"

using namespace Scenario;

void FoodTests::foodNeverLandsOnTheSnake() {
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

void FoodTests::fillingTheBoardEndsThePerfectGame() {
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

void FoodTests::bonusFollowsEveryFifthFood() {
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

void FoodTests::bonusExpiresAfterItsLifetime() {
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

void FoodTests::bonusEatenScoresItsFlatFifty() {
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

void FoodTests::sameSeedSameGame() {
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
