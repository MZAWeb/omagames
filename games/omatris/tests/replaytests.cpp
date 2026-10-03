#include "replaytests.h"

#include <QJsonDocument>
#include <QRandomGenerator>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include "bridgefixture.h"
#include "envconfig.h"
#include "omatrisenv.h"
#include "replayplayer.h"

using namespace BridgeFixture;

namespace {

const QString kSample = QStringLiteral(OMATRIS_REPLAYS "/greedy-marathon.json");

OmatrisEnv makeEnv(const QJsonObject &config) {
    QString error;
    const auto resolved = OmaGames::EnvConfig::resolve(
        OmaGames::envGameSpec().value(QStringLiteral("config")).toObject(), config, &error);
    OmatrisEnv env;
    env.configure(*resolved);
    env.reset(kSeed);
    return env;
}

// An agent's game: random legal actions, `steps` of them or until it ends.
OmatrisEnv played(const QString &space, int steps) {
    OmatrisEnv env = makeEnv({{QStringLiteral("actions"), space}});
    QRandomGenerator rng(11);
    std::vector<quint8> mask(size_t(env.actionCount()));
    for (int i = 0; i < steps && env.game().phase() == Phase::Playing; ++i) {
        env.actionMask(mask.data());
        std::vector<int> legal;
        for (int a = 0; a < int(mask.size()); ++a) {
            if (mask[size_t(a)])
                legal.push_back(a);
        }
        env.step(legal[rng.bounded(int(legal.size()))]);
    }
    return env;
}

// A game that stacks every piece as high as it can, so it tops out.
OmatrisEnv toppedOut() {
    OmatrisEnv env = makeEnv({{QStringLiteral("hold"), false}});
    while (env.game().phase() == Phase::Playing) {
        const std::vector<Landing> &landings = env.landings();
        const auto highest = std::min_element(landings.begin(), landings.end(), [](const Landing &a, const Landing &b) {
            return Placements::bottomRow(a.placement) < Placements::bottomRow(b.placement);
        });
        env.step(int(highest - landings.begin()));
    }
    return env;
}

QString write(const QJsonObject &json) {
    static QTemporaryDir dir;
    static int count = 0;
    const QString path = dir.filePath(QStringLiteral("replay-%1.json").arg(++count));
    QFile file(path);
    // A file that could not be written comes back empty, and the load that
    // follows fails on it rather than on whatever was there before.
    if (!file.open(QIODevice::WriteOnly))
        return {};
    file.write(QJsonDocument(json).toJson());
    return path;
}

QString refusal(const QJsonObject &json) {
    QString error;
    return ReplayPlayer::load(json, &error) ? QString() : error;
}

int beats(ReplayPlayer &player) {
    Game game = player.deal();
    int count = 0;
    while (!player.done()) {
        player.beat(game);
        ++count;
    }
    return count;
}

ReplayPlayer fromCalls(const QString &calls) {
    QJsonObject json = OmaGames::Replay(QStringLiteral("omatris"), Rules::kVersion,
                                        {{QStringLiteral("soft_drop_factor"), 0}}, kSeed)
                           .toJson();
    json.insert(QStringLiteral("calls"), calls);
    QString error;
    return *ReplayPlayer::load(json, &error);
}

// Steps the bridge's pacer by hand until the replay has nothing left.
void watchToTheEnd(OmatrisGame &game) {
    for (int guard = 0; guard < 100000 && !game.replayEnded() && game.phase() == kPlaying; ++guard)
        game.step();
}

}  // namespace

void ReplayTests::initTestCase() {
    QVERIFY(!redirectSettings().isEmpty());
}

void ReplayTests::init() {
    clearSettings();
}

void ReplayTests::aReplayPlaysBackTheGameItRecorded() {
    for (const QString &space : {QStringLiteral("placement"), QStringLiteral("raw")}) {
        const OmatrisEnv env = played(space, 120);
        QString error;
        auto player = ReplayPlayer::load(env.replay().toJson(), &error);
        QVERIFY2(player, qPrintable(error));
        QCOMPARE(player->mode(), Mode::Marathon);
        Game game = player->deal();
        while (!player->done())
            player->beat(game);
        QCOMPARE(game.score(), env.game().score());
        QCOMPARE(game.ticks(), env.game().ticks());
        QCOMPARE(game.piece().cells(), env.game().piece().cells());
        for (int i = 0; i < Board::kCellCount; ++i)
            QCOMPARE(game.board().at(i), env.game().board().at(i));
    }
}

