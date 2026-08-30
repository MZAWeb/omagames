#include "inputtests.h"

#include <QKeyEvent>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include "gameprobe.h"
#include "savedgame.h"
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

}  // namespace

using TestSupport::notesOf;
using TestSupport::selectionOf;
using TestSupport::valueOf;

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
