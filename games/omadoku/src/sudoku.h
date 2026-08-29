#pragma once

#include <QtGlobal>

#include <array>
#include <vector>

// Core Sudoku rules and solving. Deliberately QtCore-only (in fact almost
// dependency free) so the whole rule set is unit-testable headlessly and the
// UI can be replaced without touching any of this.
namespace Sudoku {

// 0 = empty, row-major: index = row * 9 + col.
using Grid = std::array<int, 81>;

inline constexpr int kSize = 9;
inline constexpr int kCells = 81;

Grid emptyGrid();

inline int rowOf(int index) { return index / kSize; }
inline int colOf(int index) { return index % kSize; }
inline int boxOf(int index) { return (rowOf(index) / 3) * 3 + colOf(index) / 3; }

// The 20 cells sharing a row, column or box with `index` (never `index` itself).
const std::vector<int> &peers(int index);

// True when `value` (1-9) can go in `index` without clashing with the row,
// column or box. The cell's own current value is ignored.
bool isValidPlacement(const Grid &grid, int index, int value);

// Every cell filled and no conflicts.
bool isComplete(const Grid &grid);

// Backtracking solver counting solutions up to `limit` (2 is enough to prove
// uniqueness). Writes the first solution found when `firstSolution` is given.
int countSolutions(const Grid &grid, int limit, Grid *firstSolution = nullptr);

// Solves in place; false (and `inOut` untouched) when there is no solution.
bool solve(Grid &inOut);

// Filled cells of `current` that disagree with `solution`, in index order.
std::vector<int> wrongCells(const Grid &current, const Grid &solution);

}
