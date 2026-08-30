#pragma once

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

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
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE select NOTIFY selectedIndexChanged)
    Q_PROPERTY(int selectedValue READ selectedValue NOTIFY selectedValueChanged)
    Q_PROPERTY(int highlightDigit READ highlightDigit NOTIFY highlightDigitChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY boardChanged)
    Q_PROPERTY(int filledCount READ filledCount NOTIFY boardChanged)
    Q_PROPERTY(bool inProgress READ inProgress NOTIFY boardChanged)
    Q_PROPERTY(QVariantList digitCounts READ digitCounts NOTIFY boardChanged)
    Q_PROPERTY(int elapsedSeconds READ elapsedSeconds NOTIFY elapsedSecondsChanged)
    Q_PROPERTY(bool hasSavedGame READ hasSavedGame NOTIFY hasSavedGameChanged)

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
    int selectedIndex() const { return m_selectedIndex; }
    // Digit under the selection (0 when empty), so the UI can highlight twins.
    int selectedValue() const { return m_board.value(m_selectedIndex); }
    bool canUndo() const { return m_board.canUndo(); }
    int filledCount() const { return m_board.filledCount(); }
    bool inProgress() const;
    QVariantList digitCounts() const;
    // Digit the player asked to see everywhere on the board (-1 = none).
    int highlightDigit() const { return m_highlightDigit; }
    int elapsedSeconds() const { return m_elapsedSeconds; }
    bool hasSavedGame() const { return m_hasSavedGame; }

    // Which digit a key press means, 0 when it means none. QML asks rather
    // than deciding, so the layout quirks stay tested (see sudokukeys.h).
    Q_INVOKABLE int digitForKey(int key, int modifiers, const QString &text, int nativeScanCode) const;

    Q_INVOKABLE void newGame(const QString &difficulty);
    Q_INVOKABLE void resumeSavedGame();
    Q_INVOKABLE void select(int index);
    Q_INVOKABLE void moveSelection(int deltaRow, int deltaColumn);
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
    void selectedIndexChanged();
    void selectedValueChanged();
    void highlightDigitChanged();
    void elapsedSecondsChanged();
    void hasSavedGameChanged();

private:
    enum class Screen { Start, Playing, Won };

    // What a click on the digit pad does. Fill by default: writing digits is
    // what a player does most, and every other action has its own chord.
    enum class ClickMode { Highlight, Note, Fill };

    static ClickMode modeFromId(const QString &id, ClickMode fallback);
    void applyChange(const std::vector<int> &changed);
    void setScreen(Screen screen);
    void setHighlightDigit(int digit);
    void setHasSavedGame(bool hasSavedGame);
    void selectFirstEmptyCell();
    void loadSettings();
    void saveGame();
    void clearSavedGame();

    SudokuBoard m_board;
    CellModel m_cells {&m_board, this};
    QTimer m_clock {this};
    QTimer m_saveTimer {this};
    Screen m_screen = Screen::Start;
    int m_selectedIndex = -1;
    int m_highlightDigit = -1;
    int m_elapsedSeconds = 0;
    ClickMode m_clickMode = ClickMode::Fill;
    bool m_hasSavedGame = false;
};
