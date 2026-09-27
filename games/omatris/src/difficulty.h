#pragma once

#include <vector>

#include "board.h"

// How hard a Challenge board is to finish, from 1 (a clear or two away) to
// 100 (hopeless). It estimates the work left rather than judging the board's
// looks: which rows have to be cleared before the dealt ones are gone, how
// many cells in them still need filling, and how many of those cells are
// buried where no piece can reach them yet. Building on top of the mess
// without covering a gap costs nothing; burying a gap costs a lot.
namespace Difficulty {

constexpr int kMin = 1;
constexpr int kMax = 100;

// What the rating is made of, kept apart so tests can check each one.
struct Features {
    // The dealt rows still on the board, plus every row that has a block over
    // an empty cell of one of those: it has to be cleared before that cell
    // can be filled, which makes it a row to clear too.
    int rowsToClear = 0;
    // Empty cells across those rows: what the pieces still have to fill.
    int cellsToFill = 0;
    // The ones among them with a block somewhere above: dig first, then fill.
    int buried = 0;
    int height = 0;  // the tallest column, in rows
};

Features measure(const Board &board, const std::vector<int> &dealtRows);
int rate(const Features &features);
inline int rate(const Board &board, const std::vector<int> &dealtRows) {
    return rate(measure(board, dealtRows));
}

}  // namespace Difficulty
