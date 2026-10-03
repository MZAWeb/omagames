#include "replaytests.h"

#include <QJsonDocument>
#include <QRandomGenerator>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include "choices.h"
#include "envconfig.h"
#include "omasnakeenv.h"
#include "omasnakegame.h"
#include "replayplayer.h"

namespace {

constexpr quint32 kSeed = 20260830u;
const QString kSample = QStringLiteral(OMASNAKE_REPLAYS "/greedy-classic.json");

// An agent's game in Wrap, so random turns last a while: `moves` of them or
// until it ends.
OmasnakeEnv played(int moves) {
    QString error;
    const auto config = OmaGames::EnvConfig::resolve(
        OmaGames::envGameSpec().value(QStringLiteral("config")).toObject(),
        {{QStringLiteral("mode"), QStringLiteral("wrap")}}, &error);
    OmasnakeEnv env;
    env.configure(*config);
    env.reset(kSeed);
    QRandomGenerator rng(8);
    for (int i = 0; i < moves && env.game().phase() == Phase::Playing; ++i)
        env.step(int(rng.bounded(OmasnakeEnv::kRelativeActions)));
    return env;
}

QString refusal(const QJsonObject &json) {
    QString error;
    return ReplayPlayer::load(json, &error) ? QString() : error;
}

QString write(const QJsonObject &json) {
    static QTemporaryDir dir;
    static int count = 0;
    const QString path = dir.filePath(QStringLiteral("replay-%1.json").arg(++count));
    QFile file(path);
    file.open(QIODevice::WriteOnly);
    file.write(QJsonDocument(json).toJson());
    return path;
}

OmasnakeGame *watching(const QString &path) {
    auto *game = new OmasnakeGame;
    game->setStepInterval(0);
    QString error;
    if (!game->loadReplay(path, &error))
        qWarning("%s", qPrintable(error));
    return game;
}

}  // namespace

void ReplayTests::initTestCase() {
    static QTemporaryDir dir;
    QVERIFY(dir.isValid());
    m_settingsDir = dir.path();
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir);
}

void ReplayTests::init() {
    QSettings().clear();
}

void ReplayTests::aReplayPlaysBackTheGameItRecorded() {
    const OmasnakeEnv env = played(400);
    QString error;
    auto player = ReplayPlayer::load(env.replay().toJson(), &error);
    QVERIFY2(player, qPrintable(error));
    QCOMPARE(player->mode(), Mode::Wrap);
    QCOMPARE(player->difficulty(), Difficulty::Normal);
    Game game = player->deal();
    while (!player->done())
        player->tick(game);
    QCOMPARE(game.score(), env.game().score());
    QCOMPARE(game.snake().body(), env.game().snake().body());
    QCOMPARE(game.food(), env.game().food());
    QCOMPARE(game.phase(), env.game().phase());
}

void ReplayTests::replaysOfAnotherGameOrRulesAreRefused() {
    const QJsonObject good = played(5).replay().toJson();
    QVERIFY(refusal(good).isEmpty());
    auto with = [&good](const QString &key, const QJsonValue &value) {
        QJsonObject json = good;
        json.insert(key, value);
        return json;
    };
    QVERIFY(refusal(with(QStringLiteral("game"), QStringLiteral("omatris"))).contains(QStringLiteral("omatris")));
    QVERIFY(refusal(with(QStringLiteral("rules_version"), Rules::kVersion + 1)).contains(QStringLiteral("version")));
    QVERIFY(refusal(with(QStringLiteral("calls"), QStringLiteral("t3 X t1"))).contains(QStringLiteral("\"X\"")));
    QJsonObject config = good.value(QStringLiteral("config")).toObject();
    config.insert(QStringLiteral("difficulty"), QStringLiteral("ludicrous"));
    QVERIFY(refusal(with(QStringLiteral("config"), config)).contains(QStringLiteral("ludicrous")));
}

void ReplayTests::nextMoveMovesTheSnakeOnce() {
    const OmasnakeEnv env = played(30);
    QString error;
    auto player = ReplayPlayer::load(env.replay().toJson(), &error);
    Game game = player->deal();
    for (int move = 0; move < 30 && !player->done(); ++move) {
        const QPoint before = game.snake().head();
        player->nextMove(game);
        const QPoint step = game.snake().head() - before;
        // One cell, or across the field when it wrapped.
        QVERIFY(step.manhattanLength() == 1 || step.manhattanLength() == Game::kWidth - 1
                || step.manhattanLength() == Game::kHeight - 1);
    }
}

