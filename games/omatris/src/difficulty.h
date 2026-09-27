#pragma once

#include <vector>

#include "board.h"

// How hard a Challenge board is to finish, from 1 (a clear or two away) to
// 100 (about to top out under buried holes). It reads the board the way a
// player sizes it up: the holes that have to be dug out, what is piled on top
// of them, how ragged the surface is, how close the stack is to the ceiling
// and how many dealt rows are still to go. A heuristic, not a solver: the
// same board always rates the same, and every feature only ever pushes it up.
namespace Difficulty {

constexpr int kMin = 1;
constexpr int kMax = 100;

// What the rating is made of, kept apart so tests can check each one.
struct Features {
    int holes = 0;       // empty cells with something above them in the column
    int cover = 0;       // filled cells stacked over those holes, summed
    int bumpiness = 0;   // height steps between neighbouring columns
    int height = 0;      // the tallest column, in visible rows
    int rowsLeft = 0;    // dealt rows still on the board
};

Features measure(const Board &board, const std::vector<int> &dealtRows);
int rate(const Features &features);
inline int rate(const Board &board, const std::vector<int> &dealtRows) {
    return rate(measure(board, dealtRows));
}

}  // namespace Difficulty
