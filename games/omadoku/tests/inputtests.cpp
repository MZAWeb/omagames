#include "inputtests.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include "savedgame.h"
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
