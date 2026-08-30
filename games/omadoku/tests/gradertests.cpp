#include "gradertests.h"

#include <QRandomGenerator>
#include <QtTest>

#include "sudoku.h"
#include "sudokugrader.h"
#include "testgrids.h"

using Sudoku::Grid;
using SudokuGrader::CandidateGrid;
using SudokuGrader::Technique;

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

// Positions from the technique fixtures whose whole solve is known.
const QString kHiddenPairPuzzle = QStringLiteral(
    "65193..2.4326.591.7891426..1635....25283...9.947......81....3..27..13.69396...2.1");
const QString kXWingPuzzle = QStringLiteral(
    "63.94...55......4.7.4......46523871981.49..639.3..1..424.81..3.1.....4..3.67.4..1");
const QString kXYWingPuzzle = QStringLiteral(
    "54928..1.37815.9.226179..85432578196785619234916342..88574.1.29693827..11249.58..");
// The swordfish fixture goes on to need an XY-wing, so a generated Extra
// hard whose top rung really is the swordfish stands in for it here.
const QString kSwordfishPuzzle = QStringLiteral(
    "..45.3.9..578.........7..6.......32.3.178..5.....56..9..9.3...74..2......634.....");

// Singles only, but stuck without a row or column scan (see the technique
// fixture for the line hidden single).
const QString kLineSinglePuzzle = QStringLiteral(
    "78293615419385426756421798331542967847....532.285734..83..457...473..8.5.5.7..34.");

// "AI Escargot" - a famously hard puzzle that needs chains no rung of the
// ladder provides.
const QString kEscargot = QStringLiteral(
    "1....7.9."
    ".3..2...8"
    "..96..5.."
    "..53..9.."
    ".1..8...2"
    "6....4..."
    "3......1."
    ".4......7"
    "..7...3..");

// A puzzle with as few clues as a random removal order allows while staying
// unique: as far up the ladder as carving alone gets.
Grid minimalPuzzle(quint32 seed) {
    QRandomGenerator rng(seed);
    Grid grid = gridFromString(kSolution);
    std::vector<int> order(Sudoku::kCells);
    for (int i = 0; i < Sudoku::kCells; ++i)
        order[size_t(i)] = i;
    for (int i = Sudoku::kCells - 1; i > 0; --i)
        std::swap(order[size_t(i)], order[rng.bounded(i + 1)]);
    for (int index : order) {
        const int value = grid[size_t(index)];
        grid[size_t(index)] = 0;
        if (Sudoku::countSolutions(grid, 2) != 1)
            grid[size_t(index)] = value;
    }
    return grid;
}

}  // namespace

void GraderTests::completeGridNeedsNothing() {
    const SudokuGrader::Grading grading = SudokuGrader::grade(gridFromString(kSolution));
    QVERIFY(grading.solved);
    QCOMPARE(grading.hardest, Technique::LastDigit);
}

void GraderTests::stepTakesTheEasiestRungUnderTheCeiling() {
    // Row 2 has a single gap here, so even the full ladder starts at the bottom.
    Grid easy = gridFromString(kSolution);
    for (int index : {0, 5, 13, 27, 40, 44, 58, 66, 71, 80})
        easy[size_t(index)] = 0;
    CandidateGrid singles(easy);
    const std::optional<Technique> first = SudokuGrader::step(singles, SudokuGrader::kHardestTechnique);
    QVERIFY(first.has_value());
    QCOMPARE(*first, Technique::LastDigit);

    // Only an X-wing moves this position, so the ceiling decides everything;
    // and what the X-wing uncovers is an XY-wing, two rungs too far for it.
    CandidateGrid grid(gridFromString(kXWingPuzzle));
    QVERIFY(!SudokuGrader::step(grid, Technique::HiddenTriple));
    const std::optional<Technique> used = SudokuGrader::step(grid, Technique::XWing);
    QVERIFY(used.has_value());
    QCOMPARE(*used, Technique::XWing);
    QVERIFY(!SudokuGrader::step(grid, Technique::Swordfish));
    const std::optional<Technique> next = SudokuGrader::step(grid, SudokuGrader::kHardestTechnique);
    QVERIFY(next.has_value());
    QCOMPARE(*next, Technique::XYWing);
}

