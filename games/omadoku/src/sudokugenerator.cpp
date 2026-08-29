#include "sudokugenerator.h"

#include <QRandomGenerator>

#include <algorithm>
#include <array>

using Sudoku::Grid;

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
        grid[index] = digit;
        if (fillGrid(grid, index + 1, rng))
            return true;
        grid[index] = 0;
    }
    return false;
}

}  // namespace

int SudokuGenerator::minClues(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Easy:
        return 38;
    case Difficulty::Medium:
        return 30;
    case Difficulty::Hard:
        return 24;
    }
    return 24;
}

int SudokuGenerator::maxClues(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Easy:
        return 42;
    case Difficulty::Medium:
        return 34;
    case Difficulty::Hard:
        return 28;
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

    // Carve clues away in random order, keeping only removals that leave the
    // puzzle unique. Stop as soon as we are inside the difficulty's window.
    const int target = minClues(difficulty) + int(rng.bounded(maxClues(difficulty) - minClues(difficulty) + 1));
    std::vector<int> order(Sudoku::kCells);
    for (int i = 0; i < Sudoku::kCells; ++i)
        order[size_t(i)] = i;
    shuffle(order, rng);

    puzzle.givens = puzzle.solution;
    int clues = Sudoku::kCells;
    for (int index : order) {
        if (clues <= target)
            break;
        const int removed = puzzle.givens[index];
        puzzle.givens[index] = 0;
        if (Sudoku::countSolutions(puzzle.givens, 2) == 1)
            --clues;
        else
            puzzle.givens[index] = removed;
    }
    return puzzle;
}
