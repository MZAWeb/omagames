#pragma once

#include <QJsonObject>

#include <array>
#include <deque>
#include <vector>

#include "sudokugenerator.h"

// The playable state of one puzzle: entered values, pencil marks, wrong-cell
// flags and undo history. Deliberately free of QObject so every rule below is
// testable headlessly; SudokuGame only wraps it for QML.
class SudokuBoard {
public:
    using Notes = std::array<quint16, Sudoku::kCells>;  // bit d-1 set = note d shown

    void setPuzzle(const Puzzle &puzzle);
    const Puzzle &puzzle() const { return m_puzzle; }

    bool isGiven(int index) const;
    int value(int index) const;
    quint16 notes(int index) const;
    bool isWrong(int index) const;

    // The first cell with no value in it, or -1 when the grid is full: where a
    // puzzle puts the cursor when it opens.
    int firstEmptyIndex() const;
    int filledCount() const;
    int entryCount() const;  // cells the player filled in or pencilled
    int digitCount(int digit) const;  // placed digits, givens included
    bool isSolved() const;

    // Off means "validate when full": nothing is flagged until all 81 cells
    // hold a value, then every mistake shows at once. Flipping it mid-game
    // re-evaluates every cell, so marks appear or vanish immediately.
    bool validateAsYouGo() const { return m_validateAsYouGo; }
    void setValidateAsYouGo(bool validateAsYouGo);

    // With Auto-notes on the pencil marks stop being the player's: every empty
    // cell shows the digits its row, column and box still leave open, worked
    // out from the grid on every read so they can never fall out of date. The
    // marks the player made are kept untouched underneath and come back the
    // moment it goes off — turning it on borrows the notes, it never spends
    // them. While it is on toggleNotes() does nothing: there is no pencil to
    // pick up.
    bool autoNotes() const { return m_autoNotes; }
    void setAutoNotes(bool autoNotes) { m_autoNotes = autoNotes; }
    // The digits `index` still allows, given every value on the board. Empty
    // cells only; a contradiction leaves nothing open, and says so.
    quint16 candidates(int index) const;

    // Mutations return the cells they touched so the QML model can refresh
    // exactly those rows. Givens and out-of-range indices are ignored.
    std::vector<int> setValue(int index, int value);
    // Pencilling one digit across several cells is one act and one undo step.
    // Cells that already hold a value are skipped, not emptied. The empty ones
    // move together: the note is added to all of them unless every one already
    // has it, in which case it is removed from all. Over a single cell that is
    // the plain toggle it has always been.
    std::vector<int> toggleNotes(const std::vector<int> &indices, int digit);
    std::vector<int> toggleNote(int index, int digit) { return toggleNotes({index}, digit); }
    std::vector<int> erase(int index);
    std::vector<int> undo();
    std::vector<int> restart();  // clears every non-given cell and the history
    bool canUndo() const { return !m_undo.empty(); }

    QJsonObject toJson() const;
    static bool fromJson(const QJsonObject &json, SudokuBoard *board);

private:
    struct CellState {
        int index = 0;
        int value = 0;
        quint16 notes = 0;
    };

    void pushUndo(const std::vector<int> &indices);
    void applyState(const CellState &state);
    void refreshWrong();

    Puzzle m_puzzle;
    Sudoku::Grid m_values = Sudoku::emptyGrid();
    Notes m_notes {};
    std::array<bool, Sudoku::kCells> m_wrong {};
    std::deque<std::vector<CellState>> m_undo;
    bool m_validateAsYouGo = true;
    bool m_autoNotes = false;
};
