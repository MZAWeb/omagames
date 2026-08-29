#pragma once

#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include "cellmodel.h"
#include "sudokuboard.h"

// The only bridge between the engine and QML: state as properties, actions as
// invokables. It owns no rules of its own beyond screen flow and persistence.
class SudokuGame : public QObject {
    Q_OBJECT
    Q_PROPERTY(QAbstractListModel *cells READ cells CONSTANT)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(int difficulty READ difficulty NOTIFY boardChanged)
    Q_PROPERTY(bool notesMode READ notesMode WRITE setNotesMode NOTIFY notesModeChanged)
    Q_PROPERTY(bool checkAsYouGo READ checkAsYouGo WRITE setCheckAsYouGo NOTIFY checkAsYouGoChanged)
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
    enum State { Start, Playing, Won };
    Q_ENUM(State)

    // Mirrors Difficulty so QML can say SudokuGame.Medium.
    enum Level { Easy = int(Difficulty::Easy), Medium = int(Difficulty::Medium), Hard = int(Difficulty::Hard) };
    Q_ENUM(Level)

    explicit SudokuGame(QObject *parent = nullptr);
    ~SudokuGame() override;

    QAbstractListModel *cells() { return &m_cells; }
    State state() const { return m_state; }
    int difficulty() const { return int(m_board.puzzle().difficulty); }
    bool notesMode() const { return m_notesMode; }
    void setNotesMode(bool notesMode);
    bool checkAsYouGo() const { return m_board.checkAsYouGo(); }
    void setCheckAsYouGo(bool checkAsYouGo);
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

    Q_INVOKABLE void newGame(int level);
    Q_INVOKABLE void resumeSavedGame();
    Q_INVOKABLE void select(int index);
    Q_INVOKABLE void moveSelection(int deltaRow, int deltaColumn);
    // Entry (Ctrl+digit), notes (Shift+digit) and highlighting (plain digit)
    // are separate actions; enterDigit is the mode-governed one the digit pad
    // uses when a cell is selected.
    Q_INVOKABLE void enterDigit(int digit);
    Q_INVOKABLE void enterValue(int digit);
    Q_INVOKABLE void toggleNote(int digit);
    Q_INVOKABLE void toggleHighlight(int digit);
    Q_INVOKABLE void clearHighlight();
    Q_INVOKABLE void pressPad(int digit);
    Q_INVOKABLE void erase();
    Q_INVOKABLE void toggleNotesMode();
    Q_INVOKABLE void undo();
    Q_INVOKABLE void restart();
    Q_INVOKABLE void backToStart();

    Q_INVOKABLE QVariantMap windowGeometry() const;
    Q_INVOKABLE void saveWindowGeometry(int x, int y, int width, int height, bool maximized);

signals:
    void stateChanged();
    void boardChanged();
    void notesModeChanged();
    void checkAsYouGoChanged();
    void selectedIndexChanged();
    void selectedValueChanged();
    void highlightDigitChanged();
    void elapsedSecondsChanged();
    void hasSavedGameChanged();

private:
    void applyChange(const std::vector<int> &changed);
    void setState(State state);
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
    State m_state = Start;
    int m_selectedIndex = -1;
    int m_highlightDigit = -1;
    int m_elapsedSeconds = 0;
    bool m_notesMode = false;
    bool m_hasSavedGame = false;
};
