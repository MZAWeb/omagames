#include "selectiontests.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include "gameprobe.h"
#include "savedgame.h"
#include "sudokuselection.h"

using TestSupport::emptyRow;
using TestSupport::notesOf;
using TestSupport::selectionOf;
using TestSupport::valueOf;

namespace {

QList<int> indicesOf(const SudokuSelection &selection) {
    QList<int> indices;
    for (int index : selection.indices())
        indices << index;
    return indices;
}

}  // namespace

void SelectionTests::initTestCase() {
    // Never touch the real ~/.config/Omacom while testing.
    static QTemporaryDir dir;
    QVERIFY(dir.isValid());
    m_settingsDir = dir.path();
    TestSupport::redirectSettings(m_settingsDir);
}

void SelectionTests::init() {
    QSettings settings;
    settings.clear();
    settings.sync();
}

void SelectionTests::selectingCollapsesOntoOneCell() {
    SudokuSelection selection;
    QCOMPARE(selection.cursor(), -1);
    QVERIFY(indicesOf(selection).isEmpty());

    QVERIFY(selection.select(40));
    QCOMPARE(indicesOf(selection), QList<int>({40}));
    QVERIFY(!selection.select(40));  // already there: nothing to announce

    selection.toggle(41);
    QVERIFY(selection.select(40));   // the same cell, but the sweep goes
    QCOMPARE(indicesOf(selection), QList<int>({40}));

    QVERIFY(selection.select(-1));   // nothing selected at all
    QCOMPARE(selection.cursor(), -1);
    QVERIFY(indicesOf(selection).isEmpty());

    QVERIFY(!selection.select(Sudoku::kCells));  // off the grid, ignored
    QVERIFY(!selection.select(-2));
    QCOMPARE(selection.cursor(), -1);
}

void SelectionTests::movingClampsAtTheEdges() {
    SudokuSelection selection;
    QVERIFY(selection.moveCursor(1, 1));  // nowhere yet: land on the first cell
    QCOMPARE(selection.cursor(), 0);

    QVERIFY(selection.moveCursor(1, 2));
    QCOMPARE(selection.cursor(), Sudoku::kSize + 2);

    QVERIFY(selection.moveCursor(-5, -5));  // clamped into the corner, but moved
    QCOMPARE(selection.cursor(), 0);
    QVERIFY(!selection.moveCursor(-1, 0));  // already there: nothing to announce
    QCOMPARE(selection.cursor(), 0);

    selection.select(Sudoku::kCells - 1);
    QVERIFY(!selection.moveCursor(1, 1));
    QCOMPARE(selection.cursor(), Sudoku::kCells - 1);
}

void SelectionTests::extendingTakesEveryCellCrossedAlongOnce() {
    SudokuSelection selection;
    QVERIFY(selection.extend(0, 1));  // nowhere yet: land on the first cell
    QCOMPARE(indicesOf(selection), QList<int>({0}));

    selection.select(40);
    QVERIFY(selection.extend(0, 2));  // two columns, both cells crossed
    QCOMPARE(indicesOf(selection), QList<int>({40, 41, 42}));
    QCOMPARE(selection.cursor(), 42);

    QVERIFY(selection.extend(0, -1));  // back over covered ground: no repeats
    QCOMPARE(indicesOf(selection), QList<int>({40, 41, 42}));
    QCOMPARE(selection.cursor(), 41);

    selection.select(0);
    QVERIFY(!selection.extend(-1, 0));  // the edge stops it rather than wrapping
    QCOMPARE(indicesOf(selection), QList<int>({0}));
}

void SelectionTests::togglingAddsAndDropsCellsAndCarriesTheCursor() {
    SudokuSelection selection;
    selection.select(40);
    QVERIFY(selection.toggle(41));
    QVERIFY(selection.toggle(42));
    QCOMPARE(indicesOf(selection), QList<int>({40, 41, 42}));
    QCOMPARE(selection.cursor(), 42);

    QVERIFY(selection.toggle(41));  // out again, cursor unaffected
    QCOMPARE(indicesOf(selection), QList<int>({40, 42}));
    QCOMPARE(selection.cursor(), 42);

    QVERIFY(selection.toggle(42));  // dropping the cursor falls back
    QCOMPARE(selection.cursor(), 40);
    QVERIFY(selection.toggle(40));  // an empty selection has no cursor at all
    QCOMPARE(selection.cursor(), -1);

    QVERIFY(!selection.toggle(-1));  // off the grid, ignored
    QVERIFY(!selection.toggle(Sudoku::kCells));
}

