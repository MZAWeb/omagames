#include "sudokutechniques.h"

// The techniques that reach across the grid: fish (X-wing, swordfish) on one
// digit's rows and columns, and the Y-wing on three bi-value cells.
namespace SudokuGrader {
namespace {

// Where `digit` can still go on line `i`, as a mask over the crossing lines:
// column positions along a row, row positions down a column.
Mask lineSpots(const CandidateGrid &grid, bool byRow, int i, int digit) {
    const Unit &unit = byRow ? rowUnit(i) : columnUnit(i);
    Mask spots = 0;
    for (int slot = 0; slot < 9; ++slot) {
        if (grid.has(unit[size_t(slot)], digit))
            spots |= Mask(1u << slot);
    }
    return spots;
}

// A fish: `digit` confined, on the `base` lines, to the `cover` crossing
// lines. It must land in one cover line per base line, which uses up all of
// them, so every other cell on the cover lines loses the digit.
bool eliminateFish(CandidateGrid &grid, bool byRow, Mask base, Mask cover, int digit) {
    bool changed = false;
    for (int c = 0; c < 9; ++c) {
        if (!(cover & Mask(1u << c)))
            continue;
        const Unit &unit = byRow ? columnUnit(c) : rowUnit(c);
        for (int slot = 0; slot < 9; ++slot) {
            if (!(base & Mask(1u << slot)))
                changed |= grid.eliminate(unit[size_t(slot)], digit);
        }
    }
    return changed;
}

bool xWingFor(CandidateGrid &grid, bool byRow, int digit) {
    std::array<Mask, 9> spots {};
    for (int i = 0; i < 9; ++i)
        spots[size_t(i)] = lineSpots(grid, byRow, i, digit);
    for (int i = 0; i < 9; ++i) {
        if (popCount(spots[size_t(i)]) != 2)
            continue;
        for (int j = i + 1; j < 9; ++j) {
            if (spots[size_t(j)] != spots[size_t(i)])
                continue;
            if (eliminateFish(grid, byRow, Mask((1u << i) | (1u << j)), spots[size_t(i)], digit))
                return true;
        }
    }
    return false;
}

bool swordfishFor(CandidateGrid &grid, bool byRow, int digit) {
    std::array<Mask, 9> spots {};
    for (int i = 0; i < 9; ++i) {
        const Mask mask = lineSpots(grid, byRow, i, digit);
        spots[size_t(i)] = (popCount(mask) == 2 || popCount(mask) == 3) ? mask : 0;
    }
    for (int i = 0; i < 9; ++i) {
        if (!spots[size_t(i)])
            continue;
        for (int j = i + 1; j < 9; ++j) {
            if (!spots[size_t(j)])
                continue;
            for (int k = j + 1; k < 9; ++k) {
                if (!spots[size_t(k)])
                    continue;
                const Mask cover = Mask(spots[size_t(i)] | spots[size_t(j)] | spots[size_t(k)]);
                if (popCount(cover) != 3)
                    continue;
                const Mask base = Mask((1u << i) | (1u << j) | (1u << k));
                if (eliminateFish(grid, byRow, base, cover, digit))
                    return true;
            }
        }
    }
    return false;
}

}  // namespace

bool xWing(CandidateGrid &grid) {
    for (int d = 1; d <= 9; ++d) {
        if (xWingFor(grid, true, d) || xWingFor(grid, false, d))
            return true;
    }
    return false;
}

bool swordfish(CandidateGrid &grid) {
    for (int d = 1; d <= 9; ++d) {
        if (swordfishFor(grid, true, d) || swordfishFor(grid, false, d))
            return true;
    }
    return false;
}

bool yWing(CandidateGrid &grid) {
    for (int pivot = 0; pivot < Sudoku::kCells; ++pivot) {
        const Mask ab = grid.candidates(pivot);
        if (popCount(ab) != 2)
            continue;
        const std::vector<int> &peers = Sudoku::peers(pivot);
        for (int wing1 : peers) {
            const Mask ac = grid.candidates(wing1);
            // A wing shares exactly one digit with the pivot and brings one of
            // its own, which is the digit the pattern eliminates.
            if (popCount(ac) != 2 || ac == ab || popCount(Mask(ac & ab)) != 1)
                continue;
            const Mask c = Mask(ac & ~ab);
            const Mask bc = Mask((ab & ~ac) | c);
            for (int wing2 : peers) {
                if (grid.candidates(wing2) != bc)
                    continue;
                // Whichever way the pivot goes, one wing is forced to C, so
                // any cell seeing both wings cannot be C.
                bool changed = false;
                for (int cell = 0; cell < Sudoku::kCells; ++cell) {
                    if (cell == wing1 || cell == wing2 || cell == pivot)
                        continue;
                    if (sees(cell, wing1) && sees(cell, wing2))
                        changed |= grid.eliminate(cell, c);
                }
                if (changed)
                    return true;
            }
        }
    }
    return false;
}

}
