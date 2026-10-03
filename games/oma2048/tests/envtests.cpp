#include "envtests.h"

#include <QJsonArray>
#include <QRandomGenerator>
#include <QtTest>

#include <vector>

#include "envconfig.h"
#include "oma2048env.h"

namespace {

constexpr quint32 kSeed = 20261003u;

// An env as the ABI would build it: the config resolved against the schema.
Oma2048Env makeEnv(const QJsonObject &config = {}) {
    QString error;
    const auto resolved = OmaGames::EnvConfig::resolve(
        OmaGames::envGameSpec().value(QStringLiteral("config")).toObject(), config, &error);
    Oma2048Env env;
    env.configure(*resolved);
    env.reset(kSeed);
    return env;
}

std::vector<quint8> observed(const Oma2048Env &env) {
    std::vector<quint8> buffer(size_t(env.observationLayout().size()));
    env.observe(reinterpret_cast<std::byte *>(buffer.data()));
    return buffer;
}

// A legal move, at random.
int pick(const Oma2048Env &env, QRandomGenerator &rng) {
    quint8 mask[Oma2048Env::kActions];
    env.actionMask(mask);
    std::vector<int> legal;
    for (int a = 0; a < Oma2048Env::kActions; ++a) {
        if (mask[a])
            legal.push_back(a);
    }
    return legal[rng.bounded(int(legal.size()))];
}

int log2(int value) {
    int power = 0;
    while (value > 1) {
        value /= 2;
        ++power;
    }
    return power;
}

}  // namespace

void EnvTests::theSpecDescribesTheGame() {
    const Oma2048Env env = makeEnv();
    QCOMPARE(env.actionCount(), 4);
    QCOMPARE(env.actionLabels(), (QStringList{QStringLiteral("left"), QStringLiteral("right"),
                                              QStringLiteral("up"), QStringLiteral("down")}));
    QCOMPARE(env.signalNames().size(), 5);
    const QJsonObject spec = OmaGames::envGameSpec();
    QCOMPARE(spec.value(QStringLiteral("game")).toString(), QStringLiteral("oma2048"));
    QCOMPARE(spec.value(QStringLiteral("rules_version")).toInt(), Rules::kVersion);
}

void EnvTests::theMaskAndAfterstatesAreTheRealSlides() {
    Oma2048Env env = makeEnv();
    QRandomGenerator rng(4);
    const int cells = Board::kSize * Board::kSize;
    for (int move = 0; move < 60 && !env.game().over(); ++move) {
        const std::vector<quint8> buffer = observed(env);
        const quint8 *after = buffer.data() + cells;  // board first, then the afterstates
        quint8 mask[Oma2048Env::kActions];
        env.actionMask(mask);
        for (int action = 0; action < Oma2048Env::kActions; ++action) {
            Board slid = env.game().board();
            const MoveResult result = slid.move(Direction(action));
            QCOMPARE(bool(mask[action]), result.moved);
            for (int i = 0; i < cells && result.moved; ++i)
                QCOMPARE(int(after[action * cells + i]), log2(slid.valueAt(i / Board::kSize, i % Board::kSize)));
        }
        env.step(pick(env, rng));
    }
}

void EnvTests::aStepSlidesSpawnsAndPaysTheScore() {
    Oma2048Env env = makeEnv();
    QRandomGenerator rng(9);
    for (int move = 0; move < 80 && !env.game().over(); ++move) {
        const int score = env.game().score();
        const int tiles = int(env.game().board().tiles().size());
        const OmaGames::EnvStep step = env.step(pick(env, rng));
        QCOMPARE(step.reward, double(env.game().score() - score));
        // Merges took tiles away; one spawned.
        QCOMPARE(int(env.game().board().tiles().size()), tiles - int(step.signalValues[1]) + 1);
        QCOMPARE(step.signalValues[2], double(env.game().board().highestValue()));
    }
    QCOMPARE(env.info().value(QStringLiteral("moves")).toInt() > 0, true);
}

void EnvTests::theSameSeedAndMovesPlayTheSameGame() {
    Oma2048Env a = makeEnv(), b = makeEnv();
    QRandomGenerator rng(2);
    while (!a.game().over()) {
        const int action = pick(a, rng);
        a.step(action);
        b.step(action);
    }
    QCOMPARE(b.game().score(), a.game().score());
    QVERIFY(b.game().over());
    QCOMPARE(observed(b), observed(a));
}

void EnvTests::aReseededCopyKeepsTheBoardNotTheSpawns() {
    Oma2048Env env = makeEnv();
    const auto same = env.clone(false, 0);
    const auto other = env.clone(true, 77);
    QCOMPARE(observed(static_cast<const Oma2048Env &>(*same)), observed(env));
    QCOMPARE(observed(static_cast<const Oma2048Env &>(*other)), observed(env));  // the board is visible
    // Thirty moves on, the spawns have parted ways.
    QRandomGenerator rng(6);
    bool differed = false;
    for (int move = 0; move < 30 && !env.game().over(); ++move) {
        const int action = pick(env, rng);
        quint8 mask[Oma2048Env::kActions];
        other->actionMask(mask);
        if (!mask[action])
            break;
        env.step(action);
        other->step(action);
        differed |= observed(static_cast<const Oma2048Env &>(*other)) != observed(env);
    }
    QVERIFY(differed);
}

void EnvTests::theGoalEndsTheRun() {
    Oma2048Env env = makeEnv({{QStringLiteral("goal"), 32}});
    QRandomGenerator rng(1);
    OmaGames::EnvStep step;
    for (int guard = 0; guard < 2000 && !step.terminated; ++guard)
        step = env.step(pick(env, rng));
    QVERIFY(step.terminated);
    QVERIFY(env.game().board().highestValue() >= 32 || env.game().over());
    if (!env.game().over())
        QCOMPARE(env.info().value(QStringLiteral("phase")).toString(), QStringLiteral("finished"));
}

void EnvTests::aReplayIsDrawnMoveByMove() {
    Oma2048Env env = makeEnv();
    QRandomGenerator rng(3);
    int moves = 0;
    for (; moves < 100 && !env.game().over(); ++moves)
        env.step(pick(env, rng));
    QString error;
    const auto drawn = env.frames(env.replay(), &error);
    QVERIFY2(drawn, qPrintable(error));
    const QJsonArray frames = drawn->value(QStringLiteral("frames")).toArray();
    QCOMPARE(frames.size(), moves + 1);  // the deal, then one per move
    const QJsonObject last = frames.last().toObject();
    QCOMPARE(last.value(QStringLiteral("score")).toInt(), env.game().score());
    const QJsonArray board = last.value(QStringLiteral("board")).toArray();
    for (int i = 0; i < board.size(); ++i)
        QCOMPARE(board.at(i).toInt(), env.game().board().valueAt(i / Board::kSize, i % Board::kSize));

    OmaGames::Replay other(QStringLiteral("omatris"), 1, {}, 1);
    QVERIFY(!env.frames(other, &error));
}