void ReplayTests::replaysOfAnotherGameOrRulesAreRefused() {
    const QJsonObject good = played(QStringLiteral("placement"), 3).replay().toJson();
    QVERIFY(refusal(good).isEmpty());
    auto with = [&good](const QString &key, const QJsonValue &value) {
        QJsonObject json = good;
        json.insert(key, value);
        return json;
    };
    QVERIFY(refusal(with(QStringLiteral("game"), QStringLiteral("omasnake"))).contains(QStringLiteral("omasnake")));
    QVERIFY(refusal(with(QStringLiteral("rules_version"), Rules::kVersion + 1)).contains(QStringLiteral("version")));
    QVERIFY(refusal(with(QStringLiteral("calls"), QStringLiteral("L t1 SPIN"))).contains(QStringLiteral("SPIN")));
    QJsonObject config = good.value(QStringLiteral("config")).toObject();
    config.insert(QStringLiteral("mode"), QStringLiteral("blitz"));
    QVERIFY(refusal(with(QStringLiteral("config"), config)).contains(QStringLiteral("blitz")));
    QVERIFY(!refusal(with(QStringLiteral("format"), QStringLiteral("save/v1"))).isEmpty());
}

void ReplayTests::beatsShowEveryMoveOfAPlacement() {
    // A placing agent moves with no time between inputs: each move gets a
    // beat, the sonic drop's tick one, the hard drop one, and the clear
    // flash a beat a tick. The first move rides with the previous frame.
    ReplayPlayer placing = fromCalls(QStringLiteral("L L CW SD+ t1 SD- HD t9"));
    QCOMPARE(beats(placing), 2 + 1 + 1 + 9);
    // An agent pressing keys in real time: each input rides with its tick.
    ReplayPlayer pressing = fromCalls(QStringLiteral("L t1 R t1 t1"));
    QCOMPARE(beats(pressing), 3);
}

void ReplayTests::nextPieceStopsAtTheLock() {
    const OmatrisEnv env = played(QStringLiteral("placement"), 10);
    QString error;
    auto player = ReplayPlayer::load(env.replay().toJson(), &error);
    Game game = player->deal();
    for (int piece = 0; piece < 10; ++piece) {
        const std::vector<Event> events = player->nextPiece(game);
        QCOMPARE(std::count_if(events.begin(), events.end(), [](const Event &e) { return e.type == Event::Locked; }),
                 1L);
    }
}

void ReplayTests::theSampleReplayPlaysToItsEnd() {
    OmatrisGame game;
    game.setStepInterval(0);
    QString error;
    QVERIFY2(game.loadReplay(kSample, &error), qPrintable(error));
    QVERIFY(game.replaying());
    QCOMPARE(game.replayAgent(), QStringLiteral("greedy heuristic"));
    QCOMPARE(game.mode(), QStringLiteral("marathon"));
    QCOMPARE(game.phase(), kPlaying);
    watchToTheEnd(game);
    QVERIFY(game.replayEnded());
    // What the heuristic scored when it was recorded: a rule change that
    // alters it means the sample needs recording again.
    QCOMPARE(game.score(), 101325);
    QCOMPARE(game.lines(), 117);
    QCOMPARE(game.phase(), kPlaying);  // cut short at 300 pieces, still going
}

void ReplayTests::theKeysDoNotMoveAReplay() {
    OmatrisGame game;
    game.setStepInterval(0);
    QString error;
    QVERIFY(game.loadReplay(kSample, &error));
    const QPoint origin = game.engine()->piece().origin;
    game.pressLeft();
    game.releaseLeft();
    game.rotateCw();
    game.swapHold();
    game.hardDrop();
    QCOMPARE(game.engine()->piece().origin, origin);
    QCOMPARE(game.engine()->piece().rotation, 0);
    QCOMPARE(game.score(), 0);
    QVERIFY(game.holdAvailable());
}

