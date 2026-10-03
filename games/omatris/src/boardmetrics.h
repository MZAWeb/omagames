#pragma once

#include <array>

#include "board.h"

// The shape of a stack in the numbers Tetris players and bots judge it by.
// The rules never look at these; an agent's reward and features do, and they
// are cheaper counted here once than in Python every step.
struct BoardMetrics {
    // Rows a column's stack rises above the floor, hidden rows included; 0
    // for an empty column.
    std::array<int, size_t(Board::kWidth)> heights {};
    int maxHeight = 0;
    // Empty cells with a filled cell somewhere above them in their column.
    int holes = 0;
    // How jagged the surface is: the height differences between neighbouring
    // columns, summed.
    int bumpiness = 0;

    static BoardMetrics of(const Board &board);
};
