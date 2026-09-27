#pragma once

#include <vector>

#include "board.h"

// A Challenge's mess once it is on the board: which rows of it are left, where
// they have fallen to, and how hard the board around them rates. The game
// owns one only in Challenge and tells it when rows clear and when the board
// has settled; it never touches the board itself after the deal.
class DealtStack {
public:
    // Deals the mess from `seed` onto an empty board.
    DealtStack(Board &board, quint32 seed);

    // How many rows the deal covered.
    int dealt() const { return m_dealt; }
    // The board rows that still belong to it, rows mid-clear included.
    const std::vector<int> &rows() const { return m_rows; }
    // The rows left once those in `clearing` are counted gone.
    int rowsLeft(const std::vector<int> &clearing) const;
    // Difficulty::rate() as of the last rate().
    int difficulty() const { return m_difficulty; }

    // `cleared` have left the board; the rest fall past them.
    void clear(const std::vector<int> &cleared);
    void rate(const Board &board);

private:
    std::vector<int> m_rows;
    int m_dealt = 0;
    int m_difficulty = 0;
};
