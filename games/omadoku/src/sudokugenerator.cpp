#include "sudokugenerator.h"

#include <QRandomGenerator>

#include "sudokugrader.h"

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

namespace {

// One carving pass: removes clues in `order` while the puzzle stays unique
// (and, for Easy, stays solvable by singles). Hard keeps carving past the
// target while singles still crack it, but never below the difficulty floor.
Sudoku::Grid carve(const Grid &solution, const std::vector<int> &order, int target, int floorClues,
                   bool requireSingles, bool forbidSingles) {
    Grid givens = solution;
    int clues = Sudoku::kCells;
    for (int index : order) {
        if (clues <= floorClues)
            break;
        if (clues <= target && !(forbidSingles && SudokuGrader::solvableWithSingles(givens)))
            break;
        const int removed = givens[index];
        givens[index] = 0;
        const bool unique = Sudoku::countSolutions(givens, 2) == 1;
        if (unique && (!requireSingles || SudokuGrader::solvableWithSingles(givens)))
            --clues;
        else
            givens[index] = removed;
    }
    return givens;
}

}  // namespace

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

    // Easy must stay solvable by singles alone; Hard must need more than that.
    // A removal order that cannot deliver a hard-enough puzzle is retried with
    // a fresh shuffle a bounded number of times - we keep the first attempt as
    // a fallback rather than ever looping forever.
    const bool requireSingles = difficulty == Difficulty::Easy;
    const bool forbidSingles = difficulty == Difficulty::Hard;
    constexpr int kMaxAttempts = 12;
    for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
        shuffle(order, rng);
        const Grid givens = carve(puzzle.solution, order, target, minClues(difficulty),
                                  requireSingles, forbidSingles);
        if (attempt == 0)
            puzzle.givens = givens;
        if (!forbidSingles || !SudokuGrader::solvableWithSingles(givens)) {
            puzzle.givens = givens;
            break;
        }
    }
    return puzzle;
}
