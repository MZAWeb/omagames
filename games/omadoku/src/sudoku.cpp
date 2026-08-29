#include "sudoku.h"

namespace Sudoku {
namespace {

// Row/column/box candidate bitmasks (bit d-1 set = digit d already used).
struct Masks {
    std::array<quint16, 9> rows {};
    std::array<quint16, 9> cols {};
    std::array<quint16, 9> boxes {};

    void set(int index, int value) {
        const quint16 bit = quint16(1u << (value - 1));
        rows[rowOf(index)] |= bit;
        cols[colOf(index)] |= bit;
        boxes[boxOf(index)] |= bit;
    }
    void clear(int index, int value) {
        const quint16 bit = quint16(1u << (value - 1));
        rows[rowOf(index)] &= quint16(~bit);
        cols[colOf(index)] &= quint16(~bit);
        boxes[boxOf(index)] &= quint16(~bit);
    }
    quint16 candidates(int index) const {
        const quint16 used = quint16(rows[rowOf(index)] | cols[colOf(index)] | boxes[boxOf(index)]);
        return quint16(~used & 0x1ff);
    }
};

int popCount(quint16 mask) {
    int n = 0;
    while (mask) {
        mask &= quint16(mask - 1);
        ++n;
    }
    return n;
}

// Builds the masks for a grid; false when the grid already contains a conflict.
bool buildMasks(const Grid &grid, Masks &masks) {
    for (int i = 0; i < kCells; ++i) {
        const int value = grid[i];
        if (value == 0)
            continue;
        if (value < 1 || value > 9)
            return false;
        const quint16 bit = quint16(1u << (value - 1));
        if ((masks.rows[rowOf(i)] | masks.cols[colOf(i)] | masks.boxes[boxOf(i)]) & bit)
            return false;
        masks.set(i, value);
    }
    return true;
}

// Recursive search on the most-constrained empty cell, which keeps even the
// hardest generated puzzles well under a millisecond.
int search(Grid &grid, Masks &masks, int limit, Grid *firstSolution, int found) {
    int best = -1;
    quint16 bestCandidates = 0;
    int bestCount = 10;
    for (int i = 0; i < kCells; ++i) {
        if (grid[i] != 0)
            continue;
        const quint16 candidates = masks.candidates(i);
        const int count = popCount(candidates);
        if (count == 0)
            return found;  // dead end
        if (count < bestCount) {
            best = i;
            bestCandidates = candidates;
            bestCount = count;
            if (count == 1)
                break;
        }
    }
    if (best < 0) {
        if (firstSolution && found == 0)
            *firstSolution = grid;
        return found + 1;
    }

    for (int d = 1; d <= 9 && found < limit; ++d) {
        if (!(bestCandidates & quint16(1u << (d - 1))))
            continue;
        grid[best] = d;
        masks.set(best, d);
        found = search(grid, masks, limit, firstSolution, found);
        masks.clear(best, d);
        grid[best] = 0;
    }
    return found;
}

}  // namespace

Grid emptyGrid() {
    Grid grid {};
    grid.fill(0);
    return grid;
}

const std::vector<int> &peers(int index) {
    static const std::vector<std::vector<int>> table = [] {
        std::vector<std::vector<int>> all(kCells);
        for (int i = 0; i < kCells; ++i) {
            for (int j = 0; j < kCells; ++j) {
                if (i == j)
                    continue;
                if (rowOf(i) == rowOf(j) || colOf(i) == colOf(j) || boxOf(i) == boxOf(j))
                    all[i].push_back(j);
            }
        }
        return all;
    }();
    return table[index];
}

bool isValidPlacement(const Grid &grid, int index, int value) {
    if (index < 0 || index >= kCells || value < 1 || value > 9)
        return false;
    for (int peer : peers(index)) {
        if (grid[peer] == value)
            return false;
    }
    return true;
}

bool isComplete(const Grid &grid) {
    for (int i = 0; i < kCells; ++i) {
        if (grid[i] == 0)
            return false;
        if (!isValidPlacement(grid, i, grid[i]))
            return false;
    }
    return true;
}

int countSolutions(const Grid &grid, int limit, Grid *firstSolution) {
    if (limit <= 0)
        return 0;
    Masks masks;
    if (!buildMasks(grid, masks))
        return 0;
    Grid working = grid;
    return search(working, masks, limit, firstSolution, 0);
}

bool solve(Grid &inOut) {
    Grid solution {};
    if (countSolutions(inOut, 1, &solution) == 0)
        return false;
    inOut = solution;
    return true;
}

std::vector<int> wrongCells(const Grid &current, const Grid &solution) {
    std::vector<int> wrong;
    for (int i = 0; i < kCells; ++i) {
        if (current[i] != 0 && current[i] != solution[i])
            wrong.push_back(i);
    }
    return wrong;
}

}
