#include "techniquetests.h"

#include <QtTest>

#include "sudoku.h"
#include "sudokutechniques.h"
#include "testgrids.h"

using Sudoku::Grid;
using SudokuGrader::CandidateGrid;
using SudokuGrader::Mask;
using SudokuGrader::Technique;

namespace {

// What a technique did, as "r1c2=5" for a placement and "r1c2-46" for the
// digits it struck out of a cell, in cell order. A placement also clears its
// digit from the peers' candidates; that is bookkeeping, not the technique's
// finding, so it goes unlisted.
QString describe(const CandidateGrid &before, const CandidateGrid &after) {
    QStringList changes;
    const bool placed = after.emptyCount() != before.emptyCount();
    for (int i = 0; i < Sudoku::kCells; ++i) {
        const QString cell = QStringLiteral("r%1c%2").arg(i / 9 + 1).arg(i % 9 + 1);
        if (after.value(i) != before.value(i)) {
            changes << cell + QLatin1Char('=') + QString::number(after.value(i));
            continue;
        }
        if (placed)
            continue;
        const Mask gone = Mask(before.candidates(i) & ~after.candidates(i));
        if (!gone)
            continue;
        QString digits;
        for (int d = 1; d <= 9; ++d) {
            if (gone & SudokuGrader::bitOf(d))
                digits += QString::number(d);
        }
        changes << cell + QLatin1Char('-') + digits;
    }
    return changes.join(QLatin1Char(' '));
}

}  // namespace

// Every grid below is a real position - unique solution, reached by
// placements only - where nothing easier on the ladder makes progress and
// the technique under test does exactly what is listed.
void TechniqueTests::firesOnItsOwnGridAndNothingEasierDoes_data() {
    QTest::addColumn<int>("technique");
    QTest::addColumn<QString>("grid");
    QTest::addColumn<QString>("expected");

    QTest::newRow("naked single") << int(Technique::NakedSingle)
        << ".34678912672195348198342567859761423426853791713924856961537284287419635345286179"
        << "r1c1=5";
    QTest::newRow("hidden single") << int(Technique::HiddenSingle)
        << "471953268869142375235867..9.8..25..7..647.823....8...435.296.816.8734..2...518.36"
        << "r4c3=4";
    QTest::newRow("naked pair") << int(Technique::NakedPair)
        << ".....12..85..4..63....6...4.8.7....93.......81.98347..498.....25.24.....7.3...4.."
        << "r1c2-6 r3c2-1 r5c2-6 r6c2-6";
    QTest::newRow("hidden pair") << int(Technique::HiddenPair)
        << "65193..2.4326.591.7891426..1635....25283...9.947......81....3..27..13.69396...2.1"
        << "r7c5-257 r7c6-47";
    QTest::newRow("pointing pair") << int(Technique::PointingPair)
        << "439.61.5..2..583..5.8.34.......82...96..75....85693.7.89.547..1.5.826..9...319..."
        << "r2c8-6 r2c9-6";
    QTest::newRow("claiming") << int(Technique::Claiming)
        << ".8...5.79....8.1.2....3..858....3.27...8.2...2.35..8..798..6..1.3.417298421..87.."
        << "r4c4-9 r4c5-9 r5c5-9";
    QTest::newRow("naked triple") << int(Technique::NakedTriple)
        << "7.13....89.8.5.21335.8..47.4..9...871.....3.28....2.4.2831759645946..7216172..835"
        << "r5c5-6 r5c6-6";
    QTest::newRow("x-wing") << int(Technique::XWing)
        << "63.94...55......4.7.4......46523871981.49..639.3..1..424.81..3.1.....4..3.67.4..1"
        << "r3c6-5 r6c7-5 r8c6-5 r9c7-5";
    QTest::newRow("y-wing") << int(Technique::YWing)
        << "54928..1.37815.9.226179..85432578196785619234916342..88574.1.29693827..11249.58.."
        << "r1c7-6 r9c8-6";
    QTest::newRow("swordfish") << int(Technique::Swordfish)
        << "6.287935..75.43..88.3.25.....8214.6..647589.35.7936.8.451362879..958..46.8649..3."
        << "r2c7-1 r3c2-1 r3c7-1 r3c9-1";
}

void TechniqueTests::firesOnItsOwnGridAndNothingEasierDoes() {
    QFETCH(int, technique);
    QFETCH(QString, grid);
    QFETCH(QString, expected);

    const Grid puzzle = gridFromString(grid);
    QCOMPARE(Sudoku::countSolutions(puzzle, 2), 1);
    Grid solution = puzzle;
    QVERIFY(Sudoku::solve(solution));

    const CandidateGrid before(puzzle);
    for (int easier = 0; easier < technique; ++easier) {
        CandidateGrid probe = before;
        QVERIFY2(!SudokuGrader::apply(Technique(easier), probe),
                 qPrintable(SudokuGrader::techniqueId(Technique(easier)) + QStringLiteral(" made progress")));
    }

    CandidateGrid after = before;
    QVERIFY(SudokuGrader::apply(Technique(technique), after));
    QCOMPARE(describe(before, after), expected);
    QVERIFY(agreesWithSolution(after, solution));

    // Applied again to the same position it finds nothing new to do.
    CandidateGrid twice = after;
    if (!SudokuGrader::apply(Technique(technique), twice))
        QCOMPARE(describe(after, twice), QString());
}

void TechniqueTests::idsRoundTrip() {
    QSet<QString> seen;
    for (int t = 0; t < SudokuGrader::kTechniqueCount; ++t) {
        const QString id = SudokuGrader::techniqueId(Technique(t));
        QVERIFY(!id.isEmpty());
        QVERIFY2(!seen.contains(id), qPrintable(id + QStringLiteral(" used twice")));
        seen.insert(id);
        Technique back = Technique::NakedSingle;
        QVERIFY(SudokuGrader::techniqueFromId(id, &back));
        QCOMPARE(int(back), t);
    }
    Technique unknown = Technique::YWing;
    QVERIFY(!SudokuGrader::techniqueFromId(QStringLiteral("guessing"), &unknown));
    QCOMPARE(unknown, Technique::YWing);  // left untouched
}