void SelectionTests::collapsingLeavesTheCursorWhereItIs() {
    SudokuSelection selection;
    selection.select(40);
    QVERIFY(!selection.collapse());  // one cell already

    selection.toggle(41);
    selection.toggle(42);
    QVERIFY(selection.collapse());
    QCOMPARE(indicesOf(selection), QList<int>({42}));
    QCOMPARE(selection.cursor(), 42);
    QVERIFY(!selection.collapse());
}

void SelectionTests::cursorMovesWithinTheGrid() {
    SudokuGame game;
    game.newGame(QStringLiteral("easy"));

    game.select(40);
    game.moveCursor(-1, 0);
    QCOMPARE(game.cursorIndex(), 31);
    game.moveCursor(0, 1);
    QCOMPARE(game.cursorIndex(), 32);

    game.select(0);
    game.moveCursor(-1, -1);  // clamped at the top-left corner
    QCOMPARE(game.cursorIndex(), 0);
    game.select(80);
    game.moveCursor(1, 1);
    QCOMPARE(game.cursorIndex(), 80);

    game.select(200);  // out of range is ignored
    QCOMPARE(game.cursorIndex(), 80);
}

void SelectionTests::ctrlClickTogglesCellsInAndOutOfTheSelection() {
    SudokuGame game;
    game.newGame(QStringLiteral("easy"));
    game.select(40);
    QCOMPARE(selectionOf(game), QList<int>({40}));

    QSignalSpy spy(&game, &SudokuGame::selectionChanged);
    game.toggleSelection(41);
    game.toggleSelection(42);
    QCOMPARE(selectionOf(game), QList<int>({40, 41, 42}));
    QCOMPARE(game.cursorIndex(), 42);  // the cell just picked is where you are
    QCOMPARE(spy.count(), 2);

    game.toggleSelection(41);  // out again, and the cursor is unaffected
    QCOMPARE(selectionOf(game), QList<int>({40, 42}));
    QCOMPARE(game.cursorIndex(), 42);

    game.toggleSelection(42);  // dropping the cursor falls back to the last left
    QCOMPARE(selectionOf(game), QList<int>({40}));
    QCOMPARE(game.cursorIndex(), 40);

    game.toggleSelection(40);  // and an empty selection has no cursor at all
    QVERIFY(selectionOf(game).isEmpty());
    QCOMPARE(game.cursorIndex(), -1);

    game.toggleSelection(200);  // out of range is ignored
    QVERIFY(selectionOf(game).isEmpty());
}

void SelectionTests::shiftArrowsSweepTheCellsTheCursorCrosses() {
    SudokuGame game;
    game.newGame(QStringLiteral("easy"));
    game.select(40);

    game.extendSelection(0, 1);
    game.extendSelection(0, 1);
    QCOMPARE(selectionOf(game), QList<int>({40, 41, 42}));
    QCOMPARE(game.cursorIndex(), 42);

    // Sweeping back over ground already covered moves the cursor and nothing
    // else: a cell joins the selection once.
    game.extendSelection(0, -1);
    QCOMPARE(selectionOf(game), QList<int>({40, 41, 42}));
    QCOMPARE(game.cursorIndex(), 41);

    game.extendSelection(1, 0);
    QCOMPARE(selectionOf(game), QList<int>({40, 41, 42, 50}));
    QCOMPARE(game.cursorIndex(), 50);

    // The edge of the grid stops the sweep instead of wrapping it.
    game.select(0);
    game.extendSelection(-1, 0);
    QCOMPARE(selectionOf(game), QList<int>({0}));
    QCOMPARE(game.cursorIndex(), 0);
}

