#include "sudokugrader.h"

namespace SudokuGrader {

std::optional<Technique> step(CandidateGrid &grid, Technique ceiling) {
    for (int t = 0; t <= int(ceiling); ++t) {
        if (apply(Technique(t), grid))
            return Technique(t);
    }
    return std::nullopt;
}

Grading grade(const Sudoku::Grid &grid, Technique ceiling) {
    CandidateGrid candidates(grid);
    Grading grading;
    while (!candidates.isSolved()) {
        const std::optional<Technique> used = step(candidates, ceiling);
        if (!used)
            break;
        grading.hardest = std::max(grading.hardest, *used);
    }
    grading.solved = candidates.isSolved();
    return grading;
}

bool solvableWith(const Sudoku::Grid &grid, Technique ceiling) {
    return grade(grid, ceiling).solved;
}

}