void ReplayTests::speedsPauseAndStepping() {
    OmatrisGame game;
    game.setStepInterval(0);
    QString error;
    QVERIFY(game.loadReplay(kSample, &error));
    QCOMPARE(game.replaySpeedLabel(), QStringLiteral("½×"));
    game.setReplaySpeed(4);
    QCOMPARE(game.replaySpeedLabel(), QStringLiteral("8×"));
    game.setReplaySpeed(9);
    QCOMPARE(game.replaySpeed(), 4);
    game.setReplaySpeed(1);  // a beat every fourth frame
    const int ticks = game.engine()->ticks();
    for (int i = 0; i < 40; ++i)
        game.step();
    QVERIFY(game.engine()->ticks() > ticks);

    game.pause();
    QVERIFY(game.paused());
    const int paused = game.engine()->ticks();
    game.step();
    QCOMPARE(game.engine()->ticks(), paused);
    QSignalSpy locked(&game, &OmatrisGame::pieceLocked);
    game.replayNextPiece();
    QCOMPARE(locked.count(), 1);
    QVERIFY(game.paused());
}

void ReplayTests::popupsOnlyAtSpeedsThatCanReadThem() {
    OmatrisGame game;
    game.setStepInterval(0);
    QString error;
    QVERIFY(game.loadReplay(kSample, &error));
    QSignalSpy bonuses(&game, &OmatrisGame::bonusEarned);
    game.setReplaySpeed(4);
    for (int i = 0; i < 40; ++i)
        game.replayNextPiece();
    QCOMPARE(bonuses.count(), 0);
    game.setReplaySpeed(3);
    for (int i = 0; i < 80; ++i)
        game.replayNextPiece();
    QVERIFY(bonuses.count() > 0);
}

void ReplayTests::watchAgainAndLeave() {
    OmatrisGame game;
    game.setStepInterval(0);
    QString error;
    QVERIFY(game.loadReplay(kSample, &error));
    const PieceType first = game.engine()->piece().type;
    for (int i = 0; i < 5; ++i)
        game.replayNextPiece();
    QVERIFY(game.score() > 0);
    game.restart();  // R is "watch again" in a replay
    QVERIFY(game.replaying());
    QCOMPARE(game.score(), 0);
    QCOMPARE(game.engine()->piece().type, first);

    game.backToStart();
    QVERIFY(!game.replaying());
    QCOMPARE(game.phase(), QStringLiteral("start"));
    game.newGame(QStringLiteral("zen"));
    QVERIFY(!game.replaying());
    const int x = game.engine()->piece().origin.x();
    game.pressLeft();
    QCOMPARE(game.engine()->piece().origin.x(), x - 1);  // the player's again
}

void ReplayTests::aReplayIsNeverAHighScore() {
    OmatrisGame game;
    game.setStepInterval(0);
    QString error;
    QVERIFY(game.loadReplay(write(toppedOut().replay().toJson()), &error));
    watchToTheEnd(game);
    QCOMPARE(game.phase(), QStringLiteral("gameover"));
    QVERIFY(game.score() > 0);
    QCOMPARE(game.newHighScoreRank(), -1);
    QVERIFY(game.highScores().isEmpty());
}

void ReplayTests::aReplayLeavesTheChosenModeAlone() {
    OmatrisGame game;
    game.setStepInterval(0);
    game.newGame(QStringLiteral("sprint"));
    game.backToStart();
    QString error;
    QVERIFY(game.loadReplay(kSample, &error));
    QCOMPARE(game.mode(), QStringLiteral("marathon"));
    QCOMPARE(game.lineGoal(), 0);
    game.backToStart();
    QCOMPARE(game.mode(), QStringLiteral("sprint"));
}

void ReplayTests::aFileThatIsNotAReplayDoesNotLoad() {
    OmatrisGame game;
    QString error;
    QVERIFY(!game.loadReplay(QStringLiteral("/nonexistent/replay.json"), &error));
    QVERIFY(error.contains(QStringLiteral("cannot read")));
    QVERIFY(!game.loadReplay(write({{QStringLiteral("format"), QStringLiteral("replay/v1")}}), &error));
    QVERIFY(!game.replaying());
    QCOMPARE(game.phase(), QStringLiteral("start"));
}
