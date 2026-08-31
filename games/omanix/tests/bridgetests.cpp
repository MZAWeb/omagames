#include "bridgetests.h"

#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include "omanixgame.h"

namespace {

constexpr quint32 kSeed = 20260830u;
constexpr int kInteriorHeight = Field::kDefaultHeight - 2 * Field::kBorder;

// A game with no chasers and the ball walled off on the right, like the
// engine suite's quiet scenario, but reached through the bridge.
void quietStart(OmanixGame &game, int wallX = 40) {
    game.setStepInterval(0);
    game.startGame(Difficulty::Normal, kSeed);
    Game *engine = game.engineForTests();
    engine->placeChasers({});
    Field &field = engine->mutableField();
    for (int y = Field::kBorder; y < field.height() - Field::kBorder; ++y)
        field.set({wallX, y}, Cell::Claimed);
    engine->placeBalls({{{wallX + 2, Field::kBorder}, {1, 1}}});
    engine->placePlayer({field.width() / 2, field.height() - 1});
    for (int i = 0; i < Game::kLevelIntroTicks; ++i)
        game.step();
}

void hold(OmanixGame &game, const QString &direction, int cells) {
    game.setDirection(direction);
    for (int i = 0; i < cells * game.engine()->params().playerPeriod; ++i)
        game.step();
    game.releaseDirection(direction);
}

}  // namespace

void BridgeTests::initTestCase() {
    static QTemporaryDir dir;
    QVERIFY(dir.isValid());
    m_settingsDir = dir.path();
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir);
}

void BridgeTests::init() {
    QSettings settings;
    settings.clear();
    settings.sync();
}

void BridgeTests::startsOnTheStartScreenWithDifficulties() {
    OmanixGame game;
    QCOMPARE(game.phase(), QStringLiteral("start"));
    QCOMPARE(game.difficulties().size(), 3);
    QCOMPARE(game.difficulties().at(0).toMap().value(QStringLiteral("id")).toString(), QStringLiteral("easy"));
    QCOMPARE(game.difficulties().at(2).toMap().value(QStringLiteral("label")).toString(), QStringLiteral("Hard"));
    QVERIFY(!game.difficulties().at(1).toMap().value(QStringLiteral("description")).toString().isEmpty());
    QCOMPARE(game.difficulty(), QStringLiteral("normal"));
    QCOMPARE(game.lives(), 0);
    QCOMPARE(game.fieldWidth(), 64);
    QCOMPARE(game.fieldHeight(), 40);
    QCOMPARE(game.goalPercent(), Game::kGoalPercent);
    QVERIFY(!game.engine());
    QVERIFY(game.highScores().isEmpty());
    QCOMPARE(game.bests().value(QStringLiteral("hard")).toInt(), 0);
}

void BridgeTests::newGameExposesEngineState() {
    OmanixGame game;
    game.setStepInterval(0);
    QSignalSpy phases(&game, &OmanixGame::phaseChanged);
    game.newGame(QStringLiteral("hard"));
    QCOMPARE(phases.count(), 1);
    QCOMPARE(game.phase(), QStringLiteral("playing"));
    QCOMPARE(game.difficulty(), QStringLiteral("hard"));
    QCOMPARE(game.difficultyLabel(), QStringLiteral("Hard"));
    QCOMPARE(game.level(), 1);
    QCOMPARE(game.score(), 0);
    QCOMPARE(game.lives(), Game::kStartLives);
    QCOMPARE(game.claimedPercent(), 0.0);
    QCOMPARE(int(game.engine()->balls().size()), 4);
    QCOMPARE(game.newHighScoreRank(), -1);

    // An unknown id falls back to the current difficulty.
    game.newGame(QStringLiteral("nightmare"));
    QCOMPARE(game.difficulty(), QStringLiteral("hard"));

    game.backToStart();
    QCOMPARE(game.phase(), QStringLiteral("start"));
    QVERIFY(!game.engine());
}

