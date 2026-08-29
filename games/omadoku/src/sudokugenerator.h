#pragma once

#include <QtGlobal>

#include "sudoku.h"
#include "sudokutechniques.h"

// Levels are defined by the human techniques they need, not by clue counts:
// see SudokuGenerator::ceiling.
enum class Difficulty { Easy, Medium, Hard, ExtraHard };
constexpr int kDifficultyCount = 4;

struct Puzzle {
    Sudoku::Grid givens = Sudoku::emptyGrid();
    Sudoku::Grid solution = Sudoku::emptyGrid();
    Difficulty difficulty = Difficulty::Easy;
    quint32 seed = 0;
    // The hardest rung of the ladder the puzzle actually needs, as graded at
    // generation, so what the player is told is what they will meet.
    SudokuGrader::Technique hardest = SudokuGrader::Technique::NakedSingle;
};

// Generates puzzles with exactly one solution. Deterministic for a given
// non-zero seed, which is what the tests rely on.
class SudokuGenerator {
public:
    static Puzzle generate(Difficulty difficulty, quint32 seed = 0);

    // The hardest technique a level may need. Every puzzle of a level is
    // solvable with the ladder up to its ceiling, and (Easy aside) not with
    // the ladder up to the previous level's, so each level asks for something
    // the one below never does.
    static SudokuGrader::Technique ceiling(Difficulty difficulty);
    static bool meetsLevel(const Sudoku::Grid &givens, Difficulty difficulty);

    // Clue-count target for a level. It is where carving would like to stop;
    // the technique requirement decides where it actually does.
    static int minClues(Difficulty difficulty);
    static int maxClues(Difficulty difficulty);
};