void SelectionTests::plainMovesAndClicksCollapseTheSelection() {
    SudokuGame game;
    game.newGame(QStringLiteral("easy"));
    game.select(40);
    game.extendSelection(0, 1);
    game.extendSelection(0, 1);
    QCOMPARE(selectionOf(game).size(), 3);

    game.moveCursor(0, 1);
    QCOMPARE(selectionOf(game), QList<int>({43}));

    game.toggleSelection(44);
    game.toggleSelection(45);
    QCOMPARE(selectionOf(game).size(), 3);
    game.select(10);  // a plain click starts over
    QCOMPARE(selectionOf(game), QList<int>({10}));

    // Even a move that runs into an edge collapses: the arrow was still plain.
    game.select(0);
    game.toggleSelection(1);
    game.moveCursor(-1, 0);
    QCOMPARE(selectionOf(game), QList<int>({1}));
    QCOMPARE(game.cursorIndex(), 1);
}

void SelectionTests::notesGoToEverySelectedEmptyCellAsOneStep() {
    SudokuGame game;
    game.newGame(QStringLiteral("easy"));
    const QList<int> empties = emptyRow(&game);
    QVERIFY(empties.size() >= 3);

    // One of the three is filled in first, so the note has something to skip.
    game.select(empties.at(1));
    game.enterValue(5);
    QCOMPARE(valueOf(&game, empties.at(1)), 5);

    game.select(empties.at(0));
    game.toggleSelection(empties.at(1));
    game.toggleSelection(empties.at(2));

    game.toggleNote(7);
    QCOMPARE(notesOf(&game, empties.at(0)), 1 << 6);
    QCOMPARE(notesOf(&game, empties.at(2)), 1 << 6);
    QCOMPARE(notesOf(&game, empties.at(1)), 0);      // filled, so skipped
    QCOMPARE(valueOf(&game, empties.at(1)), 5);      // and left alone

    // One act, one undo step: both cells come back together.
    game.undo();
    QCOMPARE(notesOf(&game, empties.at(0)), 0);
    QCOMPARE(notesOf(&game, empties.at(2)), 0);
    QCOMPARE(valueOf(&game, empties.at(1)), 5);      // the entry is still there

    // A mixed selection converges rather than flipping cell by cell: with the
    // note in only one of them, one press writes it into both.
    game.select(empties.at(0));
    game.toggleNote(7);
    game.select(empties.at(0));
    game.toggleSelection(empties.at(1));
    game.toggleSelection(empties.at(2));
    game.toggleNote(7);
    QCOMPARE(notesOf(&game, empties.at(0)), 1 << 6);
    QCOMPARE(notesOf(&game, empties.at(2)), 1 << 6);

    // The press after that finds them all noted and clears the lot, in one
    // step: the undo brings the whole converged selection back.
    game.toggleNote(7);
    QCOMPARE(notesOf(&game, empties.at(0)), 0);
    QCOMPARE(notesOf(&game, empties.at(2)), 0);
    game.undo();
    QCOMPARE(notesOf(&game, empties.at(0)), 1 << 6);
    QCOMPARE(notesOf(&game, empties.at(2)), 1 << 6);
    QCOMPARE(valueOf(&game, empties.at(1)), 5);      // still filled, still skipped
}

void SelectionTests::aValueGoesToTheCursorAndFoldsTheSelection() {
    SudokuGame game;
    game.newGame(QStringLiteral("easy"));
    const QList<int> empties = emptyRow(&game);
    QVERIFY(empties.size() >= 3);

    game.select(empties.at(0));
    game.toggleSelection(empties.at(1));
    game.toggleSelection(empties.at(2));
    QCOMPARE(game.cursorIndex(), empties.at(2));

    game.enterValue(4);
    QCOMPARE(valueOf(&game, empties.at(2)), 4);
    QCOMPARE(valueOf(&game, empties.at(0)), 0);
    QCOMPARE(valueOf(&game, empties.at(1)), 0);
    QCOMPARE(selectionOf(game), QList<int>({empties.at(2)}));
}

void SelectionTests::escapeUnwindsSelectionThenHighlight() {
    SudokuGame game;
    game.newGame(QStringLiteral("easy"));
    game.select(40);
    game.extendSelection(0, 1);
    game.toggleHighlight(3);

    // The selection goes first, so a stray sweep never costs you the puzzle.
    QVERIFY(game.backOut());
    QCOMPARE(selectionOf(game), QList<int>({41}));
    QCOMPARE(game.highlightDigit(), 3);

    QVERIFY(game.backOut());
    QCOMPARE(game.highlightDigit(), -1);

    // Nothing left to unwind: the caller may leave the puzzle now.
    QVERIFY(!game.backOut());
}
