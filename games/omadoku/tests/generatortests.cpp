#include "generatortests.h"

#include <QtTest>

#include "sudoku.h"
#include "sudokugenerator.h"
#include "sudokugrader.h"

using SudokuGrader::Technique;

namespace {

const QVector<QPair<QString, Difficulty>> kLevels {
    {QStringLiteral("easy"), Difficulty::Easy},
    {QStringLiteral("medium"), Difficulty::Medium},
    {QStringLiteral("hard"), Difficulty::Hard},
    {QStringLiteral("extrahard"), Difficulty::ExtraHard},
};

int clueCount(const Sudoku::Grid &grid) {
    int clues = 0;
    for (int value : grid) {
        if (value != 0)
            ++clues;
    }
    return clues;
}

}  // namespace

void GeneratorTests::ceilingsClimbTheLadder() {
    QCOMPARE(SudokuGenerator::ceiling(Difficulty::Easy), Technique::NakedSingle);
    QCOMPARE(SudokuGenerator::ceiling(Difficulty::Medium), Technique::HiddenSingle);
    QCOMPARE(SudokuGenerator::ceiling(Difficulty::Hard), Technique::NakedTriple);
    QCOMPARE(SudokuGenerator::ceiling(Difficulty::ExtraHard), SudokuGrader::kHardestTechnique);
    for (int level = 1; level < kDifficultyCount; ++level)
        QVERIFY(SudokuGenerator::ceiling(Difficulty(level)) > SudokuGenerator::ceiling(Difficulty(level - 1)));
}

void GeneratorTests::everyLevelNeedsExactlyItsTechniques_data() {
    QTest::addColumn<int>("difficulty");
    QTest::addColumn<quint32>("seed");

    const QVector<quint32> seeds {1u, 7u, 42u, 4242u, 31337u, 987654321u};
    for (const auto &level : kLevels) {
        for (quint32 seed : seeds)
            QTest::newRow(qPrintable(QStringLiteral("%1-%2").arg(level.first).arg(seed)))
                << int(level.second) << seed;
    }
}

// The promise of a level: solvable with its own rungs of the ladder, not
// with the level below's, and graded as such.
void GeneratorTests::everyLevelNeedsExactlyItsTechniques() {
    QFETCH(int, difficulty);
    QFETCH(quint32, seed);
    const Difficulty level = Difficulty(difficulty);

    const Puzzle puzzle = SudokuGenerator::generate(level, seed);
    QVERIFY(Sudoku::isComplete(puzzle.solution));
    QCOMPARE(Sudoku::countSolutions(puzzle.givens, 2), 1);
    QCOMPARE(puzzle.seed, seed);
    QCOMPARE(puzzle.difficulty, level);
    for (int i = 0; i < Sudoku::kCells; ++i) {
        if (puzzle.givens[i] != 0)
            QCOMPARE(puzzle.givens[i], puzzle.solution[i]);
    }

    const Technique ceiling = SudokuGenerator::ceiling(level);
    QVERIFY(SudokuGrader::solvableWith(puzzle.givens, ceiling));
    if (level != Difficulty::Easy) {
        const Technique below = SudokuGenerator::ceiling(Difficulty(difficulty - 1));
        QVERIFY2(!SudokuGrader::solvableWith(puzzle.givens, below),
                 "the level below would have solved it");
        QVERIFY(puzzle.hardest > below);
    }
    QVERIFY(puzzle.hardest <= ceiling);
    // Easy and Medium never ask for more than singles; Medium is the one
    // that makes the player hunt for hidden ones.
    if (level == Difficulty::Easy || level == Difficulty::Medium)
        QVERIFY(SudokuGrader::solvableWith(puzzle.givens, Technique::HiddenSingle));
    if (level == Difficulty::Medium)
        QVERIFY(!SudokuGrader::solvableWith(puzzle.givens, Technique::NakedSingle));
    QVERIFY(SudokuGenerator::meetsLevel(puzzle.givens, level));
    QCOMPARE(puzzle.hardest, SudokuGrader::grade(puzzle.givens).hardest);

    qInfo("%s seed %u: %d clues, needs %s", qPrintable(QTest::currentDataTag()), seed,
          clueCount(puzzle.givens), qPrintable(SudokuGrader::techniqueId(puzzle.hardest)));
}

void GeneratorTests::isDeterministicPerSeed() {
    const Puzzle first = SudokuGenerator::generate(Difficulty::Medium, 20260829u);
    const Puzzle again = SudokuGenerator::generate(Difficulty::Medium, 20260829u);
    QCOMPARE(first.givens, again.givens);
    QCOMPARE(first.solution, again.solution);
    QCOMPARE(first.hardest, again.hardest);
}

void GeneratorTests::differentSeedsGiveDifferentPuzzles() {
    const Puzzle a = SudokuGenerator::generate(Difficulty::Easy, 11u);
    const Puzzle b = SudokuGenerator::generate(Difficulty::Easy, 12u);
    QVERIFY(a.givens != b.givens);
}

void GeneratorTests::randomSeedIsRecorded() {
    const Puzzle puzzle = SudokuGenerator::generate(Difficulty::Medium);
    QVERIFY(puzzle.seed != 0);
    // Replaying the recorded seed must rebuild the very same puzzle.
    QCOMPARE(SudokuGenerator::generate(Difficulty::Medium, puzzle.seed).givens, puzzle.givens);
}

// A level that takes longer than a moment to hand out a puzzle is broken,
// however honest its grading; the bound is loose enough for a slow CI box.
void GeneratorTests::everyLevelGeneratesQuickly() {
    constexpr int kSeeds = 3;
    for (const auto &level : kLevels) {
        QElapsedTimer timer;
        timer.start();
        for (quint32 seed = 100; seed < 100 + kSeeds; ++seed)
            SudokuGenerator::generate(level.second, seed);
        const qint64 average = timer.elapsed() / kSeeds;
        qInfo("%s: %lld ms per puzzle", qPrintable(level.first), average);
        QVERIFY2(average < 2000, qPrintable(QStringLiteral("%1 took %2 ms per puzzle").arg(level.first).arg(average)));
    }
}
