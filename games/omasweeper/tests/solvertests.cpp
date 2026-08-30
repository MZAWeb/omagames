#include "solvertests.h"

#include <QtTest>
#include <algorithm>

#include "presets.h"
#include "solver.h"

namespace {

Board boardWith(int width, int height, const std::vector<int> &mines) {
    Board board(width, height, int(mines.size()));
    board.placeMines(mines);
    return board;
}

std::vector<int> sorted(std::vector<int> cells) {
    std::sort(cells.begin(), cells.end());
    return cells;
}

}  // namespace

void SolverTests::singlesFindMines() {
    // 01*?  — the 1 has one hidden neighbour.
    Board board = boardWith(4, 1, {2});
    board.reveal(QPoint(0, 0));
    const Deduction step = Solver::deduce(board);
    QCOMPARE(step.technique, Technique::Singles);
    QCOMPARE(step.mines, (std::vector<int>{2}));
    QVERIFY(step.safe.empty());
}

void SolverTests::singlesFindSafeCells() {
    // 1F?   the player's flag satisfies both 1s, so the other neighbour is safe.
    // 1??
    Board board = boardWith(3, 2, {1});
    board.reveal(QPoint(0, 0));
    board.reveal(QPoint(0, 1));
    board.toggleFlag(QPoint(1, 0));
    const Deduction step = Solver::deduce(board);
    QCOMPARE(step.technique, Technique::Singles);
    QCOMPARE(step.safe, (std::vector<int>{4}));
    QVERIFY(step.mines.empty());
}

void SolverTests::subsetsSplitOverlappingNumbers() {
    // 1121   top row revealed; {a,b}=1 ⊂ {a,b,c}=1 makes c safe and
    // ?*?*   {c,d}=1 ⊂ {b,c,d}=2 makes b a mine.
    Board board = boardWith(4, 2, {5, 7});
    for (int x = 0; x < 4; ++x)
        board.reveal(QPoint(x, 0));
    const Deduction step = Solver::deduce(board);
    QCOMPARE(step.technique, Technique::Subsets);
    QCOMPARE(step.safe, (std::vector<int>{6}));
    QCOMPARE(step.mines, (std::vector<int>{5}));
}

void SolverTests::enumerationSolvesWhatSubsetsCannot() {
    // 1?*?1   No number is satisfied, no constraint is a subset of another,
    // *323*   yet only one assignment of the eight hidden cells fits.
    // 1?*?1
    Board board = boardWith(5, 3, {2, 5, 9, 12});
    for (int i : {0, 4, 6, 7, 8, 10, 14})
        board.reveal(board.point(i));
    const Deduction step = Solver::deduce(board);
    QCOMPARE(step.technique, Technique::Enumeration);
    QCOMPARE(sorted(step.mines), (std::vector<int>{2, 5, 9, 12}));
    QCOMPARE(sorted(step.safe), (std::vector<int>{1, 3, 11, 13}));
}

void SolverTests::enumerationUsesTheMineCountBehindTheFrontier() {
    // ?*10   The two 1s cannot tell which of their shared cells is the
    // ??10   mine, but it is the only mine left, so column 0 is safe.
    Board board = boardWith(4, 2, {1});
    board.reveal(QPoint(3, 1));
    QCOMPARE(board.revealedCount(), 4);
    const Deduction step = Solver::deduce(board);
    QCOMPARE(step.technique, Technique::Enumeration);
    QCOMPARE(sorted(step.safe), (std::vector<int>{0, 4}));
    QVERIFY(step.mines.empty());
}

void SolverTests::enumerationFindsMinesBehindTheFrontier() {
    // **10   Three mines left, the frontier can hold at most one, so both
    // *?10   cells behind it are mines.
    Board board = boardWith(4, 2, {0, 1, 4});
    board.reveal(QPoint(3, 1));
    const Deduction step = Solver::deduce(board);
    QCOMPARE(step.technique, Technique::Enumeration);
    QCOMPARE(sorted(step.mines), (std::vector<int>{0, 4}));
    QVERIFY(step.safe.empty());
}

void SolverTests::exhaustedBudgetDeducesNothing() {
    // Same position, but a budget too small to finish the enumeration: the
    // partial counts must not leak into a deduction.
    Board board = boardWith(5, 3, {2, 5, 9, 12});
    for (int i : {0, 4, 6, 7, 8, 10, 14})
        board.reveal(board.point(i));
    QVERIFY(Solver::deduce(board, 8).empty());
    QVERIFY(Solver::solve(board, 8).stuck);
}

void SolverTests::solveStopsAtAFiftyFifty() {
    Board board = boardWith(4, 2, {1});
    board.reveal(QPoint(3, 1));
    const SolveStats stats = Solver::solve(board);
    QVERIFY(stats.stuck);
    QCOMPARE(stats.status, Status::Playing);
    QCOMPARE(stats.enumerations, 2);
    QCOMPARE(board.revealedCount(), 6);
    QCOMPARE(board.cell(1).state, CellState::Hidden);
    QCOMPARE(board.cell(5).state, CellState::Hidden);
}

void SolverTests::solveNeverMarksAWrongCell() {
    int enumerations = 0;
    int wins = 0;
    for (const PresetSpec &preset : Presets::kAll) {
        for (quint32 seed = 1; seed <= 60; ++seed) {
            Board board(preset.width, preset.height, preset.mines, seed);
            const QPoint click(preset.width / 2, preset.height / 2);
            QVERIFY(!board.reveal(click).lost());
            const SolveStats stats = Solver::solve(board);
            QVERIFY(stats.status != Status::Lost);
            for (int i = 0; i < board.cellCount(); ++i) {
                const Cell &cell = board.cell(i);
                if (cell.state == CellState::Flagged)
                    QVERIFY2(cell.mine, qPrintable(QStringLiteral("%1 seed %2 cell %3").arg(preset.label).arg(seed).arg(i)));
                if (cell.state == CellState::Revealed)
                    QVERIFY(!cell.mine);
            }
            QCOMPARE(stats.stuck, stats.status != Status::Won);
            enumerations += stats.enumerations;
            wins += stats.status == Status::Won ? 1 : 0;
        }
    }
    // The hard technique actually gets exercised, and some boards fall to it.
    QVERIFY(enumerations > 0);
    QVERIFY(wins > 0);
}

void SolverTests::solvabilityFromAFirstClick() {
    QVERIFY(Solver::isSolvableWithoutGuessing(boardWith(4, 1, {2}), QPoint(0, 0)));
    QVERIFY(!Solver::isSolvableWithoutGuessing(boardWith(4, 2, {1}), QPoint(3, 1)));
    // Clicking a mine is not a solve, and unplaced mines cannot be judged.
    QVERIFY(!Solver::isSolvableWithoutGuessing(boardWith(4, 1, {2}), QPoint(2, 0)));
    QVERIFY(!Solver::isSolvableWithoutGuessing(Board(9, 9, 10), QPoint(4, 4)));
}

void SolverTests::deduceIsSilentWhenThereIsNothingToSee() {
    Board fresh = boardWith(3, 3, {0});
    QVERIFY(Solver::deduce(fresh).empty());
    fresh.reveal(QPoint(0, 0));
    QVERIFY(Solver::deduce(fresh).empty());
    Board done = boardWith(3, 3, {0});
    done.reveal(QPoint(2, 2));
    QCOMPARE(done.status(), Status::Won);
    QVERIFY(Solver::deduce(done).empty());
    QVERIFY(!Solver::solve(done).stuck);
}
