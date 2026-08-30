#include "persistencetests.h"

#include <QSignalSpy>
#include <QtTest>
#include <algorithm>

#include "bridgefixture.h"

using namespace BridgeFixture;

namespace {

void fillRow(Board &board, int y, std::initializer_list<int> gaps) {
    for (int x = 0; x < Board::kWidth; ++x) {
        if (std::find(gaps.begin(), gaps.end(), x) == gaps.end())
            board.set({x, y}, PieceType::L);
    }
}

// Four rows open only at column 0, wiped by an I stood on end in that column,
// then held still while the clear flashes.
void tetris(OmatrisGame &game) {
    Game *engine = game.engineForTests();
    for (int y = Board::kHeight - 4; y < Board::kHeight; ++y)
        fillRow(engine->mutableBoard(), y, {0});
    engine->placePiece({PieceType::I, 1, {-2, 0}});
    game.hardDrop();
    for (int i = 0; i < Rules::kClearDelayTicks; ++i)
        game.step();
}

// Dropped straight down, every piece stacks over the middle columns: the
// outer ones never fill, so the pile reaches the ceiling.
void stackUntilItTopsOut(OmatrisGame &game) {
    int guard = 0;
    while (game.phase() == kPlaying && ++guard < 2000) {
        if (game.engine()->hasPiece())
            game.hardDrop();
        else
            game.step();
    }
}

QStringList bonusTexts(const QSignalSpy &spy) {
    QStringList texts;
    for (const QList<QVariant> &shot : spy)
        texts << shot.at(0).toString();
    return texts;
}

}  // namespace

void PersistenceTests::initTestCase() {
    QVERIFY(!redirectSettings().isEmpty());
}

void PersistenceTests::init() {
    clearSettings();
}

void PersistenceTests::scriptedMarathonTopsOutAndRecordsAScore() {
    OmatrisGame game;
    quietStart(game, Mode::Marathon);
    QSignalSpy tables(&game, &OmatrisGame::highScoresChanged);
    QSignalSpy phases(&game, &OmatrisGame::phaseChanged);

    stackUntilItTopsOut(game);
    QCOMPARE(game.phase(), QStringLiteral("gameover"));
    QVERIFY(game.score() > 0);
    QCOMPARE(game.newHighScoreRank(), 0);
    QCOMPARE(phases.count(), 1);
    QCOMPARE(tables.count(), 1);
    QCOMPARE(game.highScores().size(), 1);
    const QVariantMap entry = game.highScores().first().toMap();
    QCOMPARE(entry.value(QStringLiteral("mode")).toString(), QStringLiteral("marathon"));
    QCOMPARE(entry.value(QStringLiteral("score")).toInt(), game.score());
    QCOMPARE(entry.value(QStringLiteral("date")).toString(), QDate::currentDate().toString(Qt::ISODate));
    QCOMPARE(game.bests().value(QStringLiteral("marathon")).toInt(), game.score());
    // Nothing moves after the end.
    const int score = game.score();
    game.hardDrop();
    game.step();
    QCOMPARE(game.score(), score);
}

void PersistenceTests::scriptedSprintRecordsItsTime() {
    OmatrisGame game;
    quietStart(game, Mode::Sprint);
    QSignalSpy cleared(&game, &OmatrisGame::linesCleared);
    QSignalSpy bonuses(&game, &OmatrisGame::bonusEarned);
    QSignalSpy levels(&game, &OmatrisGame::levelReached);

    for (int i = 0; i < Rules::kSprintLines / 4; ++i)
        tetris(game);

    QCOMPARE(game.phase(), QStringLiteral("finished"));
    QCOMPARE(game.lines(), Rules::kSprintLines);
    QCOMPARE(game.linesLeft(), 0);
    QCOMPARE(cleared.count(), Rules::kSprintLines / 4);
    QCOMPARE(levels.count(), 4);
    QVERIFY(bonusTexts(bonuses).contains(QStringLiteral("Tetris")));
    QVERIFY(bonusTexts(bonuses).contains(QStringLiteral("Back-to-Back")));
    QVERIFY(game.elapsedMs() > 0);
    QCOMPARE(game.newHighScoreRank(), 0);
    const QVariantMap entry = game.highScores().first().toMap();
    QCOMPARE(entry.value(QStringLiteral("mode")).toString(), QStringLiteral("sprint"));
    QCOMPARE(entry.value(QStringLiteral("millis")).toInt(), game.elapsedMs());
    QCOMPARE(entry.value(QStringLiteral("lines")).toInt(), Rules::kSprintLines);
    QCOMPARE(game.bests().value(QStringLiteral("sprint")).toInt(), game.elapsedMs());

    // A Sprint that tops out has no time worth keeping.
    OmatrisGame lost;
    quietStart(lost, Mode::Sprint);
    stackUntilItTopsOut(lost);
    QCOMPARE(lost.phase(), QStringLiteral("gameover"));
    QCOMPARE(lost.newHighScoreRank(), -1);
    QCOMPARE(lost.highScores().size(), 1);
}

