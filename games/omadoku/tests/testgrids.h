#pragma once

#include <QString>

#include "candidategrid.h"
#include "sudoku.h"

// Grids written as 81 characters ('.' or '0' = empty) keep the fixtures in the
// tests readable.
inline Sudoku::Grid gridFromString(const QString &text) {
    Sudoku::Grid grid = Sudoku::emptyGrid();
    int index = 0;
    for (QChar ch : text) {
        if (ch.isSpace())
            continue;
        if (index >= Sudoku::kCells)
            break;
        grid[index++] = (ch == QLatin1Char('.') || ch == QLatin1Char('0')) ? 0 : ch.digitValue();
    }
    return grid;
}

// The soundness invariant of the technique solver: every placed digit is the
// solution's, and every empty cell still lists the solution's digit.
inline bool agreesWithSolution(const SudokuGrader::CandidateGrid &grid, const Sudoku::Grid &solution) {
    for (int i = 0; i < Sudoku::kCells; ++i) {
        if (grid.value(i) != 0 ? grid.value(i) != solution[size_t(i)] : !grid.has(i, solution[size_t(i)]))
            return false;
    }
    return true;
}