void GraderTests::gradeReportsTheHardestRungNeeded() {
    QCOMPARE(SudokuGrader::grade(gridFromString(kHiddenPairPuzzle)).hardest, Technique::HiddenPair);
    QCOMPARE(SudokuGrader::grade(gridFromString(kXYWingPuzzle)).hardest, Technique::XYWing);
    QCOMPARE(SudokuGrader::grade(gridFromString(kSwordfishPuzzle)).hardest, Technique::Swordfish);
    for (const QString &puzzle : {kHiddenPairPuzzle, kXYWingPuzzle, kSwordfishPuzzle})
        QVERIFY(SudokuGrader::grade(gridFromString(puzzle)).solved);
}

void GraderTests::ceilingDecidesWhatIsSolvable() {
    const Grid puzzle = gridFromString(kXYWingPuzzle);
    QVERIFY(SudokuGrader::solvableWith(puzzle, Technique::XYWing));
    QVERIFY(!SudokuGrader::solvableWith(puzzle, Technique::Swordfish));
    QVERIFY(!SudokuGrader::solvableWith(puzzle, Technique::XWing));
    QVERIFY(!SudokuGrader::solvableWith(puzzle, Technique::HiddenSingleLine));

    Grid easy = gridFromString(kSolution);
    for (int index : {0, 5, 13, 27, 40, 44, 58, 66, 71, 80})
        easy[size_t(index)] = 0;
    QVERIFY(SudokuGrader::solvableWith(easy, Technique::NakedSingle));
}

// The two hidden-single rungs are what tell Easy from Medium, so the box
// one must never quietly solve what only a row or column scan can.
void GraderTests::boxAndLineHiddenSinglesAreSeparateRungs() {
    const Grid puzzle = gridFromString(kLineSinglePuzzle);
    CandidateGrid grid(puzzle);
    QVERIFY(!SudokuGrader::hiddenSingleBox(grid));
    QVERIFY(!SudokuGrader::nakedSingle(grid));
    QVERIFY(SudokuGrader::hiddenSingleLine(grid));

    QVERIFY(!SudokuGrader::solvableWith(puzzle, Technique::NakedSingle));
    QVERIFY(SudokuGrader::solvableWith(puzzle, Technique::HiddenSingleLine));
    QCOMPARE(SudokuGrader::grade(puzzle).hardest, Technique::HiddenSingleLine);
}

void GraderTests::neverGuessesOnAPuzzleBeyondTheLadder() {
    const Grid puzzle = gridFromString(kEscargot);
    QCOMPARE(Sudoku::countSolutions(puzzle, 2), 1);
    const SudokuGrader::Grading grading = SudokuGrader::grade(puzzle);
    QVERIFY(!grading.solved);
    Grid solution = puzzle;
    QVERIFY(Sudoku::solve(solution));

    // Whatever it did manage before getting stuck was still correct.
    CandidateGrid grid(puzzle);
    while (SudokuGrader::step(grid, SudokuGrader::kHardestTechnique))
        QVERIFY(agreesWithSolution(grid, solution));
    QVERIFY(!grid.isSolved());
    QVERIFY(!grid.isStuck());
}

void GraderTests::ladderAgreesWithBacktracking() {
    const Grid solution = gridFromString(kSolution);
    int solved = 0;
    for (quint32 seed = 1; seed <= 40; ++seed) {
        const Grid puzzle = minimalPuzzle(seed);
        CandidateGrid grid(puzzle);
        QVERIFY(agreesWithSolution(grid, solution));
        while (const std::optional<Technique> used = SudokuGrader::step(grid, SudokuGrader::kHardestTechnique)) {
            QVERIFY2(agreesWithSolution(grid, solution),
                     qPrintable(QStringLiteral("seed %1: %2 broke the solution")
                                    .arg(seed).arg(SudokuGrader::techniqueId(*used))));
        }
        if (grid.isSolved()) {
            QCOMPARE(grid.values(), solution);
            ++solved;
        }
    }
    QVERIFY2(solved > 0, "no minimal puzzle fell to the ladder at all");
}
