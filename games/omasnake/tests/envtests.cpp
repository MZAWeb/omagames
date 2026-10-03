#include "envtests.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QRandomGenerator>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include "choices.h"
#include "envconfig.h"
#include "omagames_env.h"
#include "omasnakeenv.h"

namespace {

constexpr quint32 kSeed = 20260830u;

OmasnakeEnv makeEnv(const QJsonObject &config = {}) {
    QString error;
    const auto resolved = OmaGames::EnvConfig::resolve(
        OmaGames::envGameSpec().value(QStringLiteral("config")).toObject(), config, &error);
    OmasnakeEnv env;
    env.configure(*resolved);
    env.reset(kSeed);
    return env;
}

OmasnakeEnv absoluteEnv(const QString &mode = QStringLiteral("classic")) {
    return makeEnv({{QStringLiteral("actions"), QStringLiteral("absolute")}, {QStringLiteral("mode"), mode}});
}

std::vector<quint8> mask(const OmasnakeEnv &env) {
    std::vector<quint8> bytes(size_t(env.actionCount()));
    env.actionMask(bytes.data());
    return bytes;
}

std::vector<std::byte> observe(const OmasnakeEnv &env) {
    std::vector<std::byte> bytes(size_t(env.observationLayout().size()));
    env.observe(bytes.data());
    return bytes;
}

// The nearest-food player every Snake tutorial starts with: of the legal
// absolute moves that do not walk into a wall or the body, the one that ends
// closest to the food. Enough to eat a good many dots before boxing itself in.
int greedy(const OmasnakeEnv &env) {
    const Game &game = env.game();
    const std::vector<quint8> legal = mask(env);
    int best = -1;
    int bestDistance = 0;
    for (int action = 0; action < OmasnakeEnv::kAbsoluteActions; ++action) {
        if (!legal[size_t(action)])
            continue;
        const QPoint next = game.snake().head() + delta(Direction(action));
        if (!Game::contains(next) || game.snake().occupies(next, true))
            continue;
        const int distance = std::abs(next.x() - game.food().x()) + std::abs(next.y() - game.food().y());
        if (best < 0 || distance < bestDistance) {
            best = action;
            bestDistance = distance;
        }
    }
    return best < 0 ? int(game.snake().heading()) : best;
}

// Steers greedily until a dot is eaten; the step that ate it.
OmaGames::EnvStep eatOne(OmasnakeEnv &env) {
    OmaGames::EnvStep step;
    for (int guard = 0; guard < 200 && !step.terminated && step.signalValues[1] == 0; ++guard)
        step = env.step(greedy(env));
    return step;
}

// What a replay amounts to: the same game dealt, the same turns at the
// same ticks.
Game playBack(const OmaGames::Replay &replay) {
    Mode mode = Mode::Classic;
    Difficulty difficulty = Difficulty::Normal;
    Modes::fromId(replay.config().value(QStringLiteral("mode")).toString(), &mode);
    Difficulties::fromId(replay.config().value(QStringLiteral("difficulty")).toString(), &difficulty);
    Game game(mode, difficulty, replay.seed());
    const QStringList turns {QStringLiteral("U"), QStringLiteral("D"), QStringLiteral("L"), QStringLiteral("R")};
    for (const OmaGames::Replay::Step &step : replay.steps()) {
        for (int i = 0; i < step.ticks; ++i)
            game.tick();
        if (!step.input.isEmpty())
            game.turn(Direction(turns.indexOf(step.input)));
    }
    return game;
}

}  // namespace

void EnvTests::initTestCase() {
    static QTemporaryDir dir;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());
    QVERIFY(dir.isValid());
}

void EnvTests::theSpecDescribesBothActionSpaces() {
    const OmasnakeEnv relative = makeEnv();
    QCOMPARE(relative.actionCount(), 3);
    QCOMPARE(relative.actionLabels().at(OmasnakeEnv::TurnLeft), QStringLiteral("turn_left"));
    const OmasnakeEnv absolute = absoluteEnv();
    QCOMPARE(absolute.actionCount(), 4);
    QCOMPARE(absolute.actionLabels().at(int(Direction::Down)), QStringLiteral("down"));

    const QJsonArray tensors = relative.observationLayout().toJson().value(QStringLiteral("tensors")).toArray();
    QCOMPARE(tensors[0].toObject().value(QStringLiteral("shape")).toArray(), (QJsonArray{Game::kHeight, Game::kWidth}));
    QCOMPARE(tensors[1].toObject().value(QStringLiteral("labels")).toArray().at(9).toString(), QStringLiteral("score"));
    QCOMPARE(relative.signalNames().size(), 7);
}