void BridgeTests::directionsAndPauseGoThroughTheBridge() {
    OmanixGame game;
    quietStart(game);
    const QPoint start = game.engine()->player().pos;
    QSignalSpy frames(&game, &OmanixGame::frameChanged);
    hold(game, QStringLiteral("left"), 2);
    QCOMPARE(game.engine()->player().pos, start + QPoint(-2, 0));
    // Only the two ticks that moved the marker redrew; the ticks in between
    // changed nothing and stayed silent (the fenced ball bounces in place).
    QVERIFY(frames.count() >= 2);
    QVERIFY(frames.count() < 2 * game.engine()->params().playerPeriod);

    game.setDirection(QStringLiteral("up"));
    for (int i = 0; i < 3 * game.engine()->params().playerPeriod; ++i)
        game.step();
    QVERIFY(game.engine()->player().onTrail);
    QSignalSpy paused(&game, &OmanixGame::pausedChanged);
    game.togglePause();
    QVERIFY(game.paused());
    QCOMPARE(paused.count(), 1);
    const QPoint frozen = game.engine()->player().pos;
    for (int i = 0; i < 20; ++i)
        game.step();
    QCOMPARE(game.engine()->player().pos, frozen);
    game.resume();
    QVERIFY(!game.paused());
    for (int i = 0; i < game.engine()->params().playerPeriod; ++i)
        game.step();
    QCOMPARE(game.engine()->player().pos, frozen + QPoint(0, -1));

    // Restarting the level drops the trail and the pause, keeps the score.
    game.pause();
    game.restartLevel();
    QVERIFY(!game.paused());
    QVERIFY(!game.engine()->player().onTrail);
    QCOMPARE(game.level(), 1);
}

void BridgeTests::levelIntroAndThreatAreExposed() {
    OmanixGame game;
    game.setStepInterval(0);
    QSignalSpy intros(&game, &OmanixGame::levelIntroChanged);
    game.newGame(QStringLiteral("hard"));
    QVERIFY(game.levelIntro());
    QCOMPARE(game.ballCount(), 4);
    QCOMPARE(game.chaserCount(), 3);
    QCOMPARE(intros.count(), 1);
    for (int i = 0; i < Game::kLevelIntroTicks; ++i)
        game.step();
    QVERIFY(!game.levelIntro());
    QCOMPARE(intros.count(), 2);

    OmanixGame quiet;
    quietStart(quiet);
    QSignalSpy threats(&quiet, &OmanixGame::trailThreatenedChanged);
    const QPoint start = quiet.engine()->player().pos;
    quiet.engineForTests()->placeBalls({{{start.x() + Game::kCloseCallDistance, start.y() - 6}, {1, -1}}});
    Field &field = quiet.engineForTests()->mutableField();
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx != 0 || dy != 0)
                field.set({start.x() + Game::kCloseCallDistance + dx, start.y() - 6 + dy}, Cell::Claimed);
        }
    }
    hold(quiet, QStringLiteral("up"), 2);
    QVERIFY(!quiet.trailThreatened());
    hold(quiet, QStringLiteral("up"), 1);
    QVERIFY(quiet.trailThreatened());
    QCOMPARE(threats.count(), 1);
    hold(quiet, QStringLiteral("left"), 1);
    hold(quiet, QStringLiteral("down"), 3);
    QVERIFY(!quiet.trailThreatened());
    QCOMPARE(threats.count(), 2);
}

