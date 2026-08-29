#include "gradertests.h"

#include <QtTest>

#include "sudoku.h"
#include "sudokugenerator.h"
#include "sudokugrader.h"
#include "testgrids.h"

using Sudoku::Grid;

namespace {

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

// "AI Escargot" — a famously hard puzzle that no amount of singles will crack.
const QString kHardPuzzle = QStringLiteral(
    "1....7.9."
    ".3..2...8"
    "..96..5.."
    "..53..9.."
    ".1..8...2"
    "6....4..."
    "3......1."
    ".4......7"
    "..7...3..");

}  // namespace

void GraderTests::completeGridNeedsNoTechnique() {
    QVERIFY(SudokuGrader::solvableWithSingles(gridFromString(kSolution)));
}

void GraderTests::singlesFinishAnEasyGrid() {
    Grid grid = gridFromString(kSolution);
    for (int index : {0, 5, 13, 27, 40, 44, 58, 66, 71, 80})
        grid[index] = 0;
    QVERIFY(SudokuGrader::solvableWithSingles(grid));
}

void GraderTests::rejectsPuzzleNeedingMoreThanSingles() {
    const Grid grid = gridFromString(kHardPuzzle);
    QCOMPARE(Sudoku::countSolutions(grid, 2), 1);
    QVERIFY(!SudokuGrader::solvableWithSingles(grid));
}

void GraderTests::easyPuzzlesFallToSinglesButHardOnesDoNot() {
    for (quint32 seed = 1; seed <= 4; ++seed) {
        QVERIFY(SudokuGrader::solvableWithSingles(SudokuGenerator::generate(Difficulty::Easy, seed).givens));
        QVERIFY(!SudokuGrader::solvableWithSingles(SudokuGenerator::generate(Difficulty::Hard, seed).givens));
    }
}
