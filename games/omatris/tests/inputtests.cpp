#include "inputtests.h"

#include <QSignalSpy>
#include <QtTest>

#include "bridgefixture.h"

using namespace BridgeFixture;

void InputTests::initTestCase() {
    QVERIFY(!redirectSettings().isEmpty());
}

void InputTests::init() {
    clearSettings();
}

void InputTests::startsOnTheStartScreenWithModes() {
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

void InputTests::newGameExposesEngineState() {
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

void InputTests::autoShiftWaitsThenRepeats() {
    OmatrisGame game;
    quietStart(game, Mode::Zen);
    const Game *engine = game.engine();
    const int start = engine->piece().origin.x();

    // The key press itself moves one cell straight away.
    game.pressLeft();
    QCOMPARE(engine->piece().origin.x(), start - 1);
    // Nothing more until the delay is up.
    for (int i = 0; i < AutoShift::kDelayTicks - 1; ++i)
        game.step();
    QCOMPARE(engine->piece().origin.x(), start - 1);
    game.step();
    QCOMPARE(engine->piece().origin.x(), start - 2);
    // Then one cell every repeat interval.
    for (int i = 0; i < AutoShift::kRepeatTicks; ++i)
        game.step();
    QCOMPARE(engine->piece().origin.x(), start - 3);

    // Letting go stops it; the other key takes over from scratch.
    game.releaseLeft();
    for (int i = 0; i < 2 * AutoShift::kDelayTicks; ++i)
        game.step();
    QCOMPARE(engine->piece().origin.x(), start - 3);
    game.pressRight();
    QCOMPARE(engine->piece().origin.x(), start - 2);
    for (int i = 0; i < AutoShift::kDelayTicks - 1; ++i)
        game.step();
    QCOMPARE(engine->piece().origin.x(), start - 2);
    game.step();
    QCOMPARE(engine->piece().origin.x(), start - 1);
    game.releaseRight();
}

void InputTests::rotationHoldAndDropsGoThroughTheBridge() {
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

void InputTests::pauseFreezesEverything() {
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

void InputTests::pieceShapesFeedTheBoxes() {
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
