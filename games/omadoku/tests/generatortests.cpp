#include "generatortests.h"

#include <QtTest>

#include "sudoku.h"
#include "sudokugenerator.h"

namespace {

int clueCount(const Sudoku::Grid &grid) {
    int clues = 0;
    for (int value : grid) {
        if (value != 0)
            ++clues;
    }
    return clues;
}

}  // namespace

void GeneratorTests::producesUniquePuzzles_data() {
    QTest::addColumn<int>("difficulty");
    QTest::addColumn<quint32>("seed");

    const QVector<quint32> seeds {1u, 7u, 4242u, 987654321u};
    const QVector<QPair<QString, Difficulty>> levels {
        {QStringLiteral("easy"), Difficulty::Easy},
        {QStringLiteral("medium"), Difficulty::Medium},
        {QStringLiteral("hard"), Difficulty::Hard},
    };
    for (const auto &level : levels) {
        for (quint32 seed : seeds)
            QTest::newRow(qPrintable(QStringLiteral("%1-%2").arg(level.first).arg(seed)))
                << int(level.second) << seed;
    }
}

void GeneratorTests::producesUniquePuzzles() {
    QFETCH(int, difficulty);
    QFETCH(quint32, seed);

    const Puzzle puzzle = SudokuGenerator::generate(Difficulty(difficulty), seed);
    QVERIFY(Sudoku::isComplete(puzzle.solution));
    QCOMPARE(Sudoku::countSolutions(puzzle.givens, 2), 1);
    QCOMPARE(puzzle.seed, seed);
    QCOMPARE(int(puzzle.difficulty), difficulty);

    const int clues = clueCount(puzzle.givens);
    QVERIFY2(clues >= SudokuGenerator::minClues(Difficulty(difficulty))
                 && clues <= SudokuGenerator::maxClues(Difficulty(difficulty)),
             qPrintable(QStringLiteral("clue count %1 outside the difficulty window").arg(clues)));

    for (int i = 0; i < Sudoku::kCells; ++i) {
        if (puzzle.givens[i] != 0)
            QCOMPARE(puzzle.givens[i], puzzle.solution[i]);
    }
}

void GeneratorTests::isDeterministicPerSeed() {
    const Puzzle first = SudokuGenerator::generate(Difficulty::Medium, 20260829u);
    const Puzzle again = SudokuGenerator::generate(Difficulty::Medium, 20260829u);
    QCOMPARE(first.givens, again.givens);
    QCOMPARE(first.solution, again.solution);
}

void GeneratorTests::differentSeedsGiveDifferentPuzzles() {
    const Puzzle a = SudokuGenerator::generate(Difficulty::Easy, 11u);
    const Puzzle b = SudokuGenerator::generate(Difficulty::Easy, 12u);
    QVERIFY(a.givens != b.givens);
}

void GeneratorTests::randomSeedIsRecorded() {
    const Puzzle puzzle = SudokuGenerator::generate(Difficulty::Hard);
    QVERIFY(puzzle.seed != 0);
    // Replaying the recorded seed must rebuild the very same puzzle.
    QCOMPARE(SudokuGenerator::generate(Difficulty::Hard, puzzle.seed).givens, puzzle.givens);
}

void GeneratorTests::hardGenerationIsFast() {
    QElapsedTimer timer;
    timer.start();
    for (quint32 seed = 100; seed < 105; ++seed)
        SudokuGenerator::generate(Difficulty::Hard, seed);
    QVERIFY2(timer.elapsed() < 5000, qPrintable(QStringLiteral("5 hard puzzles took %1 ms").arg(timer.elapsed())));
}
