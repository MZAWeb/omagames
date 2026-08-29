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
// Screen state, difficulty and pad mode cross to QML as lowercase id strings
// rather than as enums: a C++-registered QML type would need a module for QML
// tooling to resolve, and plain strings keep both sides (and qmllint) honest
// without one. The matching display labels come from `difficulties`.
class SudokuGame : public QObject {
    Q_OBJECT
    Q_PROPERTY(QAbstractListModel *cells READ cells CONSTANT)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString difficulty READ difficulty NOTIFY boardChanged)
    Q_PROPERTY(QString difficultyLabel READ difficultyLabel NOTIFY boardChanged)
    Q_PROPERTY(QVariantList difficulties READ difficulties CONSTANT)
    Q_PROPERTY(QString padMode READ padMode WRITE setPadMode NOTIFY padModeChanged)
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
    // "easy" | "medium" | "hard", plus the label to show for it and the full
    // list of {id, label} pairs the start screen offers.
    QString difficulty() const;
    QString difficultyLabel() const;
    static QVariantList difficulties();
    // What a digit does when no modifier overrides it: "highlight" | "note" |
    // "fill". Applies to the keypad and to the plain number keys alike.
    QString padMode() const;
    void setPadMode(const QString &padMode);
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

    Q_INVOKABLE void newGame(const QString &difficulty);
    Q_INVOKABLE void resumeSavedGame();
    Q_INVOKABLE void select(int index);
    Q_INVOKABLE void moveSelection(int deltaRow, int deltaColumn);
    // The one entry point for a digit, from the keypad or the number row:
    // `overrideMode` is a mode id when a modifier asked for a specific action
    // (Ctrl fills, Shift notes, Alt highlights) and empty to follow padMode.
    Q_INVOKABLE void pressDigit(int digit, const QString &overrideMode = QString());
    Q_INVOKABLE void enterValue(int digit);
    Q_INVOKABLE void toggleNote(int digit);
    Q_INVOKABLE void toggleHighlight(int digit);
    Q_INVOKABLE void clearHighlight();
    Q_INVOKABLE void cyclePadMode();
    Q_INVOKABLE void erase();
    Q_INVOKABLE void undo();
    Q_INVOKABLE void restart();
    Q_INVOKABLE void backToStart();

    Q_INVOKABLE QVariantMap windowGeometry() const;
    Q_INVOKABLE void saveWindowGeometry(int x, int y, int width, int height, bool maximized);

signals:
    void stateChanged();
    void boardChanged();
    void padModeChanged();
    void validateAsYouGoChanged();
    void selectedIndexChanged();
    void selectedValueChanged();
    void highlightDigitChanged();
    void elapsedSecondsChanged();
    void hasSavedGameChanged();

private:
    enum class Screen { Start, Playing, Won };

    // What a click on the digit pad does, mirroring the modifier-free, Shift
    // and Ctrl meanings of the number row.
    enum class PadMode { Highlight, Note, Fill };

    static PadMode modeFromId(const QString &id, PadMode fallback);
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
    PadMode m_padMode = PadMode::Highlight;
    bool m_hasSavedGame = false;
};
