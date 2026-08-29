#include "solvertests.h"

#include <QtTest>

#include "sudoku.h"
#include "testgrids.h"

using Sudoku::Grid;

namespace {

// A classic "hardest for brute force" puzzle with exactly one solution.
const QString kPuzzle = QStringLiteral(
    "53..7...."
    "6..195..."
    ".98....6."
    "8...6...3"
    "4..8.3..1"
    "7...2...6"
    ".6....28."
    "...419..5"
    "....8..79");

const QString kSolution = QStringLiteral(
    "534678912"
    "672195348"
    "198342567"
    "859761423"
    "426853791"
    "713924856"
    "961537284"
    "287419635"
    "345286179");

}  // namespace

void SolverTests::placementRejectsRowColumnAndBoxConflicts() {
    Grid grid = Sudoku::emptyGrid();
    grid[0] = 5;   // r0c0
    QVERIFY(!Sudoku::isValidPlacement(grid, 4, 5));    // same row
    QVERIFY(!Sudoku::isValidPlacement(grid, 36, 5));   // same column
    QVERIFY(!Sudoku::isValidPlacement(grid, 10, 5));   // same box
    QVERIFY(Sudoku::isValidPlacement(grid, 10, 6));
    QVERIFY(Sudoku::isValidPlacement(grid, 40, 5));    // unrelated cell
    QVERIFY(!Sudoku::isValidPlacement(grid, 40, 0));   // 0 is not a digit
    QVERIFY(!Sudoku::isValidPlacement(grid, -1, 3));
}

void SolverTests::completeGridIsRecognised() {
    const Grid solved = gridFromString(kSolution);
    QVERIFY(Sudoku::isComplete(solved));

    Grid missing = solved;
    missing[80] = 0;
    QVERIFY(!Sudoku::isComplete(missing));

    Grid duplicated = solved;
    duplicated[1] = duplicated[0];
    QVERIFY(!Sudoku::isComplete(duplicated));
}

void SolverTests::solvesAKnownPuzzle() {
    Grid grid = gridFromString(kPuzzle);
    QVERIFY(Sudoku::solve(grid));
    QCOMPARE(grid, gridFromString(kSolution));
}

void SolverTests::countsSolutionsUpToLimit() {
    const Grid puzzle = gridFromString(kPuzzle);
    Grid first = Sudoku::emptyGrid();
    QCOMPARE(Sudoku::countSolutions(puzzle, 2, &first), 1);
    QCOMPARE(first, gridFromString(kSolution));

    // Blanking a "deadly rectangle" (two rows x two columns of one box stack
    // holding the same two digits crosswise) always leaves exactly two
    // solutions: the pair can be swapped freely.
    Grid ambiguous = gridFromString(kSolution);
    for (int index : {3, 4, 30, 31})
        ambiguous[index] = 0;
    QCOMPARE(Sudoku::countSolutions(ambiguous, 5), 2);

    // The limit caps the search.
    QCOMPARE(Sudoku::countSolutions(Sudoku::emptyGrid(), 3), 3);
    QCOMPARE(Sudoku::countSolutions(puzzle, 0), 0);
}

void SolverTests::reportsNoSolutionForContradictoryGrid() {
    Grid grid = Sudoku::emptyGrid();
    grid[0] = 1;
    grid[1] = 1;  // duplicate in the first row
    QCOMPARE(Sudoku::countSolutions(grid, 2), 0);
    QVERIFY(!Sudoku::solve(grid));
    QCOMPARE(grid[0], 1);  // left untouched
}

void SolverTests::wrongCellsIgnoresEmptyCells() {
    const Grid solution = gridFromString(kSolution);
    Grid current = Sudoku::emptyGrid();
    current[3] = solution[3];
    current[4] = solution[4] % 9 + 1;  // guaranteed different digit
    current[80] = 0;

    const std::vector<int> wrong = Sudoku::wrongCells(current, solution);
    QCOMPARE(wrong, std::vector<int>({4}));
}
