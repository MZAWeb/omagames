#include "bridgetests.h"

#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
#include <algorithm>

#include "omatrisgame.h"

namespace {

constexpr quint32 kSeed = 20260830u;

const auto kPlaying = QStringLiteral("playing");

void quietStart(OmatrisGame &game, Mode mode) {
    game.setStepInterval(0);
    game.startGame(mode, kSeed);
}

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

QStringList bonusTexts(const QSignalSpy &spy) {
    QStringList texts;
    for (const QList<QVariant> &shot : spy)
        texts << shot.at(0).toString();
    return texts;
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

void BridgeTests::startsOnTheStartScreenWithModes() {
    OmatrisGame game;
    QCOMPARE(game.phase(), QStringLiteral("start"));
    QCOMPARE(game.modes().size(), kModeCount);
    QCOMPARE(game.modes().at(0).toMap().value(QStringLiteral("id")).toString(), QStringLiteral("marathon"));
    QCOMPARE(game.modes().at(1).toMap().value(QStringLiteral("goal")).toInt(), Rules::kSprintLines);
    QVERIFY(!game.modes().at(2).toMap().value(QStringLiteral("description")).toString().isEmpty());
    QCOMPARE(game.mode(), QStringLiteral("marathon"));
    QCOMPARE(game.modeLabel(), QStringLiteral("Marathon"));
    QVERIFY(!game.rankByTime());
    QCOMPARE(game.boardWidth(), 10);
    QCOMPARE(game.boardHeight(), 20);
    QCOMPARE(game.score(), 0);
    QCOMPARE(game.holdPiece(), kPieceCount);
    QVERIFY(game.nextQueue().isEmpty());
    QVERIFY(!game.engine());
    QVERIFY(game.highScores().isEmpty());
    QCOMPARE(game.bests().value(QStringLiteral("sprint")).toInt(), 0);
}

void BridgeTests::newGameExposesEngineState() {
    OmatrisGame game;
    game.setStepInterval(0);
    QSignalSpy phases(&game, &OmatrisGame::phaseChanged);
    game.newGame(QStringLiteral("sprint"));
    QCOMPARE(phases.count(), 1);
    QCOMPARE(game.phase(), kPlaying);
    QCOMPARE(game.mode(), QStringLiteral("sprint"));
    QCOMPARE(game.modeLabel(), QStringLiteral("Sprint"));
    QVERIFY(game.rankByTime());
    QCOMPARE(game.lineGoal(), Rules::kSprintLines);
    QCOMPARE(game.linesLeft(), Rules::kSprintLines);
    QCOMPARE(game.level(), Rules::kFirstLevel);
    QCOMPARE(game.score(), 0);
    QCOMPARE(game.combo(), -1);
    QCOMPARE(game.holdPiece(), kPieceCount);
    QVERIFY(game.holdAvailable());
    QCOMPARE(game.nextQueue().size(), Rules::kNextQueue);
    QVERIFY(game.engine()->hasPiece());
    QCOMPARE(game.newHighScoreRank(), -1);

    // An unknown id falls back to the mode in play.
    game.newGame(QStringLiteral("battle"));
    QCOMPARE(game.mode(), QStringLiteral("sprint"));

    game.backToStart();
    QCOMPARE(game.phase(), QStringLiteral("start"));
    QVERIFY(!game.engine());
}

void BridgeTests::autoShiftWaitsThenRepeats() {
    OmatrisGame game;
    quietStart(game, Mode::Zen);
    const Game *engine = game.engine();
    const int start = engine->piece().origin.x();

    // The key press itself moves one cell straight away.
    game.pressLeft();
    QCOMPARE(engine->piece().origin.x(), start - 1);
    // Nothing more until the delay is up.
    for (int i = 0; i < OmatrisGame::kDasTicks - 1; ++i)
        game.step();
    QCOMPARE(engine->piece().origin.x(), start - 1);
    game.step();
    QCOMPARE(engine->piece().origin.x(), start - 2);
    // Then one cell every repeat interval.
    for (int i = 0; i < OmatrisGame::kArrTicks; ++i)
        game.step();
    QCOMPARE(engine->piece().origin.x(), start - 3);

    // Letting go stops it; the other key takes over from scratch.
    game.releaseLeft();
    for (int i = 0; i < 2 * OmatrisGame::kDasTicks; ++i)
        game.step();
    QCOMPARE(engine->piece().origin.x(), start - 3);
    game.pressRight();
    QCOMPARE(engine->piece().origin.x(), start - 2);
    for (int i = 0; i < OmatrisGame::kDasTicks - 1; ++i)
        game.step();
    QCOMPARE(engine->piece().origin.x(), start - 2);
    game.step();
    QCOMPARE(engine->piece().origin.x(), start - 1);
    game.releaseRight();
}

void BridgeTests::rotationHoldAndDropsGoThroughTheBridge() {
    OmatrisGame game;
    quietStart(game, Mode::Zen);
    const Game *engine = game.engine();
    QSignalSpy frames(&game, &OmatrisGame::frameChanged);
    QSignalSpy holds(&game, &OmatrisGame::holdChanged);
    QSignalSpy locks(&game, &OmatrisGame::pieceLocked);
    QSignalSpy scores(&game, &OmatrisGame::scoreChanged);

    game.rotateCw();
    QCOMPARE(engine->piece().rotation, 1);
    game.rotateCcw();
    game.rotateCcw();
    QCOMPARE(engine->piece().rotation, 3);

    const int first = int(engine->piece().type);
    game.swapHold();
    QCOMPARE(game.holdPiece(), first);
    QVERIFY(!game.holdAvailable());
    QCOMPARE(holds.count(), 1);
    // A held piece comes back upright.
    QCOMPARE(engine->piece().rotation, 0);

    game.setSoftDrop(true);
    for (int i = 0; i < 3; ++i)
        game.step();
    QCOMPARE(game.score(), Rules::kSoftDropPoints);
    game.setSoftDrop(false);

    game.hardDrop();
    QCOMPARE(locks.count(), 1);
    QCOMPARE(locks.first().at(0).value<QVector<int>>().size(), 4);
    QVERIFY(game.score() > Rules::kSoftDropPoints);
    QVERIFY(scores.count() >= 2);
    QVERIFY(game.holdAvailable());
    QVERIFY(frames.count() >= 6);
}

void BridgeTests::pauseFreezesEverything() {
    OmatrisGame game;
    quietStart(game, Mode::Marathon);
    const Game *engine = game.engine();
    QSignalSpy paused(&game, &OmatrisGame::pausedChanged);
    game.pressLeft();
    game.togglePause();
    QVERIFY(game.paused());
    QCOMPARE(paused.count(), 1);
    const QPoint frozen = engine->piece().origin;
    for (int i = 0; i < 4 * Rules::kTicksPerSecond; ++i)
        game.step();
    QCOMPARE(engine->piece().origin, frozen);
    // Keys are dead while it is paused, and the held key is forgotten.
    game.rotateCw();
    game.hardDrop();
    QCOMPARE(engine->piece().origin, frozen);
    game.resume();
    QVERIFY(!game.paused());
    for (int i = 0; i < Rules::kTicksPerSecond; ++i)
        game.step();
    QCOMPARE(engine->piece().origin, frozen + QPoint(0, 1));
}

void BridgeTests::scriptedMarathonTopsOutAndRecordsAScore() {
    OmatrisGame game;
    quietStart(game, Mode::Marathon);
    QSignalSpy tables(&game, &OmatrisGame::highScoresChanged);
    QSignalSpy phases(&game, &OmatrisGame::phaseChanged);

    // Dropped straight down, every piece stacks over the middle columns: the
    // outer ones never fill, so the pile reaches the ceiling.
    int guard = 0;
    while (game.phase() == kPlaying && ++guard < 2000) {
        if (game.engine()->hasPiece())
            game.hardDrop();
        else
            game.step();
    }
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

void BridgeTests::scriptedSprintRecordsItsTime() {
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
    int guard = 0;
    while (lost.phase() == kPlaying && ++guard < 2000) {
        if (lost.engine()->hasPiece())
            lost.hardDrop();
        else
            lost.step();
    }
    QCOMPARE(lost.phase(), QStringLiteral("gameover"));
    QCOMPARE(lost.newHighScoreRank(), -1);
    QCOMPARE(lost.highScores().size(), 1);
}

void BridgeTests::pieceShapesFeedTheBoxes() {
    OmatrisGame game;
    const QVariantMap bar = game.pieceShape(int(PieceType::I));
    QCOMPARE(bar.value(QStringLiteral("width")).toInt(), 4);
    QCOMPARE(bar.value(QStringLiteral("height")).toInt(), 1);
    QCOMPARE(bar.value(QStringLiteral("cells")).toList().size(), 4);
    QCOMPARE(bar.value(QStringLiteral("cells")).toList().first().toMap().value(QStringLiteral("x")).toInt(), 0);

    const QVariantMap block = game.pieceShape(int(PieceType::O));
    QCOMPARE(block.value(QStringLiteral("width")).toInt(), 2);
    QCOMPARE(block.value(QStringLiteral("height")).toInt(), 2);

    const QVariantMap tee = game.pieceShape(int(PieceType::T));
    QCOMPARE(tee.value(QStringLiteral("width")).toInt(), 3);
    QCOMPARE(tee.value(QStringLiteral("height")).toInt(), 2);

    // An empty hold box asks for a piece that is not one.
    const QVariantMap none = game.pieceShape(kPieceCount);
    QCOMPARE(none.value(QStringLiteral("width")).toInt(), 0);
    QVERIFY(none.value(QStringLiteral("cells")).toList().isEmpty());
}

void BridgeTests::highScoresRankByScoreOrByTime() {
    HighScores scores;
    const QDate day(2026, 8, 30);
    QCOMPARE(scores.insert(Mode::Marathon, {100, 4, 1, 9000, day}), 0);
    QCOMPARE(scores.insert(Mode::Marathon, {300, 12, 2, 9000, day}), 0);
    QCOMPARE(scores.insert(Mode::Marathon, {200, 8, 1, 9000, day}), 1);
    // A tie ranks below the older run.
    QCOMPARE(scores.insert(Mode::Marathon, {200, 9, 1, 9000, day}), 2);
    QCOMPARE(scores.entries(Mode::Marathon).at(2).lines, 9);
    QCOMPARE(scores.best(Mode::Marathon), 300);
    QVERIFY(scores.entries(Mode::Zen).empty());
    QCOMPARE(scores.best(Mode::Zen), 0);

    // Sprint is the other way round: the shortest clock wins.
    QCOMPARE(scores.insert(Mode::Sprint, {900, 40, 5, 62000, day}), 0);
    QCOMPARE(scores.insert(Mode::Sprint, {800, 40, 5, 48000, day}), 0);
    QCOMPARE(scores.insert(Mode::Sprint, {700, 40, 5, 55000, day}), 1);
    QCOMPARE(scores.best(Mode::Sprint), 48000);

    for (int i = 0; i < 20; ++i)
        scores.insert(Mode::Zen, {i * 10, 1, 1, 1000, day});
    QCOMPARE(int(scores.entries(Mode::Zen).size()), HighScores::kMaxEntries);
    QCOMPARE(scores.entries(Mode::Zen).front().score, 190);
    QCOMPARE(scores.entries(Mode::Zen).back().score, 100);
    QCOMPARE(scores.insert(Mode::Zen, {50, 1, 1, 1000, day}), -1);
    QCOMPARE(scores.insert(Mode::Zen, {150, 1, 1, 1000, day}), 5);
    QCOMPARE(scores.entries(Mode::Zen).back().score, 110);
}

void BridgeTests::highScoresRoundTripThroughSettings() {
    {
        HighScores scores;
        scores.insert(Mode::Marathon, {42000, 63, 7, 300000, QDate(2026, 8, 30)});
        scores.insert(Mode::Marathon, {9000, 21, 3, 90000, QDate(2026, 8, 29)});
        scores.insert(Mode::Sprint, {5400, 40, 5, 51230, QDate(2026, 8, 28)});
        scores.save();
    }
    HighScores loaded;
    loaded.load();
    QCOMPARE(int(loaded.entries(Mode::Marathon).size()), 2);
    QCOMPARE(loaded.entries(Mode::Marathon).front().score, 42000);
    QCOMPARE(loaded.entries(Mode::Marathon).front().level, 7);
    QCOMPARE(loaded.entries(Mode::Marathon).front().date, QDate(2026, 8, 30));
    QCOMPARE(loaded.best(Mode::Sprint), 51230);
    QVERIFY(loaded.entries(Mode::Zen).empty());

    OmatrisGame game;
    QCOMPARE(game.highScores().size(), 3);
    QCOMPARE(game.highScores().first().toMap().value(QStringLiteral("label")).toString(), QStringLiteral("Marathon"));
    QCOMPARE(game.bests().value(QStringLiteral("marathon")).toInt(), 42000);
    QCOMPARE(game.bests().value(QStringLiteral("sprint")).toInt(), 51230);
}

void BridgeTests::lastModeIsRemembered() {
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

void BridgeTests::ghostToggleIsRemembered() {
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

void BridgeTests::windowGeometryRoundTrips() {
    OmatrisGame game;
    QVERIFY(!game.windowGeometry().value(QStringLiteral("valid")).toBool());
    game.saveWindowGeometry(-20, 30, 900, 760, true);
    const QVariantMap geometry = game.windowGeometry();
    QVERIFY(geometry.value(QStringLiteral("valid")).toBool());
    QCOMPARE(geometry.value(QStringLiteral("x")).toInt(), -20);
    QCOMPARE(geometry.value(QStringLiteral("height")).toInt(), 760);
    QVERIFY(geometry.value(QStringLiteral("maximized")).toBool());
}