void ReplayTests::theSampleReplayPlaysToItsEnd() {
    std::unique_ptr<OmasnakeGame> game(watching(kSample));
    QVERIFY(game->replaying());
    QCOMPARE(game->replayAgent(), QStringLiteral("greedy toward the food"));
    QCOMPARE(game->mode(), QStringLiteral("classic"));
    QCOMPARE(game->difficulty(), QStringLiteral("normal"));
    QCOMPARE(game->replaySpeedLabel(), QStringLiteral("1×"));
    for (int guard = 0; guard < 100000 && game->phase() == QStringLiteral("playing"); ++guard)
        game->step();
    // What the greedy player reached when it was recorded: a rule change
    // that alters it means the sample needs recording again.
    QCOMPARE(game->phase(), QStringLiteral("gameover"));
    QCOMPARE(game->gameOverReason(), QStringLiteral("self"));
    QCOMPARE(game->score(), 3090);
    QCOMPARE(game->length(), 81);
    QCOMPARE(game->newHighScoreRank(), -1);
    QVERIFY(game->highScores().isEmpty());
}

void ReplayTests::theKeysDoNotSteerAReplay() {
    // The same replay watched twice, keys pressed in only one: the two
    // games stay the same move for move.
    std::unique_ptr<OmasnakeGame> pressed(watching(kSample));
    std::unique_ptr<OmasnakeGame> untouched(watching(kSample));
    const QStringList keys {QStringLiteral("up"), QStringLiteral("left"), QStringLiteral("down")};
    for (int i = 0; i < 600; ++i) {
        pressed->turn(keys.at(i % keys.size()));
        pressed->step();
        untouched->step();
        QCOMPARE(pressed->engine()->snake().body(), untouched->engine()->snake().body());
    }
    QVERIFY(pressed->length() > Game::kStartLength);
}

void ReplayTests::speedsPauseAndStepping() {
    std::unique_ptr<OmasnakeGame> game(watching(kSample));
    game->setReplaySpeed(1);
    QCOMPARE(game->replaySpeedLabel(), QStringLiteral("¼×"));
    game->setReplaySpeed(7);
    QCOMPARE(game->replaySpeed(), 1);
    for (int i = 0; i < 4 * Game::kReadyTicks + 40; ++i)
        game->step();
    QVERIFY(!game->ready());

    game->pause();
    const QPoint head = game->engine()->snake().head();
    game->step();
    QCOMPARE(game->engine()->snake().head(), head);
    game->replayNextMove();
    QCOMPARE((game->engine()->snake().head() - head).manhattanLength(), 1);
    QVERIFY(game->paused());
}

void ReplayTests::popupsOnlyAtSpeedsThatCanReadThem() {
    std::unique_ptr<OmasnakeGame> game(watching(kSample));
    QSignalSpy popups(game.get(), &OmasnakeGame::scored);
    game->setReplaySpeed(4);
    for (int i = 0; i < 400; ++i)
        game->replayNextMove();
    QVERIFY(game->length() > 10);
    QCOMPARE(popups.count(), 0);
    game->setReplaySpeed(3);
    for (int i = 0; i < 400 && game->phase() == QStringLiteral("playing"); ++i)
        game->replayNextMove();
    QVERIFY(popups.count() > 0);
}

void ReplayTests::watchAgainAndLeave() {
    std::unique_ptr<OmasnakeGame> game(new OmasnakeGame);
    game->setStepInterval(0);
    game->setMode(QStringLiteral("wrap"));
    QString error;
    QVERIFY(game->loadReplay(kSample, &error));
    QCOMPARE(game->mode(), QStringLiteral("classic"));
    for (int i = 0; i < 200; ++i)
        game->replayNextMove();
    QVERIFY(game->score() > 0);
    game->restart();  // R is "watch again" in a replay
    QVERIFY(game->replaying());
    QCOMPARE(game->score(), 0);
    QCOMPARE(game->length(), Game::kStartLength);

    game->backToStart();
    QVERIFY(!game->replaying());
    QCOMPARE(game->mode(), QStringLiteral("wrap"));  // the player's choice, untouched
    game->newGame(QStringLiteral("normal"));
    for (int i = 0; i < Game::kReadyTicks; ++i)
        game->step();
    game->turn(QStringLiteral("up"));
    QCOMPARE(game->engine()->snake().queuedTurns(), 1);  // the player's again
}

void ReplayTests::aFileThatIsNotAReplayDoesNotLoad() {
    OmasnakeGame game;
    QString error;
    QVERIFY(!game.loadReplay(QStringLiteral("/nonexistent/replay.json"), &error));
    QVERIFY(error.contains(QStringLiteral("cannot read")));
    QVERIFY(!game.loadReplay(write({{QStringLiteral("format"), QStringLiteral("replay/v1")}}), &error));
    QVERIFY(!game.replaying());
    QCOMPARE(game.phase(), QStringLiteral("start"));
}
