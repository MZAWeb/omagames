#pragma once

#include <QColor>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include "cellmodel.h"
#include "sudokuboard.h"
#include "scoretable.h"
#include "sudokuinput.h"
#include "sudokuselection.h"
#include "sudokustore.h"

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
    Q_PROPERTY(bool autoNotes READ autoNotes WRITE setAutoNotes NOTIFY autoNotesChanged)
    Q_PROPERTY(int cursorIndex READ cursorIndex NOTIFY selectionChanged)
    Q_PROPERTY(QVariantList selectedIndices READ selectedIndices NOTIFY selectionChanged)
    Q_PROPERTY(int cursorValue READ cursorValue NOTIFY cursorValueChanged)
    Q_PROPERTY(int highlightDigit READ highlightDigit NOTIFY highlightDigitChanged)
    Q_PROPERTY(QColor highlightColor READ highlightColor CONSTANT)
    Q_PROPERTY(QColor highlightInk READ highlightInk CONSTANT)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY boardChanged)
    Q_PROPERTY(int filledCount READ filledCount NOTIFY boardChanged)
    Q_PROPERTY(bool inProgress READ inProgress NOTIFY boardChanged)
    Q_PROPERTY(bool restartPending READ restartPending NOTIFY restartPendingChanged)
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
    // While on, every empty cell shows the digits still open to it and the
    // player's own marks are set aside until it goes off. Notes stop being
    // something a digit can write, so the keypad leaves Note mode with it.
    bool autoNotes() const { return m_board.autoNotes(); }
    void setAutoNotes(bool autoNotes);
    // The cell the keyboard acts on (-1 = none). It is always one of
    // `selectedIndices`, which holds the whole multi-cell selection in the
    // order the cells joined it — a single cell most of the time.
    int cursorIndex() const { return m_selection.cursor(); }
    QVariantList selectedIndices() const;
    // Digit under the cursor (0 when empty), so the UI can highlight twins.
    int cursorValue() const { return m_board.value(m_selection.cursor()); }
    bool canUndo() const { return m_board.canUndo(); }
    int filledCount() const { return m_board.filledCount(); }
    bool inProgress() const;
    // True while the restart dialog is up: QML shows it, and nothing wipes
    // the board until confirmRestart() answers it.
    bool restartPending() const { return m_restartPending; }
    QVariantList digitCounts() const;
    // Digit the player asked to see everywhere on the board (-1 = none), and
    // the fixed pair it is drawn in (see SudokuInput).
    int highlightDigit() const { return m_input.highlightDigit(); }
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
    Q_INVOKABLE int digitForKey(int key, int modifiers, const QString &text) const;

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
    // the click mode: plain fills, Shift notes, Ctrl (or Alt) highlights —
    // except that over a multi-cell selection a plain digit notes too.
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
    // Restarting throws away every entry and note, so QML cannot ask for it
    // outright: requestRestart() puts the question (or restarts an untouched
    // puzzle, where there is nothing to ask about) and only the dialog's
    // answer, confirmRestart(), goes through with it. restart() itself is
    // deliberately not invokable.
    Q_INVOKABLE void requestRestart();
    Q_INVOKABLE void confirmRestart();
    Q_INVOKABLE void cancelRestart();
    void restart();
    Q_INVOKABLE void backToStart();

    Q_INVOKABLE QVariantMap windowGeometry() const;
    Q_INVOKABLE void saveWindowGeometry(int x, int y, int width, int height, bool maximized);

signals:
    void stateChanged();
    void boardChanged();
    void clickModeChanged();
    void validateAsYouGoChanged();
    void autoNotesChanged();
    void selectionChanged();
    void cursorValueChanged();
    void highlightDigitChanged();
    void elapsedSecondsChanged();
    void hasSavedGameChanged();
    void restartPendingChanged();
    void bestTimesChanged();

private:
    enum class Screen { Start, Playing, Won };

    // The one place a digit turns into an act on the board, and the one that
    // knows how many cells are selected (see SudokuInput::actionFor).
    void applyDigit(SudokuInput::Action action, int digit);
    // Every way of picking cells lands here: where the cursor is and what the
    // digit keys will act on both moved, so both are announced together.
    void emitSelectionChanged();
    void applyChange(const std::vector<int> &changed);
    void setScreen(Screen screen);
    void setHasSavedGame(bool hasSavedGame);
    void setRestartPending(bool pending);
    void selectFirstEmptyCell();
    void recordWin();
    void loadSettings();
    void saveGame();
    void clearSavedGame();

    SudokuBoard m_board;
    SudokuStore m_store;
    CellModel m_cells {&m_board, this};
    QTimer m_clock {this};
    QTimer m_saveTimer {this};
    Screen m_screen = Screen::Start;
    SudokuSelection m_selection;
    SudokuInput m_input;
    int m_elapsedSeconds = 0;
    OmaGames::ScoreTable m_times;
    int m_newBestRank = -1;
    bool m_hasSavedGame = false;
    bool m_restartPending = false;
};
