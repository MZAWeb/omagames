#pragma once

#include "sudoku.h"

#include <array>

namespace SudokuGrader {

// Bit d-1 set = digit d. Shared by candidates, pencil marks and unit scans.
using Mask = quint16;
constexpr Mask kAllDigits = 0x1ff;
inline Mask bitOf(int digit) { return Mask(1u << (digit - 1)); }
int popCount(Mask mask);
// The digit of a single-bit mask, 0 when the mask holds none or several.
int soleDigit(Mask mask);

// A row, column or box as the nine indices it covers. Rows are units 0-8,
// columns 9-17, boxes 18-26, so a scan over all 27 needs no special cases.
using Unit = std::array<int, 9>;
constexpr int kUnits = 27;
const std::array<Unit, kUnits> &units();
inline const Unit &rowUnit(int row) { return units()[size_t(row)]; }
inline const Unit &columnUnit(int column) { return units()[size_t(9 + column)]; }
inline const Unit &boxUnit(int box) { return units()[size_t(18 + box)]; }
// Whether two distinct cells share a row, column or box.
bool sees(int a, int b);

// The pencil-mark view of a grid: every empty cell's remaining candidates,
// kept exact as digits are placed. The techniques only ever read it and
// narrow it; nothing here guesses.
class CandidateGrid {
public:
    explicit CandidateGrid(const Sudoku::Grid &grid);

    const Sudoku::Grid &values() const { return m_values; }
    int value(int index) const { return m_values[size_t(index)]; }
    Mask candidates(int index) const { return m_candidates[size_t(index)]; }
    bool has(int index, int digit) const { return m_candidates[size_t(index)] & bitOf(digit); }
    int emptyCount() const { return m_empty; }
    bool isSolved() const { return m_empty == 0; }
    // An empty cell with nothing left to try: the grid was contradictory.
    bool isStuck() const;

    void place(int index, int digit);
    // True when the digit really was a candidate, i.e. something changed.
    bool eliminate(int index, int digit) { return eliminate(index, bitOf(digit)); }
    bool eliminate(int index, Mask digits);

private:
    Sudoku::Grid m_values;
    std::array<Mask, Sudoku::kCells> m_candidates {};
    int m_empty = 0;
};

}
