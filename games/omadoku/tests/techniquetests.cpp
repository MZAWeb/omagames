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

    QTest::newRow("last digit") << int(Technique::LastDigit)
        << ".34678912672195348198342567859761423426853791713924856961537284287419635345286179"
        << "r1c1=5";
    QTest::newRow("hidden single (box)") << int(Technique::HiddenSingleBox)
        << ".463.1.7..51.......379..8...8..123......5......9...7.6..8.7...2..2.......6.5...9."
        << "r3c6=5";
    QTest::newRow("naked single") << int(Technique::NakedSingle)
        << ".4.3...7..5.2......3.9..8...85.123.9....59.....9...756.98.7...2..2.9.....6.52..9."
        << "r4c8=4";
    QTest::newRow("hidden single (line)") << int(Technique::HiddenSingleLine)
        << "78293615419385426756421798331542967847....532.285734..83..457...473..8.5.5.7..34."
        << "r5c3=9";
    QTest::newRow("naked pair") << int(Technique::NakedPair)
        << ".....12..85..4..63....6...4.8.7....93.......81.98347..498.....25.24.....7.3...4.."
        << "r1c2-6 r3c2-1 r5c2-6 r6c2-6";
    QTest::newRow("hidden pair") << int(Technique::HiddenPair)
        << "65193..2.4326.591.7891426..1635....25283...9.947......81....3..27..13.69396...2.1"
        << "r7c5-257 r7c6-47";
    QTest::newRow("naked triple") << int(Technique::NakedTriple)
        << "7.13....89.8.5.21335.8..47.4..9...871.....3.28....2.4.2831759645946..7216172..835"
        << "r5c5-6 r5c6-6";
    QTest::newRow("hidden triple") << int(Technique::HiddenTriple)
        << ".9...38..2..8.....8.5741.924796325813..185974581974236.5.31.428..84.7.6......87.."
        << "r8c2-1 r9c2-16 r9c3-6";
    QTest::newRow("x-wing") << int(Technique::XWing)
        << "63.94...55......4.7.4......46523871981.49..639.3..1..424.81..3.1.....4..3.67.4..1"
        << "r3c6-5 r6c7-5 r8c6-5 r9c7-5";
    QTest::newRow("swordfish") << int(Technique::Swordfish)
        << "6.287935..75.43..88.3.25.....8214.6..647589.35.7936.8.451362879..958..46.8649..3."
        << "r2c7-1 r3c2-1 r3c7-1 r3c9-1";
    QTest::newRow("xy-wing") << int(Technique::XYWing)
        << "54928..1.37815.9.226179..85432578196785619234916342..88574.1.29693827..11249.58.."
        << "r1c7-6 r9c8-6";
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
    Technique unknown = Technique::XYWing;
    QVERIFY(!SudokuGrader::techniqueFromId(QStringLiteral("guessing"), &unknown));
    QCOMPARE(unknown, Technique::XYWing);  // left untouched
}
