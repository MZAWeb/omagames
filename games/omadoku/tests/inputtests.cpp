#include "inputtests.h"

#include <QKeyEvent>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include "cellmodel.h"
#include "savedgame.h"
#include "sudoku.h"
#include "sudokugame.h"
#include "sudokukeys.h"

namespace {

// A US keyboard sends the digit row as keycodes 10-18. Events in these tests
// carry codes from that window on purpose: nothing about the resolution may
// depend on them, because a synthetic keymap can put any key there.
constexpr quint32 kDigitRowScanCode = 10;

int digitFor(int key, Qt::KeyboardModifiers modifiers = Qt::NoModifier,
             const QString &text = QString()) {
    return SudokuKeys::digitFor(key, modifiers, text);
}

// The dispatch BoardScreen.qml performs on every press, so a test drives the
// same path the running game does.
void deliver(SudokuGame *game, const QKeyEvent &event) {
    if (event.type() != QEvent::KeyPress)
        return;  // the board only ever reads presses
    const int digit = game->digitForKey(event.key(), int(event.modifiers()), event.text());
    if (digit > 0) {
        game->pressDigitKey(digit, int(event.modifiers()));
        return;
    }
    int deltaRow = 0;
    int deltaColumn = 0;
    switch (event.key()) {
    case Qt::Key_Left:
    case Qt::Key_H:
        deltaColumn = -1;
        break;
    case Qt::Key_Right:
    case Qt::Key_L:
        deltaColumn = 1;
        break;
    case Qt::Key_Up:
    case Qt::Key_K:
        deltaRow = -1;
        break;
    case Qt::Key_Down:
    case Qt::Key_J:
        deltaRow = 1;
        break;
    default:
        return;
    }
    if (event.modifiers() & Qt::ShiftModifier)
        game->extendSelection(deltaRow, deltaColumn);
    else
        game->moveCursor(deltaRow, deltaColumn);
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

void InputTests::shiftedDigitsResolveOnUsLayouts() {
    // The character the key produced names the digit.
    QCOMPARE(digitFor(Qt::Key_Exclam, Qt::ShiftModifier, QStringLiteral("!")), 1);
    QCOMPARE(digitFor(Qt::Key_Percent, Qt::ShiftModifier, QStringLiteral("%")), 5);
    // No text on the event: the key itself still names it.
    QCOMPARE(digitFor(Qt::Key_ParenLeft, Qt::ShiftModifier), 9);

    // Without Shift those same symbols are just symbols, and a key whose only
    // claim to being a digit was where it sits on the keyboard is not one:
    // guessing from the hardware is what let an arrow write a note.
    QCOMPARE(digitFor(Qt::Key_Exclam, Qt::NoModifier, QStringLiteral("!")), 0);
    QCOMPARE(digitFor(Qt::Key_Eacute, Qt::NoModifier, QStringLiteral("é")), 0);
    QCOMPARE(digitFor(Qt::Key_Minus, Qt::NoModifier, QStringLiteral("-")), 0);
}

void InputTests::modifiersAndOtherKeysResolveToNothing() {
    // The bug this guards: a bare Shift carries no text, and an empty string
    // used to match the first shifted digit and type a 1.
    const QList<int> modifierKeys {Qt::Key_Shift, Qt::Key_Control, Qt::Key_Alt, Qt::Key_AltGr,
                                   Qt::Key_Meta, Qt::Key_Super_L, Qt::Key_CapsLock,
                                   Qt::Key_NumLock, Qt::Key_Mode_switch};
    // Every other key the board binds means itself, never a digit — the arrows
    // most of all, since Shift+arrow is how a selection is swept.
    const QList<int> otherKeys {Qt::Key_H, Qt::Key_J, Qt::Key_K, Qt::Key_L, Qt::Key_N, Qt::Key_R,
                                Qt::Key_V, Qt::Key_Left, Qt::Key_Right, Qt::Key_Up, Qt::Key_Down,
                                Qt::Key_Escape, Qt::Key_Backspace, Qt::Key_Delete, Qt::Key_Space,
                                Qt::Key_Tab, Qt::Key_F1, Qt::Key_unknown};
    for (int key : modifierKeys + otherKeys) {
        QCOMPARE(digitFor(key), 0);
        QCOMPARE(digitFor(key, Qt::ShiftModifier), 0);
        QCOMPARE(digitFor(key, Qt::ControlModifier), 0);
        // Nor does the text a compositor may attach to them make a difference.
        QCOMPARE(digitFor(key, Qt::ShiftModifier, QString()), 0);
    }
}

void InputTests::theBridgeAnswersTheSameAsTheHelper() {
    SudokuGame game;
    QCOMPARE(game.digitForKey(Qt::Key_4, Qt::NoModifier, QStringLiteral("4")), 4);
    QCOMPARE(game.digitForKey(Qt::Key_Shift, Qt::ShiftModifier, QString()), 0);
    QCOMPARE(game.digitForKey(Qt::Key_Exclam, Qt::ShiftModifier, QStringLiteral("!")), 1);
}

void InputTests::aShiftHeldSweepNeverWritesANote() {
    SudokuGame game;
    game.newGame(QStringLiteral("easy"));

    // Somewhere the sweep has room to run, starting on a cell the player may
    // actually write in, so a stray note would have somewhere to land.
    int start = -1;
    for (int index = 0; index < Sudoku::kCells && start < 0; ++index) {
        if (Sudoku::rowOf(index) <= 6 && Sudoku::colOf(index) <= 5 && valueOf(&game, index) == 0)
            start = index;
    }
    QVERIFY(start >= 0);
    game.select(start);

    // Hold Shift, sweep right, right, down, let Shift go. The events carry
    // keycodes from the window a US keyboard uses for its digit row, which is
    // exactly what a synthetic keymap (wtype driving the real app) put an
    // arrow on: the resolution must not care.
    struct Press {
        QEvent::Type type;
        int key;
        Qt::KeyboardModifiers modifiers;
        quint32 scanCode;
    };
    const QList<Press> sequence {
        {QEvent::KeyPress, Qt::Key_Shift, Qt::ShiftModifier, 50},
        {QEvent::KeyPress, Qt::Key_Right, Qt::ShiftModifier, kDigitRowScanCode},
        {QEvent::KeyPress, Qt::Key_Right, Qt::ShiftModifier, kDigitRowScanCode},
        {QEvent::KeyPress, Qt::Key_Down, Qt::ShiftModifier, kDigitRowScanCode + 1},
        {QEvent::KeyRelease, Qt::Key_Shift, Qt::NoModifier, 50},
    };
    for (const Press &press : sequence) {
        const QKeyEvent event(press.type, press.key, press.modifiers, press.scanCode, 0, 0, QString());
        deliver(&game, event);
    }

    // Nothing was written anywhere, and there is nothing to undo.
    for (int index = 0; index < Sudoku::kCells; ++index)
        QCOMPARE(notesOf(&game, index), 0);
    QVERIFY(!game.canUndo());

    // ...and the sweep itself went where it was aimed.
    QCOMPARE(selectionOf(game), QList<int>({start, start + 1, start + 2, start + 2 + Sudoku::kSize}));
    QCOMPARE(game.cursorIndex(), start + 2 + Sudoku::kSize);

    // The very same sweep with a digit in it still notes, so the guard has not
    // simply switched the keyboard off.
    const QKeyEvent five(QEvent::KeyPress, Qt::Key_5, Qt::ShiftModifier, 14, 0, 0, QStringLiteral("%"));
    deliver(&game, five);
    QCOMPARE(notesOf(&game, start), 1 << 4);
    QVERIFY(game.canUndo());
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
