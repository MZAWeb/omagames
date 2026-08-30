#include "gametests.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

#include <cmath>

#include "cellmodel.h"
#include "savedgame.h"
#include "sudoku.h"
#include "sudokugame.h"
#include "sudokugrader.h"

namespace {

int cellInt(QAbstractListModel *model, int cell, CellModel::Role role) {
    return model->data(model->index(cell, 0), role).toInt();
}

bool cellBool(QAbstractListModel *model, int cell, CellModel::Role role) {
    return model->data(model->index(cell, 0), role).toBool();
}

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
    QCOMPARE(game.selectedIndex(), -1);
    QVERIFY(!game.validateAsYouGo());  // default off
}

void GameTests::newGameSelectsTheFirstEmptyCell() {
    SudokuGame game;
    QSignalSpy stateSpy(&game, &SudokuGame::stateChanged);
    game.newGame(QStringLiteral("hard"));

    QCOMPARE(game.state(), QStringLiteral("playing"));
    QCOMPARE(stateSpy.count(), 1);
    QCOMPARE(game.difficulty(), QStringLiteral("hard"));
    QVERIFY(game.selectedIndex() >= 0);
    QCOMPARE(cellInt(game.cells(), game.selectedIndex(), CellModel::ValueRole), 0);

    QVERIFY(game.filledCount() > 0);
    QCOMPARE(game.digitCounts().size(), 9);
}

void GameTests::selectionMovesWithinTheGrid() {
    SudokuGame game;
    game.newGame(QStringLiteral("easy"));

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
    game.newGame(QStringLiteral("easy"));
    const int empty = game.selectedIndex();  // the first empty cell
    const int given = firstGiven(game.cells());
    const int givenValue = cellInt(game.cells(), given, CellModel::ValueRole);

    game.select(given);
    game.enterValue(givenValue % 9 + 1);
    QCOMPARE(cellInt(game.cells(), given, CellModel::ValueRole), givenValue);
    QVERIFY(!game.canUndo());

    game.select(empty);
    game.enterValue(5);
    QCOMPARE(cellInt(game.cells(), empty, CellModel::ValueRole), 5);
    QVERIFY(game.canUndo());
}

void GameTests::exposesDifficultiesWithLabels() {
    SudokuGame game;
    const QVariantList levels = game.difficulties();
    QCOMPARE(levels.size(), 4);

    const QVariantMap easy = levels.first().toMap();
    QCOMPARE(easy.value(QStringLiteral("id")).toString(), QStringLiteral("easy"));
    QVERIFY(!easy.value(QStringLiteral("label")).toString().isEmpty());
    QVERIFY(!easy.value(QStringLiteral("description")).toString().isEmpty());
    QCOMPARE(easy.value(QStringLiteral("techniques")).toStringList(),
             QStringList({QStringLiteral("Naked single"), QStringLiteral("Hidden single")}));

    // Every level introduces at least one rung, and no rung is listed twice.
    QStringList seen;
    for (const QVariant &entry : levels) {
        const QStringList techniques = entry.toMap().value(QStringLiteral("techniques")).toStringList();
        QVERIFY(!techniques.isEmpty());
        for (const QString &technique : techniques) {
            QVERIFY2(!seen.contains(technique), qPrintable(technique + QStringLiteral(" listed twice")));
            seen << technique;
        }
    }
    QCOMPARE(seen.size(), SudokuGrader::kTechniqueCount);

    const QVariantMap extra = levels.last().toMap();
    QCOMPARE(extra.value(QStringLiteral("id")).toString(), QStringLiteral("extrahard"));
    game.newGame(extra.value(QStringLiteral("id")).toString());
    QCOMPARE(game.difficulty(), QStringLiteral("extrahard"));
    QCOMPARE(game.difficultyLabel(), extra.value(QStringLiteral("label")).toString());
    // What the puzzle needs is one of the rungs the level promises.
    QVERIFY(extra.value(QStringLiteral("techniques")).toStringList().contains(game.techniqueLabel()));

    game.newGame(QStringLiteral("hard"));
    QCOMPARE(game.difficulty(), QStringLiteral("hard"));

    game.newGame(QStringLiteral("nonsense"));  // an unknown id lands on Easy
    QCOMPARE(game.difficulty(), QStringLiteral("easy"));

    game.setClickMode(QStringLiteral("nonsense"));  // and an unknown mode on Fill
    QCOMPARE(game.clickMode(), QStringLiteral("fill"));
}

void GameTests::clickModeDecidesWhatAKeypadClickDoes() {
    SudokuGame game;
    game.newGame(QStringLiteral("easy"));
    const int cell = game.selectedIndex();
    QCOMPARE(game.clickMode(), QStringLiteral("fill"));  // the default

    game.clickDigit(4);
    QCOMPARE(cellInt(game.cells(), cell, CellModel::ValueRole), 4);
    QCOMPARE(game.highlightDigit(), -1);

    game.erase();
    game.setClickMode(QStringLiteral("note"));
    game.clickDigit(3);
    game.clickDigit(8);
    QCOMPARE(cellInt(game.cells(), cell, CellModel::NotesRole), (1 << 2) | (1 << 7));
    QCOMPARE(cellInt(game.cells(), cell, CellModel::ValueRole), 0);
    game.clickDigit(3);  // toggles back off
    QCOMPARE(cellInt(game.cells(), cell, CellModel::NotesRole), 1 << 7);

    game.setClickMode(QStringLiteral("highlight"));
    game.clickDigit(6);
    QCOMPARE(game.highlightDigit(), 6);
    QCOMPARE(cellInt(game.cells(), cell, CellModel::ValueRole), 0);
    game.clickDigit(6);  // the same digit again clears it
    QCOMPARE(game.highlightDigit(), -1);
}