void EnvTests::aStepIsOneMove() {
    OmasnakeEnv env = makeEnv();
    const QPoint head = env.game().snake().head();
    // The first move waits out the opening beat as well.
    const OmaGames::EnvStep first = env.step(OmasnakeEnv::Straight);
    QCOMPARE(first.ticks, qint64(Game::kReadyTicks + env.game().moveTicks()));
    QCOMPARE(env.game().snake().head(), head + QPoint(1, 0));
    const OmaGames::EnvStep second = env.step(OmasnakeEnv::Straight);
    QCOMPARE(second.ticks, qint64(env.game().moveTicks()));
    QCOMPARE(env.game().snake().head(), head + QPoint(2, 0));
    QCOMPARE(second.reward, 0.0);
}

void EnvTests::relativeTurnsAreAgainstTheHeading() {
    OmasnakeEnv env = makeEnv();
    QCOMPARE(env.game().snake().heading(), Direction::Right);
    env.step(OmasnakeEnv::TurnLeft);
    QCOMPARE(env.game().snake().heading(), Direction::Up);
    env.step(OmasnakeEnv::TurnLeft);
    QCOMPARE(env.game().snake().heading(), Direction::Left);
    env.step(OmasnakeEnv::TurnRight);
    QCOMPARE(env.game().snake().heading(), Direction::Up);
    env.step(OmasnakeEnv::TurnRight);
    QCOMPARE(env.game().snake().heading(), Direction::Right);
    QCOMPARE(mask(env), (std::vector<quint8>{1, 1, 1}));
}

void EnvTests::absoluteMasksTheWayBack() {
    OmasnakeEnv env = absoluteEnv();
    QCOMPARE(mask(env), (std::vector<quint8>{1, 1, 0, 1}));  // heading right: left is the neck
    const QPoint head = env.game().snake().head();
    env.step(int(Direction::Down));
    QCOMPARE(env.game().snake().head(), head + QPoint(0, 1));
    QCOMPARE(mask(env), (std::vector<quint8>{0, 1, 1, 1}));
}

void EnvTests::eatingRewardsAndGrows() {
    OmasnakeEnv env = absoluteEnv();
    const int length = env.game().length();
    const OmaGames::EnvStep ate = eatOne(env);
    QCOMPARE(ate.signalValues[1], 1.0);
    QCOMPARE(ate.reward, double(Game::kFoodScore));
    QCOMPARE(ate.signalValues[0], ate.reward);
    QCOMPARE(ate.signalValues[3], double(length + 1));
    QCOMPARE(env.game().length(), length + 1);
    QVERIFY(ate.signalValues[4] > 0);  // the next dot is somewhere else
    // Enough dots in a row to see the multiplier and the bonus come round.
    int foods = 1;
    for (int i = 0; i < 12 && env.game().phase() == Phase::Playing; ++i)
        foods += int(eatOne(env).signalValues[1]);
    QVERIFY(foods >= Game::kFoodsPerBonus);
}

void EnvTests::theWallEndsAClassicRunAndAWrapRunCrossesIt() {
    OmasnakeEnv classic = makeEnv();
    OmaGames::EnvStep step;
    int moves = 0;
    while (!step.terminated && moves < Game::kWidth) {
        step = classic.step(OmasnakeEnv::Straight);
        ++moves;
    }
    QVERIFY(step.terminated);
    QCOMPARE(step.signalValues[5], 1.0);
    QCOMPARE(classic.info().value(QStringLiteral("death")).toString(), QStringLiteral("wall"));

    OmasnakeEnv wrap = makeEnv({{QStringLiteral("mode"), QStringLiteral("wrap")}});
    for (int i = 0; i < Game::kWidth + 4; ++i)
        QVERIFY(!wrap.step(OmasnakeEnv::Straight).terminated);
}

