#include "gametests.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include "gameprobe.h"
#include "savedgame.h"

using TestSupport::cellBool;
using TestSupport::cellInt;
using TestSupport::firstGiven;

void GameTests::initTestCase() {
    // Never touch the real ~/.config/Omacom while testing.
    static QTemporaryDir dir;
    QVERIFY(dir.isValid());
    m_settingsDir = dir.path();
    TestSupport::redirectSettings(m_settingsDir);
}

void GameTests::init() {
    QSettings settings;
    settings.clear();
    settings.sync();
}

void GameTests::startsOnTheStartScreen() {
    SudokuGame game;
    QCOMPARE(game.state(), QStringLiteral("start"));
    QVERIFY(!game.hasSavedGame());
    QVERIFY(!game.canUndo());
    QCOMPARE(game.cursorIndex(), -1);
    QVERIFY(!game.validateAsYouGo());  // default off
}

void GameTests::newGameSelectsTheFirstEmptyCell() {
    SudokuGame game;
    QSignalSpy stateSpy(&game, &SudokuGame::stateChanged);
    game.newGame(QStringLiteral("hard"));

    QCOMPARE(game.state(), QStringLiteral("playing"));
    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(game.difficulty(), QStringLiteral("hard"));
    QVERIFY(game.cursorIndex() >= 0);
    QCOMPARE(cellInt(&game, game.cursorIndex(), CellModel::ValueRole), 0);

    QVERIFY(game.filledCount() > 0);
    QCOMPARE(game.digitCounts().size(), 9);
}

void GameTests::digitsGoIntoTheSelectedCellOnly() {
    SudokuGame game;
    game.newGame(QStringLiteral("easy"));
    const int empty = game.cursorIndex();  // the first empty cell
    const int given = firstGiven(&game);
    const int givenValue = cellInt(&game, given, CellModel::ValueRole);

    game.select(given);
    game.enterValue(givenValue % 9 + 1);
    QCOMPARE(cellInt(&game, given, CellModel::ValueRole), givenValue);
    QVERIFY(!game.canUndo());

    game.select(empty);
    game.enterValue(5);
    QCOMPARE(cellInt(&game, empty, CellModel::ValueRole), 5);
    QVERIFY(game.canUndo());
}

void GameTests::undoRestartAndEraseGoThroughTheBoard() {
    SudokuGame game;
    game.newGame(QStringLiteral("easy"));
    const int cell = game.cursorIndex();
    const int filled = game.filledCount();

    game.enterValue(2);
    QCOMPARE(game.filledCount(), filled + 1);
    game.erase();
    QCOMPARE(game.filledCount(), filled);

    game.undo();  // undoes the erase
    QCOMPARE(cellInt(&game, cell, CellModel::ValueRole), 2);
    game.undo();  // undoes the entry
    QCOMPARE(cellInt(&game, cell, CellModel::ValueRole), 0);
    QVERIFY(!game.canUndo());

    game.enterValue(2);
    game.restart();
    QCOMPARE(game.filledCount(), filled);
    QVERIFY(!game.canUndo());
}

void GameTests::restartAsksBeforeWipingTheBoard() {
    SudokuGame game;
    game.newGame(QStringLiteral("easy"));
    const int filled = game.filledCount();

    // Nothing entered yet, nothing to lose: no question, no board left over.
    game.requestRestart();
    QVERIFY(!game.restartPending());

    game.enterValue(2);
    QSignalSpy spy(&game, &SudokuGame::restartPendingChanged);
    game.requestRestart();
    QVERIFY(game.restartPending());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(game.filledCount(), filled + 1);  // the question changes nothing

    game.cancelRestart();
    QVERIFY(!game.restartPending());
    QCOMPARE(game.filledCount(), filled + 1);  // and neither does saying no

    game.requestRestart();
    game.confirmRestart();
    QVERIFY(!game.restartPending());
    QCOMPARE(game.filledCount(), filled);
    QVERIFY(!game.canUndo());

    // Leaving the puzzle takes the question with it.
    game.enterValue(2);
    game.requestRestart();
    game.backToStart();
    QVERIFY(!game.restartPending());
}

