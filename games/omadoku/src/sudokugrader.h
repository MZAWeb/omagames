#pragma once

#include <optional>

#include "sudokutechniques.h"

// Grades a puzzle the way a player experiences it: climbing the technique
// ladder from the easiest rung, restarting from the bottom after every
// step, and noting the hardest rung it ever needed. Nothing guesses, so a
// grid the ladder cannot finish is simply beyond the given ceiling.
namespace SudokuGrader {

struct Grading {
    bool solved = false;
    // The hardest rung climbed; only meaningful when solved. A grid that is
    // already complete needed nothing, which reads as the lowest rung.
    Technique hardest = Technique::NakedSingle;
};

// One step: the easiest technique up to `ceiling` (inclusive) that makes
// progress, or nothing when the ladder is stuck.
std::optional<Technique> step(CandidateGrid &grid, Technique ceiling);

Grading grade(const Sudoku::Grid &grid, Technique ceiling = kHardestTechnique);
bool solvableWith(const Sudoku::Grid &grid, Technique ceiling);

}
