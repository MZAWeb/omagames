#include "candidategrid.h"

namespace SudokuGrader {

int popCount(Mask mask) {
    int n = 0;
    while (mask) {
        mask &= Mask(mask - 1);
        ++n;
    }
    return n;
}

int soleDigit(Mask mask) {
    for (int d = 1; d <= 9; ++d) {
        if (mask == bitOf(d))
            return d;
    }
    return 0;
}

const std::array<Unit, kUnits> &units() {
    static const auto table = [] {
        std::array<Unit, kUnits> all {};
        for (int i = 0; i < 9; ++i) {
            for (int j = 0; j < 9; ++j) {
                all[size_t(i)][size_t(j)] = i * 9 + j;
                all[size_t(9 + i)][size_t(j)] = j * 9 + i;
                all[size_t(18 + i)][size_t(j)] = (i / 3) * 27 + (i % 3) * 3 + (j / 3) * 9 + j % 3;
            }
        }
        return all;
    }();
    return table;
}

bool sees(int a, int b) {
    return a != b
        && (Sudoku::rowOf(a) == Sudoku::rowOf(b) || Sudoku::colOf(a) == Sudoku::colOf(b)
            || Sudoku::boxOf(a) == Sudoku::boxOf(b));
}

CandidateGrid::CandidateGrid(const Sudoku::Grid &grid) {
    m_values.fill(0);
    m_candidates.fill(kAllDigits);
    m_empty = Sudoku::kCells;
    for (int i = 0; i < Sudoku::kCells; ++i) {
        if (grid[size_t(i)] != 0)
            place(i, grid[size_t(i)]);
    }
}

bool CandidateGrid::isStuck() const {
    for (int i = 0; i < Sudoku::kCells; ++i) {
        if (m_values[size_t(i)] == 0 && m_candidates[size_t(i)] == 0)
            return true;
    }
    return false;
}

void CandidateGrid::place(int index, int digit) {
    if (m_values[size_t(index)] == 0)
        --m_empty;
    m_values[size_t(index)] = digit;
    m_candidates[size_t(index)] = 0;
    const Mask bit = bitOf(digit);
    for (int peer : Sudoku::peers(index))
        m_candidates[size_t(peer)] &= Mask(~bit);
}

bool CandidateGrid::eliminate(int index, Mask digits) {
    const Mask before = m_candidates[size_t(index)];
    m_candidates[size_t(index)] = Mask(before & ~digits);
    return m_candidates[size_t(index)] != before;
}

}
