#include "gametests.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

#include "cellmodel.h"
#include "sudoku.h"
#include "sudokugame.h"
#include "sudokugenerator.h"

namespace {

constexpr quint32 kSeed = 20260829u;

int cellInt(QAbstractListModel *model, int cell, CellModel::Role role) {
    return model->data(model->index(cell, 0), role).toInt();
}

bool cellBool(QAbstractListModel *model, int cell, CellModel::Role role) {
    return model->data(model->index(cell, 0), role).toBool();
}

int firstGiven(QAbstractListModel *model) {
    for (int i = 0; i < Sudoku::kCells; ++i) {
        if (cellBool(model, i, CellModel::GivenRole))
            return i;
    }
    return -1;
}

}  // namespace

void GameTests::initTestCase() {
    // Never touch the real ~/.config/Omacom while testing.
    static QTemporaryDir dir;
    QVERIFY(dir.isValid());
    m_settingsDir = dir.path();
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir);
}

void GameTests::init() {
    QSettings settings;
    settings.clear();
    settings.sync();
}

int GameTests::firstEmptyCell(const SudokuBoard &board) const {
    for (int i = 0; i < Sudoku::kCells; ++i) {
        if (board.value(i) == 0)
            return i;
    }
    return -1;
}

SudokuBoard GameTests::installSavedGame(int blanks) {
    SudokuBoard board;
    board.setPuzzle(SudokuGenerator::generate(Difficulty::Easy, kSeed));
    const Sudoku::Grid &solution = board.puzzle().solution;
    int left = blanks;
    for (int i = Sudoku::kCells - 1; i >= 0; --i) {
        if (board.isGiven(i))
            continue;
        if (left > 0) {
            --left;
            continue;  // leave this one empty
        }
        board.setValue(i, solution[size_t(i)]);
    }

    QJsonObject json = board.toJson();
    json.insert(QStringLiteral("elapsed"), 42);
    QSettings settings;
    settings.setValue(QStringLiteral("state/v1"),
                      QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Compact)));
    settings.sync();
    return board;
}

void GameTests::startsOnTheStartScreen() {
    SudokuGame game;
    QCOMPARE(game.state(), SudokuGame::Start);
    QVERIFY(!game.hasSavedGame());
    QVERIFY(!game.canUndo());
    QCOMPARE(game.selectedIndex(), -1);
    QVERIFY(game.checkAsYouGo());  // default on
}

void GameTests::newGameSelectsTheFirstEmptyCell() {
    SudokuGame game;
    QSignalSpy stateSpy(&game, &SudokuGame::stateChanged);
    game.newGame(SudokuGame::Hard);

    QCOMPARE(game.state(), SudokuGame::Playing);
    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(game.difficulty(), int(Difficulty::Hard));
    QVERIFY(game.selectedIndex() >= 0);
    QCOMPARE(cellInt(game.cells(), game.selectedIndex(), CellModel::ValueRole), 0);
    QVERIFY(!game.notesMode());

    QVERIFY(game.filledCount() >= SudokuGenerator::minClues(Difficulty::Hard));
    QCOMPARE(game.digitCounts().size(), 9);
}

void GameTests::selectionMovesWithinTheGrid() {
    SudokuGame game;
    game.newGame(SudokuGame::Easy);

    game.select(40);
    game.moveSelection(-1, 0);
    QCOMPARE(game.selectedIndex(), 31);
    game.moveSelection(0, 1);
    QCOMPARE(game.selectedIndex(), 32);

    game.select(0);
    game.moveSelection(-1, -1);  // clamped at the top-left corner
    QCOMPARE(game.selectedIndex(), 0);
    game.select(80);
    game.moveSelection(1, 1);
    QCOMPARE(game.selectedIndex(), 80);

    game.select(200);  // out of range is ignored
    QCOMPARE(game.selectedIndex(), 80);
}

void GameTests::digitsGoIntoTheSelectedCellOnly() {
    SudokuGame game;
    game.newGame(SudokuGame::Easy);
    const int empty = game.selectedIndex();  // the first empty cell
    const int given = firstGiven(game.cells());
    const int givenValue = cellInt(game.cells(), given, CellModel::ValueRole);

    game.select(given);
    game.enterDigit(givenValue % 9 + 1);
    QCOMPARE(cellInt(game.cells(), given, CellModel::ValueRole), givenValue);
    QVERIFY(!game.canUndo());

    game.select(empty);
    game.enterDigit(5);
    QCOMPARE(cellInt(game.cells(), empty, CellModel::ValueRole), 5);
    QVERIFY(game.canUndo());
}

void GameTests::notesModeWritesPencilMarks() {
    SudokuGame game;
    game.newGame(SudokuGame::Easy);
    const int cell = game.selectedIndex();

    game.toggleNotesMode();
    QVERIFY(game.notesMode());
    game.enterDigit(3);
    game.enterDigit(8);
    QCOMPARE(cellInt(game.cells(), cell, CellModel::NotesRole), (1 << 2) | (1 << 7));
    QCOMPARE(cellInt(game.cells(), cell, CellModel::ValueRole), 0);

    game.enterDigit(3);  // toggles back off
    QCOMPARE(cellInt(game.cells(), cell, CellModel::NotesRole), 1 << 7);

    game.toggleNotesMode();
    game.enterDigit(4);
    QCOMPARE(cellInt(game.cells(), cell, CellModel::ValueRole), 4);
    QCOMPARE(cellInt(game.cells(), cell, CellModel::NotesRole), 0);
}

