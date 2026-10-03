#include "envtests.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QRandomGenerator>
#include <QSettings>
#include <QtTest>

#include "bridgefixture.h"
#include "envconfig.h"
#include "omagames_env.h"
#include "omatrisenv.h"

namespace {

constexpr quint32 kSeed = 20260830u;

// An env as the ABI would build it: the config resolved against the game's
// schema, so every key the env reads is there.
OmatrisEnv makeEnv(const QJsonObject &config = {}) {
    QString error;
    const auto resolved = OmaGames::EnvConfig::resolve(
        OmaGames::envGameSpec().value(QStringLiteral("config")).toObject(), config, &error);
    OmatrisEnv env;
    env.configure(*resolved);
    env.reset(kSeed);
    return env;
}

std::vector<quint8> mask(const OmatrisEnv &env) {
    std::vector<quint8> bytes(size_t(env.actionCount()));
    env.actionMask(bytes.data());
    return bytes;
}

std::vector<std::byte> observe(const OmatrisEnv &env) {
    std::vector<std::byte> bytes(size_t(env.observationLayout().size()));
    env.observe(bytes.data());
    return bytes;
}

QJsonObject tensor(const OmatrisEnv &env, const QString &name) {
    for (const QJsonValue &t : env.observationLayout().toJson().value(QStringLiteral("tensors")).toArray()) {
        if (t.toObject().value(QStringLiteral("name")).toString() == name)
            return t.toObject();
    }
    return {};
}

int legal(const OmatrisEnv &env) {
    const std::vector<quint8> bytes = mask(env);
    return int(std::count(bytes.begin(), bytes.end(), quint8(1)));
}

// Picks among the legal actions from `rng`, as a random agent would.
int pick(const OmatrisEnv &env, QRandomGenerator &rng) {
    const std::vector<quint8> bytes = mask(env);
    std::vector<int> actions;
    for (int i = 0; i < int(bytes.size()); ++i) {
        if (bytes[size_t(i)])
            actions.push_back(i);
    }
    return actions[rng.bounded(int(actions.size()))];
}

// What the app will do with a replay: deal the same game and make the same
// calls.
Game playBack(const OmaGames::Replay &replay) {
    Mode mode = Mode::Marathon;
    Modes::fromId(replay.config().value(QStringLiteral("mode")).toString(), &mode);
    Game game(mode, replay.seed());
    game.setSoftDropFactor(replay.config().value(QStringLiteral("soft_drop_factor")).toInt());
    for (const OmaGames::Replay::Step &step : replay.steps()) {
        for (int i = 0; i < step.ticks; ++i)
            game.tick();
        if (!step.input.isEmpty())
            Calls::apply(game, *Calls::fromToken(step.input));
    }
    return game;
}

bool sameBoard(const Game &a, const Game &b) {
    for (int i = 0; i < Board::kCellCount; ++i) {
        if (a.board().at(i) != b.board().at(i))
            return false;
    }
    return true;
}

}  // namespace

void EnvTests::initTestCase() {
    QVERIFY(!BridgeFixture::redirectSettings().isEmpty());
}

void EnvTests::theSpecDescribesEveryActionSpace() {
    const OmatrisEnv placement = makeEnv();
    QCOMPARE(placement.actionCount(), 128);
    QVERIFY(!tensor(placement, QStringLiteral("afterstates")).isEmpty());
    QCOMPARE(tensor(placement, QStringLiteral("candidates")).value(QStringLiteral("labels")).toArray().at(6).toString(),
             QStringLiteral("lines"));

    const OmatrisEnv drop = makeEnv({{QStringLiteral("actions"), QStringLiteral("drop")}});
    QCOMPARE(drop.actionCount(), 80);
    QCOMPARE(drop.actionLabels().first(), QStringLiteral("r0c0"));
    QCOMPARE(drop.actionLabels().last(), QStringLiteral("hold_r3c9"));
    QVERIFY(tensor(drop, QStringLiteral("candidates")).isEmpty());

    const OmatrisEnv raw = makeEnv({{QStringLiteral("actions"), QStringLiteral("raw")}});
    QCOMPARE(raw.actionCount(), 8);
    QCOMPARE(raw.actionLabels().at(OmatrisEnv::HardDrop), QStringLiteral("hard_drop"));
    QCOMPARE(raw.signalNames().size(), 8);
    QCOMPARE(tensor(raw, QStringLiteral("board")).value(QStringLiteral("shape")).toArray(), (QJsonArray{24, 10}));
}

