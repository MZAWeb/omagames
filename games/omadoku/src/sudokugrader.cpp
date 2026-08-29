#include "sudokugrader.h"

#include <array>

using Sudoku::Grid;

namespace SudokuGrader {
namespace {

// Bit d-1 set = digit d is still possible in that cell.
using Candidates = std::array<quint16, Sudoku::kCells>;

constexpr quint16 kAllDigits = 0x1ff;

int singleDigit(quint16 mask) {
    for (int d = 1; d <= 9; ++d) {
        if (mask == quint16(1u << (d - 1)))
            return d;
    }
    return 0;
}

void place(Grid &grid, Candidates &candidates, int index, int digit) {
    grid[index] = digit;
    candidates[size_t(index)] = 0;
    const quint16 bit = quint16(1u << (digit - 1));
    for (int peer : Sudoku::peers(index))
        candidates[size_t(peer)] &= quint16(~bit);
}

// Indices of the 27 units (9 rows, 9 columns, 9 boxes).
const std::array<std::array<int, 9>, 27> &units() {
    static const auto table = [] {
        std::array<std::array<int, 9>, 27> all {};
        for (int i = 0; i < 9; ++i) {
            for (int j = 0; j < 9; ++j) {
                all[size_t(i)][size_t(j)] = i * 9 + j;                        // row i
                all[size_t(9 + i)][size_t(j)] = j * 9 + i;                    // column i
                all[size_t(18 + i)][size_t(j)] = (i / 3) * 27 + (i % 3) * 3 + (j / 3) * 9 + j % 3;
            }
        }
        return all;
    }();
    return table;
}

}  // namespace

bool solvableWithSingles(const Grid &grid) {
    Grid working = grid;
    Candidates candidates {};
    candidates.fill(kAllDigits);
    int empty = 0;
    for (int i = 0; i < Sudoku::kCells; ++i) {
        if (working[i] == 0)
            ++empty;
    }
    for (int i = 0; i < Sudoku::kCells; ++i) {
        if (working[i] != 0) {
            const int digit = working[i];
            working[i] = 0;
            place(working, candidates, i, digit);
        }
    }

    bool progress = true;
    while (empty > 0 && progress) {
        progress = false;
        for (int i = 0; i < Sudoku::kCells && !progress; ++i) {
            if (working[i] != 0)
                continue;
            if (const int digit = singleDigit(candidates[size_t(i)])) {
                place(working, candidates, i, digit);
                --empty;
                progress = true;
            }
        }
        if (progress)
            continue;
        for (const auto &unit : units()) {
            for (int d = 1; d <= 9 && !progress; ++d) {
                const quint16 bit = quint16(1u << (d - 1));
                int spot = -1;
                int count = 0;
                for (int index : unit) {
                    if (working[index] == d) {
                        count = 0;  // already placed in this unit
                        break;
                    }
                    if (candidates[size_t(index)] & bit) {
                        spot = index;
                        ++count;
                    }
                }
                if (count == 1) {
                    place(working, candidates, spot, d);
                    --empty;
                    progress = true;
                }
            }
            if (progress)
                break;
        }
    }
    return empty == 0;
}

}