void GameTests::undoRestartAndEraseGoThroughTheBoard() {
    SudokuGame game;
    game.newGame(SudokuGame::Easy);
    const int cell = game.selectedIndex();
    const int filled = game.filledCount();

    game.enterDigit(2);
    QCOMPARE(game.filledCount(), filled + 1);
    game.erase();
    QCOMPARE(game.filledCount(), filled);

    game.undo();  // undoes the erase
    QCOMPARE(cellInt(game.cells(), cell, CellModel::ValueRole), 2);
    game.undo();  // undoes the entry
    QCOMPARE(cellInt(game.cells(), cell, CellModel::ValueRole), 0);
    QVERIFY(!game.canUndo());

    game.enterDigit(2);
    game.restart();
    QCOMPARE(game.filledCount(), filled);
    QVERIFY(!game.canUndo());
}

void GameTests::winningSwitchesStateAndClearsTheSave() {
    const SudokuBoard expected = installSavedGame(1);
    const int last = firstEmptyCell(expected);

    SudokuGame game;
    QVERIFY(game.hasSavedGame());
    game.resumeSavedGame();
    QCOMPARE(game.state(), SudokuGame::Playing);
    QCOMPARE(game.elapsedSeconds(), 42);
    QCOMPARE(game.selectedIndex(), last);

    // A wrong entry is flagged immediately and does not win.
    game.enterDigit(expected.puzzle().solution[size_t(last)] % 9 + 1);
    QVERIFY(cellBool(game.cells(), last, CellModel::WrongRole));
    QCOMPARE(game.state(), SudokuGame::Playing);

    game.enterDigit(expected.puzzle().solution[size_t(last)]);
    QVERIFY(!cellBool(game.cells(), last, CellModel::WrongRole));
    QCOMPARE(game.state(), SudokuGame::Won);
    QVERIFY(!game.hasSavedGame());
    QVERIFY(QSettings().value(QStringLiteral("state/v1")).toString().isEmpty());
}

void GameTests::checkAsYouGoIsRemembered() {
    {
        SudokuGame game;
        game.newGame(SudokuGame::Easy);
        game.setCheckAsYouGo(false);
        QVERIFY(!game.checkAsYouGo());
        game.backToStart();
    }
    SudokuGame game;
    QVERIFY(!game.checkAsYouGo());
}

void GameTests::savedGameSurvivesRestart() {
    int cell = -1;
    {
        SudokuGame game;
        game.newGame(SudokuGame::Medium);
        cell = game.selectedIndex();
        game.enterDigit(7);
        game.toggleNotesMode();
        game.select(cell);
        game.backToStart();  // saves on the way out
        QVERIFY(game.hasSavedGame());
    }

    SudokuGame resumed;
    QVERIFY(resumed.hasSavedGame());
    QCOMPARE(resumed.state(), SudokuGame::Start);
    resumed.resumeSavedGame();
    QCOMPARE(resumed.state(), SudokuGame::Playing);
    QCOMPARE(resumed.difficulty(), int(Difficulty::Medium));
    QCOMPARE(cellInt(resumed.cells(), cell, CellModel::ValueRole), 7);

    resumed.newGame(SudokuGame::Easy);  // a new game drops the old save
    QVERIFY(!resumed.hasSavedGame());
}

void GameTests::solvedSaveIsNotOffered() {
    installSavedGame(0);
    SudokuGame game;
    QVERIFY(!game.hasSavedGame());
    QCOMPARE(game.state(), SudokuGame::Start);
}

void GameTests::clockRunsOnlyWhilePlaying() {
    SudokuGame game;
    QCOMPARE(game.elapsedSeconds(), 0);
    game.newGame(SudokuGame::Easy);
    QTRY_COMPARE_WITH_TIMEOUT(game.elapsedSeconds(), 1, 3000);

    game.backToStart();
    const int stopped = game.elapsedSeconds();
    QTest::qWait(1200);
    QCOMPARE(game.elapsedSeconds(), stopped);  // paused on the start screen
}

void GameTests::selectedValueFollowsTheSelection() {
    SudokuGame game;
    game.newGame(SudokuGame::Easy);
    const int empty = game.selectedIndex();
    QCOMPARE(game.selectedValue(), 0);

    QSignalSpy spy(&game, &SudokuGame::selectedValueChanged);
    game.enterDigit(6);
    QCOMPARE(game.selectedValue(), 6);
    QVERIFY(spy.count() > 0);

    game.select(firstGiven(game.cells()));
    QCOMPARE(game.selectedValue(), cellInt(game.cells(), game.selectedIndex(), CellModel::ValueRole));

    game.select(empty);
    QCOMPARE(game.selectedValue(), 6);
}

void GameTests::untouchedPuzzleIsNotInProgress() {
    SudokuGame game;
    QVERIFY(!game.inProgress());
    game.newGame(SudokuGame::Easy);
    QVERIFY(!game.inProgress());  // givens alone are not progress

    game.enterDigit(1);
    QVERIFY(game.inProgress());
    game.undo();
    QVERIFY(!game.inProgress());

    game.backToStart();
    QVERIFY(!game.hasSavedGame());  // nothing worth resuming
}