void EnvTests::placementOffersEveryLanding() {
    const OmatrisEnv env = makeEnv();
    const std::vector<Landing> found = Placements::find(env.game(), true);
    QCOMPARE(int(env.landings().size()), int(found.size()));
    QCOMPARE(legal(env), int(found.size()));

    const std::vector<std::byte> bytes = observe(env);
    const int rows = tensor(env, QStringLiteral("candidates")).value(QStringLiteral("offset")).toInt();
    const int boards = tensor(env, QStringLiteral("afterstates")).value(QStringLiteral("offset")).toInt();
    for (size_t i = 0; i < found.size(); ++i) {
        qint32 row[9];
        std::memcpy(row, bytes.data() + rows + i * sizeof row, sizeof row);
        QCOMPARE(row[0], 1);
        QCOMPARE(row[1], found[i].hold ? 1 : 0);
        QCOMPARE(row[4], Placements::leftColumn(found[i].placement));
        QCOMPARE(row[5], Placements::bottomRow(found[i].placement));
        for (int c = 0; c < Board::kCellCount; ++c) {
            const bool filled = found[i].afterstate.at(c) != PieceType::None;
            QCOMPARE(int(bytes[size_t(boards) + i * Board::kCellCount + size_t(c)]), filled ? 1 : 0);
        }
    }
    qint32 unused[9];
    std::memcpy(unused, bytes.data() + rows + found.size() * sizeof unused, sizeof unused);
    QCOMPARE(unused[0], 0);
}

void EnvTests::aPlacementLandsThePieceAndRunsToTheNext() {
    OmatrisEnv env = makeEnv();
    const Landing landing = env.landings().front();
    const OmaGames::EnvStep step = env.step(0);
    QVERIFY(!step.terminated);
    QVERIFY(step.ticks > 0);
    QCOMPARE(step.signalValues[2], 1.0);  // one piece placed
    QCOMPARE(step.reward, double(env.game().score()));
    for (QPoint cell : landing.placement.cells())
        QCOMPARE(env.game().board().at(cell), landing.placement.type);
    QVERIFY(env.game().hasPiece());
    QVERIFY(!env.landings().empty());
}

void EnvTests::dropMasksWhatCannotBeReached() {
    OmatrisEnv env = makeEnv({{QStringLiteral("actions"), QStringLiteral("drop")}});
    const std::vector<quint8> bytes = mask(env);
    for (int hold = 0; hold < 2; ++hold) {
        for (int rotation = 0; rotation < Piece::kStates; ++rotation) {
            for (int column = 0; column < Board::kWidth; ++column) {
                const bool reachable = Placements::drop(env.game(), hold, rotation, column).has_value();
                QCOMPARE(bool(bytes[size_t(OmatrisEnv::dropAction(hold, rotation, column))]), reachable);
            }
        }
    }
    const auto expected = Placements::drop(env.game(), false, 0, 0);
    QVERIFY(expected);
    env.step(OmatrisEnv::dropAction(false, 0, 0));
    for (QPoint cell : expected->placement.cells())
        QCOMPARE(env.game().board().at(cell), expected->placement.type);
}

void EnvTests::rawInputsMoveThePiece() {
    OmatrisEnv env = makeEnv({{QStringLiteral("actions"), QStringLiteral("raw")}});
    const int x = env.game().piece().origin.x();
    const OmaGames::EnvStep moved = env.step(OmatrisEnv::Left);
    QCOMPARE(env.game().piece().origin.x(), x - 1);
    QCOMPARE(moved.ticks, qint64(1));
    QCOMPARE(mask(env)[OmatrisEnv::Hold], quint8(1));
    env.step(OmatrisEnv::Hold);
    QCOMPARE(mask(env)[OmatrisEnv::Hold], quint8(0));  // once per piece
    const OmaGames::EnvStep dropped = env.step(OmatrisEnv::HardDrop);
    QCOMPARE(dropped.signalValues[2], 1.0);
    QVERIFY(dropped.reward > 0);  // two points a row for the hard drop

    OmatrisEnv sonic = makeEnv({{QStringLiteral("actions"), QStringLiteral("raw")}});
    sonic.step(OmatrisEnv::SoftDrop);
    QCOMPARE(sonic.game().piece().origin, sonic.game().ghost().origin);
    QVERIFY(sonic.game().hasPiece());  // on the floor, not locked
}

void EnvTests::frameSkipLetsTimePass() {
    OmatrisEnv env = makeEnv({{QStringLiteral("actions"), QStringLiteral("raw")}, {QStringLiteral("frame_skip"), 30}});
    const int row = env.game().piece().origin.y();
    QCOMPARE(env.step(OmatrisEnv::None).ticks, qint64(30));
    QCOMPARE(env.step(OmatrisEnv::None).ticks, qint64(30));
    QCOMPARE(env.game().ticks(), 60);
    QCOMPARE(env.game().piece().origin.y(), row + 1);  // a second of level 1 gravity
}

