#include "bridgetests.h"

#include <QSignalSpy>
#include <QtTest>

#include "scenario.h"
#include "solver.h"

using namespace Scenario;

void BridgeTests::initTestCase() {
    QVERIFY(Scenario::redirectSettings());
}

void BridgeTests::init() {
    Scenario::clearSettings();
}

void BridgeTests::startsOnTheStartScreenWithPresets() {
    OmasweeperGame game;
    QCOMPARE(game.phase(), kStart);
    QCOMPARE(game.status(), QStringLiteral("ready"));
    QVERIFY(game.board() == nullptr);

    const QVariantList presets = OmasweeperGame::presets();
    QCOMPARE(presets.size(), 3);
    QCOMPARE(presets.at(0).toMap().value(QStringLiteral("id")).toString(), QStringLiteral("beginner"));
    QCOMPARE(presets.at(2).toMap().value(QStringLiteral("label")).toString(), QStringLiteral("Expert"));
    QCOMPARE(presets.at(2).toMap().value(QStringLiteral("width")).toInt(), 30);
    QCOMPARE(presets.at(2).toMap().value(QStringLiteral("mines")).toInt(), 99);
    // Never played: the counter still shows the preset's mines.
    QCOMPARE(game.remainingMines(), 10);
}

void BridgeTests::newGameExposesThePreset() {
    OmasweeperGame game;
    game.setStepInterval(0);
    game.newGame(QStringLiteral("intermediate"));
    QCOMPARE(game.preset(), QStringLiteral("intermediate"));
    QCOMPARE(game.presetLabel(), QStringLiteral("Intermediate"));
    QCOMPARE(game.width(), 16);
    QCOMPARE(game.height(), 16);
    QCOMPARE(game.mines(), 40);
    QCOMPARE(game.phase(), kPlaying);
    QCOMPARE(game.status(), QStringLiteral("ready"));
    QCOMPARE(game.remainingMines(), 40);
    QCOMPARE(game.flags(), 0);
    QCOMPARE(game.elapsedSeconds(), 0);
    QVERIFY(game.noGuess());
    // The cursor starts in the middle, and no mines exist until a reveal.
    QCOMPARE(game.cursorX(), 8);
    QCOMPARE(game.cursorY(), 8);
    QVERIFY(!game.board()->minesPlaced());
}

void BridgeTests::generatorGivingUpClearsTheNoGuessFlag() {
    OmasweeperGame game;
    game.setStepInterval(0);
    // With a single attempt allowed the generator soon meets a layout it
    // cannot prove, and says so instead of pretending.
    game.setMaxAttemptsForTests(1);
    bool gaveUp = false;
    for (quint32 seed = 1; seed <= 20 && !gaveUp; ++seed) {
        game.startGame(Preset::Expert, seed);
        QVERIFY(game.noGuess());
        openFirst(game);
        gaveUp = !game.noGuess();
    }
    QVERIFY(gaveUp);
    // The board is still playable, it just carries no promise.
    QCOMPARE(game.phase(), kPlaying);
    QVERIFY(game.board()->revealedCount() >= 9);
}

void BridgeTests::restartKeepsThePresetAndDrawsANewBoard() {
    OmasweeperGame game;
    started(game, Preset::Intermediate);
    openFirst(game);
    for (int i = 0; i < 5; ++i)
        game.step();
    const std::vector<int> mines = game.board()->mines();

    game.restart();
    QCOMPARE(game.preset(), QStringLiteral("intermediate"));
    QCOMPARE(game.phase(), kPlaying);
    QCOMPARE(game.status(), QStringLiteral("ready"));
    QCOMPARE(game.elapsedSeconds(), 0);
    QCOMPARE(game.flags(), 0);
    QCOMPARE(game.board()->revealedCount(), 0);
    QVERIFY(!game.board()->minesPlaced());
    openFirst(game);
    QVERIFY(game.board()->mines() != mines);
}

void BridgeTests::backToStartClearsTheBoard() {
    OmasweeperGame game;
    started(game, Preset::Beginner);
    openFirst(game);
    game.backToStart();
    QCOMPARE(game.phase(), kStart);
    QVERIFY(game.board() == nullptr);
    // Actions on no board are harmless.
    game.revealAtCursor();
    game.flagAtCursor();
    game.chordAtCursor();
    game.moveCursor(1, 1);
    game.step();
    QCOMPARE(game.phase(), kStart);
    QCOMPARE(game.elapsedSeconds(), 0);
}

// Ordering, the cap and the ranks are ScoreTable's own tests now. What is
// Omasweeper's is that the tables are keyed by preset and that the JSON it
// wrote before the move still reads back: the string below is the literal
// output of the old BestTimes::toJson().
void BridgeTests::savedBestTimesReachQml() {
    QSettings settings;
    settings.setValue(
        QStringLiteral("scores/v1"),
        QStringLiteral(R"({"beginner":[{"date":"2026-08-27","seconds":22},)"
                       R"({"date":"2026-08-30","seconds":41}],)"
                       R"("expert":[{"date":"2026-03-09","seconds":602}],"intermediate":[]})"));
    settings.sync();

    OmasweeperGame game;
    const QVariantList rows = game.bestTimes();
    QCOMPARE(rows.size(), 3);
    const QVariantMap first = rows.first().toMap();
    QCOMPARE(first.value(QStringLiteral("preset")).toString(), QStringLiteral("beginner"));
    QCOMPARE(first.value(QStringLiteral("label")).toString(), QStringLiteral("Beginner"));
    QCOMPARE(first.value(QStringLiteral("seconds")).toInt(), 22);
    QCOMPARE(first.value(QStringLiteral("date")).toString(), QStringLiteral("2026-08-27"));
    QCOMPARE(rows.at(1).toMap().value(QStringLiteral("seconds")).toInt(), 41);
    QCOMPARE(rows.at(2).toMap().value(QStringLiteral("preset")).toString(), QStringLiteral("expert"));

    QCOMPARE(game.bests().value(QStringLiteral("beginner")).toInt(), 22);
    QCOMPARE(game.bests().value(QStringLiteral("expert")).toInt(), 602);
    QCOMPARE(game.bests().value(QStringLiteral("intermediate")).toInt(), 0);
}

void BridgeTests::lastPresetIsRemembered() {
    {
        OmasweeperGame game;
        game.setStepInterval(0);
        game.newGame(QStringLiteral("expert"));
        QCOMPARE(game.preset(), QStringLiteral("expert"));
    }
    OmasweeperGame reopened;
    QCOMPARE(reopened.preset(), QStringLiteral("expert"));
    QCOMPARE(reopened.presetLabel(), QStringLiteral("Expert"));
    // An unknown id keeps the remembered preset.
    reopened.setStepInterval(0);
    reopened.newGame(QStringLiteral("nonsense"));
    QCOMPARE(reopened.preset(), QStringLiteral("expert"));
}

void BridgeTests::windowGeometryRoundTrips() {
    OmasweeperGame game;
    QVERIFY(!game.windowGeometry().value(QStringLiteral("valid")).toBool());
    game.saveWindowGeometry(40, 50, 900, 700, true);
    const QVariantMap geometry = game.windowGeometry();
    QVERIFY(geometry.value(QStringLiteral("valid")).toBool());
    QCOMPARE(geometry.value(QStringLiteral("x")).toInt(), 40);
    QCOMPARE(geometry.value(QStringLiteral("width")).toInt(), 900);
    QVERIFY(geometry.value(QStringLiteral("maximized")).toBool());
}
