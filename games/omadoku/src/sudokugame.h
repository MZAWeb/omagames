#pragma once

#include <QColor>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include "besttimes.h"
#include "cellmodel.h"
#include "sudokuboard.h"

// The only bridge between the engine and QML: state as properties, actions as
// invokables. It owns no rules of its own beyond screen flow and persistence.
//
// Screen state, difficulty and click mode cross to QML as lowercase id strings
// rather than as enums: a C++-registered QML type would need a module for QML
// tooling to resolve, and plain strings keep both sides (and qmllint) honest
// without one. The matching display labels come from `difficulties`.
class SudokuGame : public QObject {
    Q_OBJECT
    Q_PROPERTY(QAbstractListModel *cells READ cells CONSTANT)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString difficulty READ difficulty NOTIFY boardChanged)
    Q_PROPERTY(QString difficultyLabel READ difficultyLabel NOTIFY boardChanged)
    Q_PROPERTY(QString techniqueLabel READ techniqueLabel NOTIFY boardChanged)
    Q_PROPERTY(QVariantList difficulties READ difficulties CONSTANT)
    Q_PROPERTY(QString clickMode READ clickMode WRITE setClickMode NOTIFY clickModeChanged)
    Q_PROPERTY(bool validateAsYouGo READ validateAsYouGo WRITE setValidateAsYouGo NOTIFY validateAsYouGoChanged)
    Q_PROPERTY(int cursorIndex READ cursorIndex NOTIFY selectionChanged)
    Q_PROPERTY(QVariantList selectedIndices READ selectedIndices NOTIFY selectionChanged)
    Q_PROPERTY(int cursorValue READ cursorValue NOTIFY cursorValueChanged)
    Q_PROPERTY(int highlightDigit READ highlightDigit NOTIFY highlightDigitChanged)
    Q_PROPERTY(QColor highlightColor READ highlightColor CONSTANT)
    Q_PROPERTY(QColor highlightInk READ highlightInk CONSTANT)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY boardChanged)
    Q_PROPERTY(int filledCount READ filledCount NOTIFY boardChanged)
    Q_PROPERTY(bool inProgress READ inProgress NOTIFY boardChanged)
    Q_PROPERTY(QVariantList digitCounts READ digitCounts NOTIFY boardChanged)
    Q_PROPERTY(int elapsedSeconds READ elapsedSeconds NOTIFY elapsedSecondsChanged)
    Q_PROPERTY(bool hasSavedGame READ hasSavedGame NOTIFY hasSavedGameChanged)
    Q_PROPERTY(QVariantList bestTimes READ bestTimes NOTIFY bestTimesChanged)
    Q_PROPERTY(QVariantMap bests READ bests NOTIFY bestTimesChanged)
    Q_PROPERTY(int newBestRank READ newBestRank NOTIFY stateChanged)