void GameTests::restartIsNotExposedToQmlWithoutTheDialog() {
    // QML can only ask (requestRestart) and answer (confirm/cancel): restart()
    // itself is not invokable, so no .qml file can wipe a board on its own.
    const QMetaObject &meta = SudokuGame::staticMetaObject;
    QCOMPARE(meta.indexOfMethod("restart()"), -1);
    QVERIFY(meta.indexOfMethod("requestRestart()") >= 0);
    QVERIFY(meta.indexOfMethod("confirmRestart()") >= 0);
    QVERIFY(meta.indexOfMethod("cancelRestart()") >= 0);

    // And an answer with no question outstanding does nothing.
    SudokuGame game;
    game.newGame(QStringLiteral("easy"));
    const int filled = game.filledCount();
    game.enterValue(2);
    game.confirmRestart();
    QCOMPARE(game.filledCount(), filled + 1);
}

void GameTests::clockRunsOnlyWhilePlaying() {
    SudokuGame game;
    QCOMPARE(game.elapsedSeconds(), 0);
    game.newGame(QStringLiteral("easy"));
    QTRY_COMPARE_WITH_TIMEOUT(game.elapsedSeconds(), 1, 3000);

    game.backToStart();
    const int stopped = game.elapsedSeconds();
    QTest::qWait(1200);
    QCOMPARE(game.elapsedSeconds(), stopped);  // paused on the start screen
}

void GameTests::cursorValueFollowsTheCursor() {
    SudokuGame game;
    game.newGame(QStringLiteral("easy"));
    const int empty = game.cursorIndex();
    QCOMPARE(game.cursorValue(), 0);

    QSignalSpy spy(&game, &SudokuGame::cursorValueChanged);
    game.enterValue(6);
    QCOMPARE(game.cursorValue(), 6);
    QVERIFY(spy.count() > 0);

    game.select(firstGiven(&game));
    QCOMPARE(game.cursorValue(), cellInt(&game, game.cursorIndex(), CellModel::ValueRole));

    game.select(empty);
    QCOMPARE(game.cursorValue(), 6);
}

void GameTests::validateAsYouGoFlipsMidGame() {
    // A saved game built from a known seed, so the solution is at hand to
    // pick a digit that is certainly wrong.
    const SudokuBoard expected = TestSupport::installSavedGame(3, 20260829u);
    SudokuGame game;
    game.resumeSavedGame();
    const int cell = game.cursorIndex();
    QVERIFY(!game.validateAsYouGo());
    game.setValidateAsYouGo(true);

    game.enterValue(expected.puzzle().solution[size_t(cell)] % 9 + 1);
    QVERIFY(cellBool(&game, cell, CellModel::WrongRole));

    QSignalSpy spy(&game, &SudokuGame::validateAsYouGoChanged);
    game.setValidateAsYouGo(false);  // off: the mark vanishes until the grid is full
    QVERIFY(!cellBool(&game, cell, CellModel::WrongRole));
    game.setValidateAsYouGo(true);   // on: it is back at once, no new entry needed
    QVERIFY(cellBool(&game, cell, CellModel::WrongRole));
    QCOMPARE(spy.count(), 2);
}

void GameTests::untouchedPuzzleIsNotInProgress() {
    SudokuGame game;
    QVERIFY(!game.inProgress());
    game.newGame(QStringLiteral("easy"));
    QVERIFY(!game.inProgress());  // givens alone are not progress

    game.enterValue(1);
    QVERIFY(game.inProgress());
    game.undo();
    QVERIFY(!game.inProgress());

    game.backToStart();
    QVERIFY(!game.hasSavedGame());  // nothing worth resuming
}