void EnvTests::placingTakesTimeAtAnInputRate() {
    // The same far landing, placed by an infinitely fast player and by one
    // pressing ten keys a second: the second spends six ticks a press on it.
    auto farthest = [](const OmatrisEnv &env) {
        const std::vector<Landing> &landings = env.landings();
        return int(std::max_element(landings.begin(), landings.end(), [](const Landing &a, const Landing &b) {
                       return a.calls.size() < b.calls.size();
                   }) - landings.begin());
    };
    OmatrisEnv instant = makeEnv();
    OmatrisEnv human = makeEnv({{QStringLiteral("input_rate"), 10}});
    const qint64 instantTicks = instant.step(farthest(instant)).ticks;
    const qint64 humanTicks = human.step(farthest(human)).ticks;
    QVERIFY2(humanTicks >= instantTicks + 6 * 3, qPrintable(QStringLiteral("%1 vs %2").arg(humanTicks).arg(instantTicks)));
    QVERIFY(human.game().phase() == Phase::Playing);
}

void EnvTests::aRunEndsWhenItTopsOut() {
    OmatrisEnv env = makeEnv({{QStringLiteral("hold"), false}});
    // Always the highest landing there is: the stack reaches the top fast.
    OmaGames::EnvStep step;
    for (int guard = 0; guard < 500 && !step.terminated; ++guard) {
        const std::vector<Landing> &landings = env.landings();
        const auto highest = std::min_element(landings.begin(), landings.end(), [](const Landing &a, const Landing &b) {
            return Placements::bottomRow(a.placement) < Placements::bottomRow(b.placement);
        });
        step = env.step(int(highest - landings.begin()));
    }
    QVERIFY(step.terminated);
    QCOMPARE(step.signalValues[7], 1.0);
    QCOMPARE(env.game().phase(), Phase::GameOver);
    QCOMPARE(legal(env), 0);
}

void EnvTests::theModeIsTheOneConfigured() {
    QCOMPARE(makeEnv({{QStringLiteral("mode"), QStringLiteral("sprint")}}).game().lineGoal(), Rules::kSprintLines);
    OmatrisEnv challenge = makeEnv({{QStringLiteral("mode"), QStringLiteral("challenge")}});
    QVERIFY(challenge.game().dealtRows() > 0);
    // Its info says how hard the deal is, as the app's Difficulty does.
    const QJsonObject info = challenge.info();
    QCOMPARE(info.value(QStringLiteral("dealt_difficulty")).toInt(), challenge.game().dealtDifficulty());
    QVERIFY(info.value(QStringLiteral("dealt_difficulty")).toInt() >= 1);
    QCOMPARE(info.value(QStringLiteral("difficulty")).toInt(), challenge.game().difficulty());
    QCOMPARE(makeEnv().game().mode(), Mode::Marathon);
    QVERIFY(!makeEnv().info().contains(QStringLiteral("dealt_difficulty")));
}

void EnvTests::theSameSeedAndActionsPlayTheSameGame() {
    for (const QString &space : {QStringLiteral("placement"), QStringLiteral("drop"), QStringLiteral("raw")}) {
        OmatrisEnv a = makeEnv({{QStringLiteral("actions"), space}});
        OmatrisEnv b = makeEnv({{QStringLiteral("actions"), space}});
        QRandomGenerator rng(3);
        for (int i = 0; i < 150 && a.game().phase() == Phase::Playing; ++i) {
            const int action = pick(a, rng);
            a.step(action);
            b.step(action);
            QVERIFY2(observe(a) == observe(b), qPrintable(space));
        }
    }
}

void EnvTests::theReplayIsTheGame() {
    for (const QString &space : {QStringLiteral("placement"), QStringLiteral("raw")}) {
        OmatrisEnv env = makeEnv({{QStringLiteral("actions"), space}, {QStringLiteral("mode"), QStringLiteral("zen")}});
        QRandomGenerator rng(5);
        for (int i = 0; i < 200 && env.game().phase() == Phase::Playing; ++i)
            env.step(pick(env, rng));
        QString error;
        const auto replay = OmaGames::Replay::fromJson(env.replay().toJson(), &error);
        QVERIFY2(replay, qPrintable(error));
        QCOMPARE(replay->game(), QStringLiteral("omatris"));
        QCOMPARE(replay->rulesVersion(), OmatrisEnv::kRulesVersion);
        const Game played = playBack(*replay);
        QVERIFY(sameBoard(played, env.game()));
        QCOMPARE(played.score(), env.game().score());
        QCOMPARE(played.ticks(), env.game().ticks());
        QCOMPARE(played.piece().cells(), env.game().piece().cells());
    }
}