// Ordering, the cap and the ranks are ScoreTable's own tests now. What is
// Omatris's is that Sprint ranks on the clock while Marathon and Zen rank on
// the score, that every run keeps all four numbers whichever mode it was, and
// that the JSON it wrote before the move still reads back: the string below
// is the literal output of the old HighScores::toJson().
void PersistenceTests::savedHighScoresReachQml() {
    QSettings settings;
    settings.setValue(
        QStringLiteral("scores/v1"),
        QStringLiteral(R"({"marathon":[{"date":"2026-08-30","level":7,"lines":62,"millis":305000,"score":15400},)"
                       R"({"date":"2026-08-20","level":2,"lines":12,"millis":61000,"score":2200}],)"
                       R"("sprint":[{"date":"2026-08-26","level":4,"lines":40,"millis":78040,"score":2900},)"
                       R"({"date":"2026-08-25","level":5,"lines":40,"millis":92310,"score":3100}],)"
                       R"("zen":[{"date":"2026-05-02","level":1,"lines":8,"millis":44000,"score":640}]})"));
    settings.sync();

    OmatrisGame game;
    const QVariantList rows = game.highScores();
    QCOMPARE(rows.size(), 5);
    const QVariantMap first = rows.first().toMap();
    QCOMPARE(first.value(QStringLiteral("mode")).toString(), QStringLiteral("marathon"));
    QCOMPARE(first.value(QStringLiteral("label")).toString(), QStringLiteral("Marathon"));
    QCOMPARE(first.value(QStringLiteral("score")).toInt(), 15400);
    QCOMPARE(first.value(QStringLiteral("lines")).toInt(), 62);
    QCOMPARE(first.value(QStringLiteral("level")).toInt(), 7);
    QCOMPARE(first.value(QStringLiteral("millis")).toInt(), 305000);
    QCOMPARE(first.value(QStringLiteral("date")).toString(), QStringLiteral("2026-08-30"));

    // Sprint is ranked on the clock, so the run with the lower score but the
    // faster time comes first, and it still carries its score.
    const QVariantMap sprint = rows.at(2).toMap();
    QCOMPARE(sprint.value(QStringLiteral("mode")).toString(), QStringLiteral("sprint"));
    QCOMPARE(sprint.value(QStringLiteral("millis")).toInt(), 78040);
    QCOMPARE(sprint.value(QStringLiteral("score")).toInt(), 2900);
    QCOMPARE(rows.at(3).toMap().value(QStringLiteral("millis")).toInt(), 92310);

    QCOMPARE(game.bests().value(QStringLiteral("marathon")).toInt(), 15400);
    QCOMPARE(game.bests().value(QStringLiteral("sprint")).toInt(), 78040);
    QCOMPARE(game.bests().value(QStringLiteral("zen")).toInt(), 640);
}

void PersistenceTests::lastModeIsRemembered() {
    {
        OmatrisGame game;
        game.setStepInterval(0);
        game.newGame(QStringLiteral("zen"));
        game.backToStart();
    }
    OmatrisGame game;
    QCOMPARE(game.mode(), QStringLiteral("zen"));
    QCOMPARE(game.modeLabel(), QStringLiteral("Zen"));
    QCOMPARE(game.lineGoal(), 0);
}

void PersistenceTests::ghostToggleIsRemembered() {
    {
        OmatrisGame game;
        QSignalSpy spy(&game, &OmatrisGame::ghostEnabledChanged);
        QVERIFY(game.ghostEnabled());
        game.toggleGhost();
        QVERIFY(!game.ghostEnabled());
        QCOMPARE(spy.count(), 1);
    }
    {
        OmatrisGame game;
        QVERIFY(!game.ghostEnabled());
        game.toggleGhost();
        QVERIFY(game.ghostEnabled());
    }
    OmatrisGame game;
    QVERIFY(game.ghostEnabled());
}

void PersistenceTests::windowGeometryRoundTrips() {
    OmatrisGame game;
    QVERIFY(!game.windowGeometry().value(QStringLiteral("valid")).toBool());
    game.saveWindowGeometry(-20, 30, 900, 760, true);
    const QVariantMap geometry = game.windowGeometry();
    QVERIFY(geometry.value(QStringLiteral("valid")).toBool());
    QCOMPARE(geometry.value(QStringLiteral("x")).toInt(), -20);
    QCOMPARE(geometry.value(QStringLiteral("height")).toInt(), 760);
    QVERIFY(geometry.value(QStringLiteral("maximized")).toBool());
}
