#include "inputtests.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include <cmath>

#include "gameprobe.h"
#include "savedgame.h"
#include "sudokuinput.h"

using TestSupport::cellInt;
using TestSupport::emptyRow;
using TestSupport::notesOf;
using TestSupport::selectionOf;
using TestSupport::valueOf;

namespace {

// WCAG relative luminance and contrast ratio, so "readable" is measured the
// way a contrast checker would rather than guessed at.
double luminance(const QColor &color) {
    auto channel = [](double v) { return v <= 0.03928 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4); };
    return 0.2126 * channel(color.redF()) + 0.7152 * channel(color.greenF())
        + 0.0722 * channel(color.blueF());
}

double contrastRatio(const QColor &a, const QColor &b) {
    const double first = luminance(a);
    const double second = luminance(b);
    return (std::max(first, second) + 0.05) / (std::min(first, second) + 0.05);
}

}  // namespace

void InputTests::initTestCase() {
    // Never touch the real ~/.config/Omacom while testing.
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

void InputTests::theClickModeWalksTheThreeActions() {
    SudokuInput input;
    QCOMPARE(input.clickMode(), QStringLiteral("fill"));  // the default
    QCOMPARE(int(input.clickAction()), int(SudokuInput::Action::Fill));

    const QStringList walk {QStringLiteral("highlight"), QStringLiteral("note"),
                            QStringLiteral("fill")};
    for (const QString &next : walk) {
        QCOMPARE(input.nextClickMode(), next);
        QVERIFY(input.setClickMode(next));
        QCOMPARE(input.clickMode(), next);
    }
    QVERIFY(!input.setClickMode(QStringLiteral("fill")));  // already there

    // An unknown id is a stale setting, not an error: it means Fill.
    input.setClickMode(QStringLiteral("note"));
    QVERIFY(input.setClickMode(QStringLiteral("nonsense")));
    QCOMPARE(input.clickMode(), QStringLiteral("fill"));
}

void InputTests::theNumberRowIgnoresTheClickMode() {
    using Action = SudokuInput::Action;
    QCOMPARE(int(SudokuInput::keyAction(Qt::NoModifier)), int(Action::Fill));
    QCOMPARE(int(SudokuInput::keyAction(Qt::ShiftModifier)), int(Action::Note));
    QCOMPARE(int(SudokuInput::keyAction(Qt::ControlModifier)), int(Action::Highlight));
    QCOMPARE(int(SudokuInput::keyAction(Qt::AltModifier)), int(Action::Highlight));
    // Ctrl wins over Shift, and a keypad modifier changes nothing.
    QCOMPARE(int(SudokuInput::keyAction(Qt::ControlModifier | Qt::ShiftModifier)),
             int(Action::Highlight));
    QCOMPARE(int(SudokuInput::keyAction(Qt::KeypadModifier)), int(Action::Fill));
}

void InputTests::aPlainDigitOverSeveralCellsMeansANote() {
    using Action = SudokuInput::Action;
    // One cell (or none yet) leaves every action as it was.
    for (int cells : {0, 1}) {
        QCOMPARE(int(SudokuInput::actionFor(Action::Fill, cells)), int(Action::Fill));
        QCOMPARE(int(SudokuInput::actionFor(Action::Note, cells)), int(Action::Note));
        QCOMPARE(int(SudokuInput::actionFor(Action::Highlight, cells)), int(Action::Highlight));
    }
    // Several cells can only mean pencil marks; the other two already say
    // what they mean.
    QCOMPARE(int(SudokuInput::actionFor(Action::Fill, 2)), int(Action::Note));
    QCOMPARE(int(SudokuInput::actionFor(Action::Note, 2)), int(Action::Note));
    QCOMPARE(int(SudokuInput::actionFor(Action::Highlight, 2)), int(Action::Highlight));
}

void InputTests::theHighlightTakesOneDigitAtATime() {
    SudokuInput input;
    QCOMPARE(input.highlightDigit(), -1);
    QVERIFY(!input.clearHighlight());  // nothing lit: nothing to announce

    QVERIFY(input.toggleHighlight(4));
    QCOMPARE(input.highlightDigit(), 4);
    QVERIFY(input.toggleHighlight(7));  // another digit switches
    QCOMPARE(input.highlightDigit(), 7);
    QVERIFY(input.toggleHighlight(7));  // the same digit clears
    QCOMPARE(input.highlightDigit(), -1);

    // Only 1-9 light anything up; anything else clears.
    QVERIFY(!input.toggleHighlight(0));
    QVERIFY(!input.toggleHighlight(10));
    input.toggleHighlight(3);
    QVERIFY(input.toggleHighlight(0));
    QCOMPARE(input.highlightDigit(), -1);
}

void InputTests::clickModeDecidesWhatAKeypadClickDoes() {
    SudokuGame game;
    game.newGame(QStringLiteral("easy"));
    const int cell = game.cursorIndex();
    QCOMPARE(game.clickMode(), QStringLiteral("fill"));  // the default

    game.clickDigit(4);
    QCOMPARE(cellInt(&game, cell, CellModel::ValueRole), 4);
    QCOMPARE(game.highlightDigit(), -1);

    game.erase();
    game.setClickMode(QStringLiteral("note"));
    game.clickDigit(3);
    game.clickDigit(8);
    QCOMPARE(cellInt(&game, cell, CellModel::NotesRole), (1 << 2) | (1 << 7));
    QCOMPARE(cellInt(&game, cell, CellModel::ValueRole), 0);
    game.clickDigit(3);  // toggles back off
    QCOMPARE(cellInt(&game, cell, CellModel::NotesRole), 1 << 7);

    game.setClickMode(QStringLiteral("highlight"));
    game.clickDigit(6);
    QCOMPARE(game.highlightDigit(), 6);
    QCOMPARE(cellInt(&game, cell, CellModel::ValueRole), 0);
    game.clickDigit(6);  // the same digit again clears it
    QCOMPARE(game.highlightDigit(), -1);

    game.setClickMode(QStringLiteral("nonsense"));  // an unknown mode lands on Fill
    QCOMPARE(game.clickMode(), QStringLiteral("fill"));
}

void InputTests::keyboardMappingIgnoresTheClickMode() {
    SudokuGame game;
    game.newGame(QStringLiteral("easy"));
    const int cell = game.cursorIndex();

    // Whichever mode the selector is on, the number row means the same thing.
    const QStringList modes {QStringLiteral("highlight"), QStringLiteral("note"),
                             QStringLiteral("fill")};
    for (const QString &mode : modes) {
        game.setClickMode(mode);

        game.pressDigitKey(5, Qt::NoModifier);
        QCOMPARE(cellInt(&game, cell, CellModel::ValueRole), 5);
        game.erase();

        game.pressDigitKey(7, Qt::ShiftModifier);
        QCOMPARE(cellInt(&game, cell, CellModel::NotesRole), 1 << 6);
        QCOMPARE(cellInt(&game, cell, CellModel::ValueRole), 0);
        game.pressDigitKey(7, Qt::ShiftModifier);
        QCOMPARE(cellInt(&game, cell, CellModel::NotesRole), 0);

        game.pressDigitKey(9, Qt::ControlModifier);
        QCOMPARE(game.highlightDigit(), 9);
        game.pressDigitKey(9, Qt::AltModifier);  // Alt is an alias for Ctrl here
        QCOMPARE(game.highlightDigit(), -1);
        QCOMPARE(cellInt(&game, cell, CellModel::ValueRole), 0);
    }
}

void InputTests::aPlainDigitNotesTheWholeSelection() {
    SudokuGame game;
    game.newGame(QStringLiteral("easy"));
    const QList<int> empties = emptyRow(&game);
    QVERIFY(empties.size() >= 3);

    game.select(empties.at(0));
    game.toggleSelection(empties.at(1));
    game.toggleSelection(empties.at(2));

    // A plain digit over the sweep pencils it in everywhere instead of
    // filling one cell and throwing the selection away.
    game.pressDigitKey(6, Qt::NoModifier);
    for (int index : empties.mid(0, 3))
        QCOMPARE(notesOf(&game, index), 1 << 5);
    QCOMPARE(valueOf(&game, empties.at(2)), 0);
    QCOMPARE(selectionOf(game), empties.mid(0, 3));

    // Same digit again clears the lot, like any note.
    game.pressDigitKey(6, Qt::NoModifier);
    for (int index : empties.mid(0, 3))
        QCOMPARE(notesOf(&game, index), 0);

    // The keypad in Fill mode reads the selection the same way.
    game.setClickMode(QStringLiteral("fill"));
    game.clickDigit(4);
    for (int index : empties.mid(0, 3))
        QCOMPARE(notesOf(&game, index), 1 << 3);
    QCOMPARE(valueOf(&game, empties.at(2)), 0);

    // Back on one cell a plain digit fills again.
    game.select(empties.at(2));
    game.pressDigitKey(9, Qt::NoModifier);
    QCOMPARE(valueOf(&game, empties.at(2)), 9);
}

void InputTests::clickModeCyclesAndPersists() {
    {
        SudokuGame game;
        QSignalSpy spy(&game, &SudokuGame::clickModeChanged);
        game.newGame(QStringLiteral("easy"));
        QCOMPARE(game.clickMode(), QStringLiteral("fill"));  // the default

        game.cycleClickMode();
        QCOMPARE(game.clickMode(), QStringLiteral("highlight"));
        game.cycleClickMode();
        QCOMPARE(game.clickMode(), QStringLiteral("note"));
        game.cycleClickMode();
        QCOMPARE(game.clickMode(), QStringLiteral("fill"));
        QCOMPARE(spy.count(), 3);

        game.setClickMode(QStringLiteral("note"));
        game.newGame(QStringLiteral("hard"));
        QCOMPARE(game.clickMode(), QStringLiteral("note"));  // a preference, not game state
    }
    SudokuGame restarted;
    QCOMPARE(restarted.clickMode(), QStringLiteral("note"));
}

void InputTests::highlightTogglesAndSwitchesDigits() {
    SudokuGame game;
    game.newGame(QStringLiteral("easy"));
    QCOMPARE(game.highlightDigit(), -1);

    QSignalSpy spy(&game, &SudokuGame::highlightDigitChanged);
    game.toggleHighlight(4);
    QCOMPARE(game.highlightDigit(), 4);
    game.toggleHighlight(4);  // same digit clears
    QCOMPARE(game.highlightDigit(), -1);

    game.toggleHighlight(4);
    game.toggleHighlight(7);  // another digit switches
    QCOMPARE(game.highlightDigit(), 7);
    game.clearHighlight();
    QCOMPARE(game.highlightDigit(), -1);
    QCOMPARE(spy.count(), 5);

    game.toggleHighlight(0);  // not a digit
    QCOMPARE(game.highlightDigit(), -1);

    game.toggleHighlight(2);
    game.newGame(QStringLiteral("easy"));
    QCOMPARE(game.highlightDigit(), -1);  // a new puzzle starts clean

    game.toggleHighlight(2);
    game.backToStart();
    QCOMPARE(game.highlightDigit(), -1);
}

void InputTests::highlightWearsAFixedHighlighterYellow() {
    // The deliberate exception to the theming rule: this pair never moves with
    // the desktop theme, so the test states what it is meant to look like.
    const QColor yellow = SudokuGame::highlightColor();
    QCOMPARE(yellow, SudokuInput::highlightColor());
    QVERIFY(yellow.isValid());
    QCOMPARE(yellow.alpha(), 255);
    QVERIFY(yellow.hslHueF() * 360.0 > 45.0);   // yellow, not orange or green
    QVERIFY(yellow.hslHueF() * 360.0 < 70.0);
    QVERIFY(yellow.hslSaturationF() > 0.8);     // a marker, not a pastel
    QVERIFY(yellow.lightnessF() > 0.5);

    // Dark ink on it, with room to spare over the 7:1 a contrast checker asks
    // of small text.
    const QColor ink = SudokuGame::highlightInk();
    QCOMPARE(ink, SudokuInput::highlightInk());
    QVERIFY(ink.lightnessF() < 0.15);
    QVERIFY(contrastRatio(yellow, ink) > 7.0);
}