void BridgeTests::scriptedLevelCompletesAndContinues() {
    OmanixGame game;
    quietStart(game, 56);
    QSignalSpy claims(&game, &OmanixGame::cellsClaimed);
    QSignalSpy bonuses(&game, &OmanixGame::bonusEarned);
    QSignalSpy scores(&game, &OmanixGame::scoreChanged);
    QSignalSpy percents(&game, &OmanixGame::claimedPercentChanged);
    hold(game, QStringLiteral("right"), 10);
    hold(game, QStringLiteral("up"), Field::kDefaultHeight);
    QCOMPARE(game.phase(), QStringLiteral("levelcomplete"));
    QVERIFY(game.claimedPercent() >= Game::kGoalPercent);
    QCOMPARE(claims.count(), 1);
    QCOMPARE(claims.first().at(0).value<QVector<int>>().size(), 55 * kInteriorHeight);
    QCOMPARE(bonuses.count(), 1);
    QVERIFY(bonuses.first().at(0).toString().startsWith(QStringLiteral("Big cut")));
    QCOMPARE(scores.count(), 1);
    QCOMPARE(percents.count(), 1);
    const QVariantMap stats = game.levelStats();
    QCOMPARE(stats.value(QStringLiteral("bonus")).toInt(), Game::kStartLives * Game::kLifeBonus);
    QVERIFY(stats.value(QStringLiteral("percent")).toDouble() >= Game::kGoalPercent);
    QCOMPARE(stats.value(QStringLiteral("seconds")).toInt(), game.levelSeconds());
    QCOMPARE(game.score(), 55 * kInteriorHeight * Game::kHugeCutMultiplier + Game::kStartLives * Game::kLifeBonus);

    game.nextLevel();
    QCOMPARE(game.phase(), QStringLiteral("playing"));
    QCOMPARE(game.level(), 2);
    QCOMPARE(game.claimedPercent(), 0.0);
    QCOMPARE(game.levelSeconds(), 0);
    QCOMPARE(int(game.engine()->balls().size()), Level::params(Difficulty::Normal, 2).balls);
}

void BridgeTests::scriptedGameOverRecordsAHighScore() {
    OmanixGame game;
    quietStart(game);
    hold(game, QStringLiteral("left"), 5);
    hold(game, QStringLiteral("up"), Field::kDefaultHeight);
    const int score = game.score();
    QVERIFY(score > 0);
    QCOMPARE(game.phase(), QStringLiteral("playing"));

    // Over to the open side, then loop into the own trail three times. The
    // cut left the marker on the top edge; every respawn puts it on the
    // bottom one, so the loop is drawn away from whichever edge it is on.
    QSignalSpy losses(&game, &OmanixGame::trailLost);
    QSignalSpy lives(&game, &OmanixGame::livesChanged);
    QSignalSpy tables(&game, &OmanixGame::highScoresChanged);
    for (int life = 0; life < Game::kStartLives; ++life) {
        QCOMPARE(game.phase(), QStringLiteral("playing"));
        const bool onTop = game.engine()->player().pos.y() < Field::kDefaultHeight / 2;
        const QString inward = onTop ? QStringLiteral("down") : QStringLiteral("up");
        const QString outward = onTop ? QStringLiteral("up") : QStringLiteral("down");
        hold(game, QStringLiteral("right"), 45 - game.engine()->player().pos.x());
        hold(game, inward, 6);
        hold(game, QStringLiteral("left"), 2);
        hold(game, outward, 2);
        hold(game, QStringLiteral("right"), 2);
        for (int i = 0; i < Game::kRespawnTicks; ++i)
            game.step();
    }
    QCOMPARE(losses.count(), Game::kStartLives);
    QCOMPARE(lives.count(), Game::kStartLives);
    QCOMPARE(game.phase(), QStringLiteral("gameover"));
    QCOMPARE(game.lives(), 0);
    QCOMPARE(game.newHighScoreRank(), 0);
    QCOMPARE(tables.count(), 1);
    QCOMPARE(game.bests().value(QStringLiteral("normal")).toInt(), score);
    QCOMPARE(game.highScores().size(), 1);
    const QVariantMap entry = game.highScores().first().toMap();
    QCOMPARE(entry.value(QStringLiteral("score")).toInt(), score);
    QCOMPARE(entry.value(QStringLiteral("level")).toInt(), 1);
    QCOMPARE(entry.value(QStringLiteral("difficulty")).toString(), QStringLiteral("normal"));
    QCOMPARE(entry.value(QStringLiteral("date")).toString(), QDate::currentDate().toString(Qt::ISODate));
    // Nothing moves after the end.
    game.step();
    QCOMPARE(game.phase(), QStringLiteral("gameover"));
}