void EnvTests::aReplayIsDrawnPieceByPiece() {
    // Forty placements of a Challenge at a human's speed, drawn for a viewer.
    OmatrisEnv env = makeEnv({{QStringLiteral("mode"), QStringLiteral("challenge")}, {QStringLiteral("input_rate"), 10}});
    QRandomGenerator rng(5);
    int placed = 0;
    for (; placed < 40 && env.game().phase() == Phase::Playing; ++placed)
        env.step(pick(env, rng));
    QString error;
    const auto drawn = env.frames(env.replay(), &error);
    QVERIFY2(drawn, qPrintable(error));
    const QJsonArray frames = drawn->value(QStringLiteral("frames")).toArray();
    QCOMPARE(frames.size(), placed + 1);  // the deal, then one per piece
    QVERIFY(!frames.first().toObject().contains(QStringLiteral("placed")));
    QCOMPARE(frames.last().toObject().value(QStringLiteral("placed")).toArray().size(), 4);

    // The last frame is the game as the env left it.
    const QJsonObject last = frames.last().toObject();
    const QString board = last.value(QStringLiteral("board")).toString();
    QCOMPARE(board.size(), Board::kCellCount);
    for (int i = 0; i < Board::kCellCount; ++i)
        QCOMPARE(board.at(i) == QLatin1Char('.'), env.game().board().at(i) == PieceType::None);
    QCOMPARE(last.value(QStringLiteral("score")).toInt(), env.game().score());
    QCOMPARE(last.value(QStringLiteral("lines")).toInt(), env.game().lines());
    QCOMPARE(last.value(QStringLiteral("dealt_rows_left")).toInt(), env.game().dealtRowsLeft());
    QCOMPARE(drawn->value(QStringLiteral("mode")).toString(), QStringLiteral("challenge"));

    OmaGames::Replay other(QStringLiteral("omasnake"), 1, {}, 1);
    QVERIFY(!env.frames(other, &error));
    QVERIFY(error.contains(QStringLiteral("omasnake")));
}

void EnvTests::aReseededCloneKeepsWhatIsVisible() {
    OmatrisEnv env = makeEnv();
    env.step(0);
    bool futureMoved = false;
    for (quint32 seed = 1; seed <= 5; ++seed) {
        std::unique_ptr<OmaGames::Env> copy = env.clone(true, seed);
        auto *clone = static_cast<OmatrisEnv *>(copy.get());
        QVERIFY(observe(*clone) == observe(env));
        QVERIFY(clone->replay().game().isEmpty());  // the seed no longer tells its future
        OmatrisEnv original = env;
        for (int i = 0; i < 6; ++i) {
            clone->step(0);
            original.step(0);
        }
        futureMoved = futureMoved || clone->game().nextQueue() != original.game().nextQueue();
    }
    QVERIFY(futureMoved);
    std::unique_ptr<OmaGames::Env> exact = env.clone(false, 0);
    QCOMPARE(static_cast<OmatrisEnv *>(exact.get())->replay().calls(), env.replay().calls());
}

void EnvTests::theEnvLeavesSettingsAlone() {
    BridgeFixture::clearSettings();
    OmatrisEnv env = makeEnv({{QStringLiteral("hold"), false}});
    for (int i = 0; i < 300 && env.game().phase() == Phase::Playing; ++i)
        env.step(0);
    QVERIFY(env.game().phase() != Phase::Playing);  // a whole run, finished
    QVERIFY(QSettings().allKeys().isEmpty());
}

void EnvTests::theAbiPlaysOmatris() {
    const QJsonObject spec = QJsonDocument::fromJson(og_game_spec()).object();
    QCOMPARE(spec.value(QStringLiteral("game")).toString(), QStringLiteral("omatris"));
    QVERIFY(spec.value(QStringLiteral("config")).toObject().contains(QStringLiteral("candidates")));
    OgEnv *env = og_create(R"({"actions": "raw", "mode": "zen"})");
    QVERIFY(env);
    og_reset(env, kSeed);
    OgStepResult result {};
    QCOMPARE(og_step(env, OmatrisEnv::HardDrop, &result), 0);
    QCOMPARE(result.signal_values[2], 1.0);
    QVERIFY(!og_create(R"({"actions": "keys"})"));
    og_destroy(env);
}
