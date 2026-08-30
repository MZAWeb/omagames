#include "sudokugenerator.h"

#include <QRandomGenerator>

#include "sudokugrader.h"

#include <algorithm>
#include <array>

using Sudoku::Grid;
using SudokuGrader::Technique;

namespace {

// Fisher-Yates with the puzzle's own generator so a seed fully determines the
// puzzle.
template <typename T>
void shuffle(std::vector<T> &items, QRandomGenerator &rng) {
    for (int i = int(items.size()) - 1; i > 0; --i)
        std::swap(items[size_t(i)], items[rng.bounded(i + 1)]);
}

// Randomized backtracking fill of an empty grid: a uniformly random-looking
// complete solution to carve the puzzle out of.
bool fillGrid(Grid &grid, int index, QRandomGenerator &rng) {
    if (index >= Sudoku::kCells)
        return true;
    std::vector<int> digits {1, 2, 3, 4, 5, 6, 7, 8, 9};
    shuffle(digits, rng);
    for (int digit : digits) {
        if (!Sudoku::isValidPlacement(grid, index, digit))
            continue;
        grid[size_t(index)] = digit;
        if (fillGrid(grid, index + 1, rng))
            return true;
        grid[size_t(index)] = 0;
    }
    return false;
}

// Puts one clue back so that a puzzle just past the level's ceiling drops
// into the level. Removing a clue can jump from "too easy" straight over a
// narrow band; refilling a different cell finds the state in between.
bool refillIntoLevel(Grid &givens, const Grid &solution, const std::vector<int> &order,
                     Difficulty difficulty) {
    for (int index : order) {
        if (givens[size_t(index)] != 0)
            continue;
        givens[size_t(index)] = solution[size_t(index)];
        if (SudokuGenerator::meetsLevel(givens, difficulty))
            return true;
        givens[size_t(index)] = 0;
    }
    return false;
}

// One carving pass. A clue only goes when the puzzle stays unique and stays
// within the level's ceiling, so the ladder can always finish what is left.
// Past the target it keeps carving until the level is met: fewer clues is
// what pushes a puzzle up the ladder. A removal that would overshoot the
// ceiling is the last resort, paired with a refill to land in the band.
Grid carve(const Grid &solution, const std::vector<int> &order, int target, Difficulty difficulty) {
    const Technique ceiling = SudokuGenerator::ceiling(difficulty);
    Grid givens = solution;
    int clues = Sudoku::kCells;
    for (int index : order) {
        if (clues <= target && SudokuGenerator::meetsLevel(givens, difficulty))
            break;
        const int removed = givens[size_t(index)];
        givens[size_t(index)] = 0;
        if (Sudoku::countSolutions(givens, 2) != 1) {
            givens[size_t(index)] = removed;
        } else if (SudokuGrader::solvableWith(givens, ceiling)) {
            --clues;
        } else if (clues <= target && refillIntoLevel(givens, solution, order, difficulty)) {
            break;
        } else {
            givens[size_t(index)] = removed;
        }
    }
    return givens;
}

}  // namespace

Technique SudokuGenerator::ceiling(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Easy:
        return Technique::NakedSingle;
    case Difficulty::Medium:
        return Technique::NakedPair;
    case Difficulty::Hard:
        return Technique::HiddenTriple;
    case Difficulty::ExtraHard:
        break;
    }
    return Technique::XYWing;
}

bool SudokuGenerator::meetsLevel(const Grid &givens, Difficulty difficulty) {
    if (!SudokuGrader::solvableWith(givens, ceiling(difficulty)))
        return false;
    if (difficulty == Difficulty::Easy)
        return true;
    const Difficulty below = Difficulty(int(difficulty) - 1);
    return !SudokuGrader::solvableWith(givens, ceiling(below));
}

int SudokuGenerator::minClues(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Easy:
        return 38;
    case Difficulty::Medium:
        return 32;
    case Difficulty::Hard:
        return 26;
    case Difficulty::ExtraHard:
        break;
    }
    return 24;
}

int SudokuGenerator::maxClues(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Easy:
        return 42;
    case Difficulty::Medium:
        return 36;
    case Difficulty::Hard:
        return 30;
    case Difficulty::ExtraHard:
        break;
    }
    return 28;
}

Puzzle SudokuGenerator::generate(Difficulty difficulty, quint32 seed) {
    if (seed == 0)
        seed = QRandomGenerator::global()->generate();
    QRandomGenerator rng(seed);

    Puzzle puzzle;
    puzzle.difficulty = difficulty;
    puzzle.seed = seed;
    fillGrid(puzzle.solution, 0, rng);

    const int target = minClues(difficulty)
        + int(rng.bounded(maxClues(difficulty) - minClues(difficulty) + 1));
    std::vector<int> order(Sudoku::kCells);
    for (int i = 0; i < Sudoku::kCells; ++i)
        order[size_t(i)] = i;

    // A removal order that never reaches the level is retried with a fresh
    // shuffle, a bounded number of times. Should every attempt fall short
    // the first carve is kept: it is within the ceiling, only easier than
    // promised, and `hardest` tells the truth about it.
    constexpr int kMaxAttempts = 200;
    for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
        shuffle(order, rng);
        const Grid givens = carve(puzzle.solution, order, target, difficulty);
        if (attempt == 0)
            puzzle.givens = givens;
        if (meetsLevel(givens, difficulty)) {
            puzzle.givens = givens;
            break;
        }
    }
    puzzle.hardest = SudokuGrader::grade(puzzle.givens).hardest;
    return puzzle;
}
