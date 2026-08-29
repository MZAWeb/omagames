#pragma once

#include <QtGlobal>

#include "sudoku.h"

enum class Difficulty { Easy, Medium, Hard };

struct Puzzle {
    Sudoku::Grid givens = Sudoku::emptyGrid();
    Sudoku::Grid solution = Sudoku::emptyGrid();
    Difficulty difficulty = Difficulty::Easy;
    quint32 seed = 0;
};

// Generates puzzles with exactly one solution. Deterministic for a given
// non-zero seed, which is what the tests rely on.
class SudokuGenerator {
public:
    static Puzzle generate(Difficulty difficulty, quint32 seed = 0);

    // Inclusive clue-count target for a difficulty; generation gets as close as
    // uniqueness allows and never loops forever trying.
    static int minClues(Difficulty difficulty);
    static int maxClues(Difficulty difficulty);
};
