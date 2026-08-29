#pragma once

#include "sudoku.h"

// A minimal difficulty yardstick: whether a puzzle falls to the two easiest
// human techniques. The generator uses it so "Easy" never needs guessing and
// "Hard" always needs more than singles.
namespace SudokuGrader {

// Repeatedly places naked singles (one candidate left in a cell) and hidden
// singles (one place left in a row/column/box for a digit). True when that
// alone completes the grid.
bool solvableWithSingles(const Sudoku::Grid &grid);

}
