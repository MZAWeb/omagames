#pragma once

#include <QString>

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