public:
    explicit SudokuGame(QObject *parent = nullptr);
    ~SudokuGame() override;

    QAbstractListModel *cells() { return &m_cells; }
    // "start" | "playing" | "won"
    QString state() const;
    // "easy" | "medium" | "hard" | "extrahard", plus the label to show for
    // it, and the full list the start screen offers: {id, label, techniques,
    // description}, where `techniques` names the rungs of the ladder the
    // level introduces, in the order a player would learn them.
    QString difficulty() const;
    QString difficultyLabel() const;
    static QVariantList difficulties();
    // The hardest technique the current puzzle needs, by name.
    QString techniqueLabel() const;
    // What a click on the keypad does: "highlight" | "note" | "fill". It has
    // no say over the keyboard, whose mapping is fixed (see pressDigitKey).
    QString clickMode() const;
    void setClickMode(const QString &clickMode);
    bool validateAsYouGo() const { return m_board.validateAsYouGo(); }
    void setValidateAsYouGo(bool validateAsYouGo);
    // The cell the keyboard acts on (-1 = none). It is always one of
    // `selectedIndices`, which holds the whole multi-cell selection in the
    // order the cells joined it — a single cell most of the time.
    int cursorIndex() const { return m_cursorIndex; }
    QVariantList selectedIndices() const;
    // Digit under the cursor (0 when empty), so the UI can highlight twins.
    int cursorValue() const { return m_board.value(m_cursorIndex); }
    bool canUndo() const { return m_board.canUndo(); }
    int filledCount() const { return m_board.filledCount(); }
    bool inProgress() const;
    QVariantList digitCounts() const;
    // Digit the player asked to see everywhere on the board (-1 = none).
    int highlightDigit() const { return m_highlightDigit; }
    // The single sanctioned exception to the theming rule (see CLAUDE.md): a
    // digit highlight is a highlighter pen, and a pen that changed color with
    // the desktop would stop reading as one. Fixed marker yellow, with the
    // dark ink that keeps a digit on top of it legible. Both live here rather
    // than in QML so no literal color is written into a .qml file.
    static QColor highlightColor();
    static QColor highlightInk();
    int elapsedSeconds() const { return m_elapsedSeconds; }
    bool hasSavedGame() const { return m_hasSavedGame; }
    // Every kept time as {difficulty, label, seconds, date}, fastest first
    // within each level, and the fastest of each level by id.
    QVariantList bestTimes() const;
    QVariantMap bests() const;
    // Where the win just now landed in its level's table (-1 = nowhere).
    int newBestRank() const { return m_newBestRank; }

    // Which digit a key press means, 0 when it means none. QML asks rather
    // than deciding, so the layout quirks stay tested (see sudokukeys.h).
    Q_INVOKABLE int digitForKey(int key, int modifiers, const QString &text, int nativeScanCode) const;

    Q_INVOKABLE void newGame(const QString &difficulty);
    Q_INVOKABLE void resumeSavedGame();
    // Collapse the selection onto one cell: a plain click, a plain arrow.
    Q_INVOKABLE void select(int index);
    // Ctrl+click: add the cell to the selection, or drop it out again.
    Q_INVOKABLE void toggleSelection(int index);
    Q_INVOKABLE void moveCursor(int deltaRow, int deltaColumn);
    // Shift+arrows: move the cursor and take every cell it passes over along.
    Q_INVOKABLE void extendSelection(int deltaRow, int deltaColumn);
    Q_INVOKABLE void collapseSelection();
    // Escape, one step at a time: a multi-cell selection, then the highlight.
    // False means nothing was left to undo and the caller may leave the puzzle.
    Q_INVOKABLE bool backOut();
    // A digit from the number row. The mapping is fixed and owes nothing to
    // the click mode: plain fills, Shift notes, Ctrl (or Alt) highlights.
    Q_INVOKABLE void pressDigitKey(int digit, int modifiers);
    // A digit clicked on the keypad, which is the one place clickMode decides.
    Q_INVOKABLE void clickDigit(int digit);
    Q_INVOKABLE void enterValue(int digit);
    Q_INVOKABLE void toggleNote(int digit);
    Q_INVOKABLE void toggleHighlight(int digit);
    Q_INVOKABLE void clearHighlight();
    Q_INVOKABLE void cycleClickMode();
    Q_INVOKABLE void erase();
    Q_INVOKABLE void undo();
    Q_INVOKABLE void restart();
    Q_INVOKABLE void backToStart();

    Q_INVOKABLE QVariantMap windowGeometry() const;
    Q_INVOKABLE void saveWindowGeometry(int x, int y, int width, int height, bool maximized);

signals:
    void stateChanged();
    void boardChanged();
    void clickModeChanged();
    void validateAsYouGoChanged();
    void selectionChanged();
    void cursorValueChanged();
    void highlightDigitChanged();
    void elapsedSecondsChanged();
    void hasSavedGameChanged();
    void bestTimesChanged();

private:
    enum class Screen { Start, Playing, Won };

    // What a click on the digit pad does. Fill by default: writing digits is
    // what a player does most, and every other action has its own chord.
    enum class ClickMode { Highlight, Note, Fill };

    static ClickMode modeFromId(const QString &id, ClickMode fallback);
    // The cell `deltaRow`/`deltaColumn` away, clamped to the grid: movement
    // stops at the edges because wrapping makes arrow keys feel lost.
    static int stepped(int index, int deltaRow, int deltaColumn);
    void applyChange(const std::vector<int> &changed);
    void setScreen(Screen screen);
    void setHighlightDigit(int digit);
    void setHasSavedGame(bool hasSavedGame);
    void selectFirstEmptyCell();
    void recordWin();
    void loadSettings();
    void saveGame();
    void clearSavedGame();

    SudokuBoard m_board;
    CellModel m_cells {&m_board, this};
    QTimer m_clock {this};
    QTimer m_saveTimer {this};
    Screen m_screen = Screen::Start;
    std::vector<int> m_selection;
    int m_cursorIndex = -1;
    int m_highlightDigit = -1;
    int m_elapsedSeconds = 0;
    BestTimes m_times;
    int m_newBestRank = -1;
    ClickMode m_clickMode = ClickMode::Fill;
    bool m_hasSavedGame = false;
};
