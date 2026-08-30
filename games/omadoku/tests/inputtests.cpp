#include "inputtests.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include "cellmodel.h"
#include "savedgame.h"
#include "sudoku.h"
#include "sudokugame.h"
#include "sudokukeys.h"

namespace {

// X11 sends the physical top row as keycodes 10-18; anything else here stands
// for "some other key entirely".
constexpr quint32 kScanCodeOne = 10;
constexpr quint32 kScanCodeElsewhere = 43;

int digitFor(int key, Qt::KeyboardModifiers modifiers = Qt::NoModifier,
             const QString &text = QString(), quint32 scanCode = kScanCodeElsewhere) {
    return SudokuKeys::digitFor(key, modifiers, text, scanCode);
}

QList<int> selectionOf(const SudokuGame &game) {
    QList<int> indices;
    for (const QVariant &index : game.selectedIndices())
        indices << index.toInt();
    return indices;
}

int notesOf(SudokuGame *game, int cell) {
    QAbstractListModel *model = game->cells();
    return model->data(model->index(cell, 0), CellModel::NotesRole).toInt();
}

int valueOf(SudokuGame *game, int cell) {
    QAbstractListModel *model = game->cells();
    return model->data(model->index(cell, 0), CellModel::ValueRole).toInt();
}

// A row of cells the player may write in, so a sweep across it is meaningful.
QList<int> emptyRow(SudokuGame *game) {
    for (int row = 0; row < Sudoku::kSize; ++row) {
        QList<int> empties;
        for (int column = 0; column < Sudoku::kSize; ++column) {
            const int index = row * Sudoku::kSize + column;
            if (valueOf(game, index) == 0)
                empties << index;
        }
        if (empties.size() >= 3)
            return empties;
    }
    return {};
}

}  // namespace

void InputTests::initTestCase() {
    static QTemporaryDir dir;
    QVERIFY(dir.isValid());
    m_settingsDir = dir.path();
    TestSupport::redirectSettings(m_settingsDir);
}

void InputTests::init() {
    QSettings settings;
    settings.clear();
    settings.sync();
}

void InputTests::plainDigitKeysResolve() {
    for (int digit = 1; digit <= 9; ++digit) {
        const int key = Qt::Key_0 + digit;
        QCOMPARE(digitFor(key, Qt::NoModifier, QString::number(digit)), digit);
        // A modifier changes what the digit does, never which digit it is.
        QCOMPARE(digitFor(key, Qt::ControlModifier), digit);
        QCOMPARE(digitFor(key, Qt::AltModifier), digit);
        QCOMPARE(digitFor(key, Qt::ShiftModifier), digit);
    }
    QCOMPARE(digitFor(Qt::Key_0, Qt::NoModifier, QStringLiteral("0")), 0);  // erase, not a digit
}

void InputTests::shiftedDigitsResolveOnEveryLayout() {
    // US: the character the key produced names the digit.
    QCOMPARE(digitFor(Qt::Key_Exclam, Qt::ShiftModifier, QStringLiteral("!"), kScanCodeOne), 1);
    QCOMPARE(digitFor(Qt::Key_Percent, Qt::ShiftModifier, QStringLiteral("%")), 5);
    // Same layout, no text on the event: the key itself still names it.
    QCOMPARE(digitFor(Qt::Key_ParenLeft, Qt::ShiftModifier), 9);
    // Anything else: the physical top row is the last resort.
    QCOMPARE(digitFor(Qt::Key_Eacute, Qt::NoModifier, QStringLiteral("é"), 11), 2);
    QCOMPARE(digitFor(Qt::Key_Agrave, Qt::NoModifier, QStringLiteral("à"), 18), 9);
}

void InputTests::modifiersAndOtherKeysResolveToNothing() {
    // The bug this guards: a bare Shift carries no text, and an empty string
    // used to match the first shifted digit and type a 1.
    const QList<int> modifierKeys {Qt::Key_Shift, Qt::Key_Control, Qt::Key_Alt, Qt::Key_AltGr,
                                   Qt::Key_Meta, Qt::Key_Super_L, Qt::Key_CapsLock,
                                   Qt::Key_NumLock, Qt::Key_Mode_switch};
    for (int key : modifierKeys) {
        QCOMPARE(digitFor(key), 0);
        QCOMPARE(digitFor(key, Qt::ShiftModifier), 0);
        // Even claiming a number-row scan code cannot make a modifier a digit.
        QCOMPARE(digitFor(key, Qt::ShiftModifier, QString(), kScanCodeOne), 0);
    }

    // Every other key the board binds means itself, never a digit.
    const QList<int> otherKeys {Qt::Key_H, Qt::Key_J, Qt::Key_K, Qt::Key_L, Qt::Key_N, Qt::Key_R,
                                Qt::Key_V, Qt::Key_Left, Qt::Key_Up, Qt::Key_Escape,
                                Qt::Key_Backspace, Qt::Key_Delete, Qt::Key_Space};
    for (int key : otherKeys) {
        QCOMPARE(digitFor(key), 0);
        QCOMPARE(digitFor(key, Qt::ShiftModifier), 0);
    }
}

void InputTests::theBridgeAnswersTheSameAsTheHelper() {
    SudokuGame game;
    QCOMPARE(game.digitForKey(Qt::Key_4, Qt::NoModifier, QStringLiteral("4"), int(kScanCodeElsewhere)), 4);
    QCOMPARE(game.digitForKey(Qt::Key_Shift, Qt::ShiftModifier, QString(), int(kScanCodeElsewhere)), 0);
    QCOMPARE(game.digitForKey(Qt::Key_Exclam, Qt::ShiftModifier, QStringLiteral("!"), int(kScanCodeOne)), 1);
}

void InputTests::ctrlClickTogglesCellsInAndOutOfTheSelection() {
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

void InputTests::shiftArrowsSweepTheCellsTheCursorCrosses() {
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

void InputTests::plainMovesAndClicksCollapseTheSelection() {
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

void InputTests::notesGoToEverySelectedEmptyCellAsOneStep() {
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

    // Toggling twice puts every cell back where it started, still per cell.
    game.toggleNote(7);
    game.toggleNote(7);
    QCOMPARE(notesOf(&game, empties.at(0)), 0);
    QCOMPARE(notesOf(&game, empties.at(2)), 0);
}

void InputTests::aValueGoesToTheCursorAndFoldsTheSelection() {
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

void InputTests::escapeUnwindsSelectionThenHighlight() {
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