void GameTests::keyboardMappingIgnoresTheClickMode() {
    SudokuGame game;
    game.newGame(QStringLiteral("easy"));
    const int cell = game.selectedIndex();

    // Whichever mode the selector is on, the number row means the same thing.
    const QStringList modes {QStringLiteral("highlight"), QStringLiteral("note"),
                             QStringLiteral("fill")};
    for (const QString &mode : modes) {
        game.setClickMode(mode);

        game.pressDigitKey(5, Qt::NoModifier);
        QCOMPARE(cellInt(game.cells(), cell, CellModel::ValueRole), 5);
        game.erase();

        game.pressDigitKey(7, Qt::ShiftModifier);
        QCOMPARE(cellInt(game.cells(), cell, CellModel::NotesRole), 1 << 6);
        QCOMPARE(cellInt(game.cells(), cell, CellModel::ValueRole), 0);
        game.pressDigitKey(7, Qt::ShiftModifier);
        QCOMPARE(cellInt(game.cells(), cell, CellModel::NotesRole), 0);

        game.pressDigitKey(9, Qt::ControlModifier);
        QCOMPARE(game.highlightDigit(), 9);
        game.pressDigitKey(9, Qt::AltModifier);  // Alt is an alias for Ctrl here
        QCOMPARE(game.highlightDigit(), -1);
        QCOMPARE(cellInt(game.cells(), cell, CellModel::ValueRole), 0);
    }
}

void GameTests::clickModeCyclesAndPersists() {
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

void GameTests::highlightTogglesAndSwitchesDigits() {
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

void GameTests::highlightWearsAFixedHighlighterYellow() {
    // The deliberate exception to the theming rule: this pair never moves with
    // the desktop theme, so the test states what it is meant to look like.
    const QColor yellow = SudokuGame::highlightColor();
    QVERIFY(yellow.isValid());
    QCOMPARE(yellow.alpha(), 255);
    QVERIFY(yellow.hslHueF() * 360.0 > 45.0);   // yellow, not orange or green
    QVERIFY(yellow.hslHueF() * 360.0 < 70.0);
    QVERIFY(yellow.hslSaturationF() > 0.8);     // a marker, not a pastel
    QVERIFY(yellow.lightnessF() > 0.5);

    // Dark ink on it, with room to spare over the 7:1 a contrast checker asks
    // of small text.
    const QColor ink = SudokuGame::highlightInk();
    QVERIFY(ink.lightnessF() < 0.15);
    QVERIFY(contrastRatio(yellow, ink) > 7.0);
}

void GameTests::undoRestartAndEraseGoThroughTheBoard() {
    SudokuGame game;
    game.newGame(QStringLiteral("easy"));
    const int cell = game.selectedIndex();
    const int filled = game.filledCount();

    game.enterValue(2);
    QCOMPARE(game.filledCount(), filled + 1);
    game.erase();
    QCOMPARE(game.filledCount(), filled);

    game.undo();  // undoes the erase
    QCOMPARE(cellInt(game.cells(), cell, CellModel::ValueRole), 2);
    game.undo();  // undoes the entry
    QCOMPARE(cellInt(game.cells(), cell, CellModel::ValueRole), 0);
    QVERIFY(!game.canUndo());

    game.enterValue(2);
    game.restart();
    QCOMPARE(game.filledCount(), filled);
    QVERIFY(!game.canUndo());
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

void GameTests::selectedValueFollowsTheSelection() {
    SudokuGame game;
    game.newGame(QStringLiteral("easy"));
    const int empty = game.selectedIndex();
    QCOMPARE(game.selectedValue(), 0);

    QSignalSpy spy(&game, &SudokuGame::selectedValueChanged);
    game.enterValue(6);
    QCOMPARE(game.selectedValue(), 6);
    QVERIFY(spy.count() > 0);

    game.select(firstGiven(game.cells()));
    QCOMPARE(game.selectedValue(), cellInt(game.cells(), game.selectedIndex(), CellModel::ValueRole));

    game.select(empty);
    QCOMPARE(game.selectedValue(), 6);
}

void GameTests::validateAsYouGoFlipsMidGame() {
    // A saved game built from a known seed, so the solution is at hand to
    // pick a digit that is certainly wrong.
    const SudokuBoard expected = TestSupport::installSavedGame(3, 20260829u);
    SudokuGame game;
    game.resumeSavedGame();
    const int cell = game.selectedIndex();
    QVERIFY(!game.validateAsYouGo());
    game.setValidateAsYouGo(true);

    game.enterValue(expected.puzzle().solution[size_t(cell)] % 9 + 1);
    QVERIFY(cellBool(game.cells(), cell, CellModel::WrongRole));

    QSignalSpy spy(&game, &SudokuGame::validateAsYouGoChanged);
    game.setValidateAsYouGo(false);  // off: the mark vanishes until the grid is full
    QVERIFY(!cellBool(game.cells(), cell, CellModel::WrongRole));
    game.setValidateAsYouGo(true);   // on: it is back at once, no new entry needed
    QVERIFY(cellBool(game.cells(), cell, CellModel::WrongRole));
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