// Ordering, the cap and the ranks are ScoreTable's own tests now. What is
// Omanix's is that the table it wrote before the move still reads back: the
// JSON below is the literal output of the old HighScores::toJson(), which is
// what is sitting in ~/.config/Omacom/omanix.conf on a machine that has been
// playing.
void BridgeTests::savedHighScoresReachQml() {
    QSettings settings;
    settings.setValue(QStringLiteral("scores/v1"),
                      QStringLiteral(R"({"easy":[{"date":"2026-08-29","level":7,"score":4500},)"
                                     R"({"date":"2026-08-30","level":3,"score":1200}],"hard":[],)"
                                     R"("normal":[{"date":"2026-07-01","level":2,"score":800}]})"));
    settings.sync();

    OmanixGame game;
    const QVariantList rows = game.highScores();
    QCOMPARE(rows.size(), 3);
    const QVariantMap first = rows.first().toMap();
    QCOMPARE(first.value(QStringLiteral("difficulty")).toString(), QStringLiteral("easy"));
    QCOMPARE(first.value(QStringLiteral("label")).toString(), QStringLiteral("Easy"));
    QCOMPARE(first.value(QStringLiteral("score")).toInt(), 4500);
    QCOMPARE(first.value(QStringLiteral("level")).toInt(), 7);
    QCOMPARE(first.value(QStringLiteral("date")).toString(), QStringLiteral("2026-08-29"));
    QCOMPARE(rows.at(1).toMap().value(QStringLiteral("score")).toInt(), 1200);
    // The tables come out in the order the start screen shows them.
    QCOMPARE(rows.at(2).toMap().value(QStringLiteral("difficulty")).toString(), QStringLiteral("normal"));

    QCOMPARE(game.bests().value(QStringLiteral("easy")).toInt(), 4500);
    QCOMPARE(game.bests().value(QStringLiteral("normal")).toInt(), 800);
    QCOMPARE(game.bests().value(QStringLiteral("hard")).toInt(), 0);
}

void BridgeTests::lastDifficultyIsRemembered() {
    {
        OmanixGame game;
        game.setStepInterval(0);
        game.newGame(QStringLiteral("easy"));
        game.backToStart();
    }
    OmanixGame game;
    QCOMPARE(game.difficulty(), QStringLiteral("easy"));
    QCOMPARE(game.difficultyLabel(), QStringLiteral("Easy"));
}

void BridgeTests::windowGeometryRoundTrips() {
    OmanixGame game;
    QVERIFY(!game.windowGeometry().value(QStringLiteral("valid")).toBool());
    game.saveWindowGeometry(-20, 30, 1000, 700, true);
    const QVariantMap geometry = game.windowGeometry();
    QVERIFY(geometry.value(QStringLiteral("valid")).toBool());
    QCOMPARE(geometry.value(QStringLiteral("x")).toInt(), -20);
    QCOMPARE(geometry.value(QStringLiteral("width")).toInt(), 1000);
    QVERIFY(geometry.value(QStringLiteral("maximized")).toBool());
}

// The whole point of Game::frame(): a tick on which nothing moved emits no
// frameChanged, so the view is not rasterised 60 times a second for a still
// picture.
void BridgeTests::silentTicksDoNotWakeTheRenderer() {
    OmanixGame game;
    quietStart(game);
    game.engineForTests()->placeBalls({});
    game.engineForTests()->placeChasers({});
    QSignalSpy frames(&game, &OmanixGame::frameChanged);
    for (int i = 0; i < 60; ++i)
        game.step();
    QCOMPARE(frames.count(), 0);

    game.setDirection(QStringLiteral("left"));
    for (int i = 0; i < game.engine()->params().playerPeriod; ++i)
        game.step();
    QCOMPARE(frames.count(), 1);
}