void EnvTests::theGridShowsTheSnakeAndItsFood() {
    OmasnakeEnv env = makeEnv();
    env.step(OmasnakeEnv::Straight);
    const std::vector<std::byte> bytes = observe(env);
    const Game &game = env.game();
    auto cell = [&bytes](QPoint p) { return int(bytes[size_t(p.y() * Game::kWidth + p.x())]); };
    QCOMPARE(cell(game.snake().head()), 2);
    QCOMPARE(cell(game.snake().tail()), 3);
    QCOMPARE(cell(game.snake().body()[1]), 1);
    QCOMPARE(cell(game.food()), 4);
    const int offset = env.observationLayout().toJson().value(QStringLiteral("tensors")).toArray()[1]
                           .toObject().value(QStringLiteral("offset")).toInt();
    qint32 state[12];
    std::memcpy(state, bytes.data() + offset, sizeof state);
    QCOMPARE(state[0], game.snake().head().x());
    QCOMPARE(state[2], int(Direction::Right));
    QCOMPARE(state[3], game.length());
    QCOMPARE(state[4], game.food().x());
    QCOMPARE(state[6], -1);  // no bonus yet
}

void EnvTests::theSameSeedAndTurnsPlayTheSameGame() {
    OmasnakeEnv a = makeEnv({{QStringLiteral("mode"), QStringLiteral("wrap")}});
    OmasnakeEnv b = makeEnv({{QStringLiteral("mode"), QStringLiteral("wrap")}});
    QRandomGenerator rng(4);
    for (int i = 0; i < 300 && a.game().phase() == Phase::Playing; ++i) {
        const int action = int(rng.bounded(OmasnakeEnv::kRelativeActions));
        a.step(action);
        b.step(action);
        QVERIFY(observe(a) == observe(b));
    }
}

void EnvTests::theReplayIsTheGame() {
    OmasnakeEnv env = absoluteEnv(QStringLiteral("wrap"));
    for (int i = 0; i < 8 && env.game().phase() == Phase::Playing; ++i)
        eatOne(env);
    QVERIFY(env.game().foodsEaten() > 0);
    QString error;
    const auto replay = OmaGames::Replay::fromJson(env.replay().toJson(), &error);
    QVERIFY2(replay, qPrintable(error));
    QCOMPARE(replay->game(), QStringLiteral("omasnake"));
    QCOMPARE(replay->rulesVersion(), Rules::kVersion);
    const Game played = playBack(*replay);
    QCOMPARE(played.score(), env.game().score());
    QCOMPARE(played.snake().body(), env.game().snake().body());
    QCOMPARE(played.food(), env.game().food());
}

void EnvTests::aReseededCloneHidesWhereFoodWillLand() {
    OmasnakeEnv env = absoluteEnv(QStringLiteral("wrap"));
    eatOne(env);
    bool moved = false;
    for (quint32 seed = 1; seed <= 5; ++seed) {
        std::unique_ptr<OmaGames::Env> copy = env.clone(true, seed);
        auto *clone = static_cast<OmasnakeEnv *>(copy.get());
        QVERIFY(observe(*clone) == observe(env));  // the dot already showing stays
        QVERIFY(clone->replay().game().isEmpty());
        OmasnakeEnv original = env;
        eatOne(*clone);
        eatOne(original);
        moved = moved || clone->game().food() != original.game().food();
    }
    QVERIFY(moved);
}

void EnvTests::theEnvLeavesSettingsAlone() {
    QSettings().clear();
    OmasnakeEnv env = absoluteEnv();
    while (env.game().phase() == Phase::Playing)
        env.step(greedy(env));
    QVERIFY(env.game().foodsEaten() > 0);
    QVERIFY(QSettings().allKeys().isEmpty());
}

void EnvTests::theAbiPlaysOmasnake() {
    const QJsonObject spec = QJsonDocument::fromJson(og_game_spec()).object();
    QCOMPARE(spec.value(QStringLiteral("game")).toString(), QStringLiteral("omasnake"));
    OgEnv *env = og_create(R"({"actions": "absolute", "difficulty": "fast"})");
    QVERIFY(env);
    og_reset(env, kSeed);
    OgStepResult result {};
    QCOMPARE(og_step(env, int(Direction::Left), &result), -1);  // the neck
    QCOMPARE(og_step(env, int(Direction::Up), &result), 0);
    QVERIFY(!og_create(R"({"difficulty": "insane"})"));
    og_destroy(env);
}
