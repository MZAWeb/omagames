#pragma once

#include <vector>

#include "board.h"

// The mess a Challenge starts from: a stack a careless player could really
// have built, left behind for the player to clean up. Every cell of it is a
// whole tetromino dropped straight down from above, so nothing floats and the
// colours are the pieces' own; only the choice of where each one fell is bad.
namespace Challenge {

// Between a fifth and a half of the visible well holds part of the stack.
constexpr int kMinRows = Board::kVisibleHeight / 5;
constexpr int kMaxRows = Board::kVisibleHeight / 2;

// Fills an empty board from `seed` and returns the rows the stack covers, top
// to bottom: always kMinRows..kMaxRows of them, all touching the floor, and
// none of them full.
std::vector<int> build(Board &board, quint32 seed);
// Where the rows in `tracked` are once `cleared` have left the board: the
// cleared ones are gone and every row above a cleared one fell by one.
std::vector<int> afterClear(const std::vector<int> &tracked, const std::vector<int> &cleared);

}  // namespace Challenge
