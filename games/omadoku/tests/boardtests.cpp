#include "boardtests.h"

#include <QtTest>

#include "sudoku.h"
#include "sudokugenerator.h"
#include "sudokugrader.h"

namespace {

constexpr quint32 kSeed = 424242u;

int wrongDigitFor(const Sudoku::Grid &solution, int index) {
    return solution[size_t(index)] % 9 + 1;  // any digit that is not the answer
}

}  // namespace

void BoardTests::init() {
    m_board = SudokuBoard();
    m_board.setPuzzle(SudokuGenerator::generate(Difficulty::Medium, kSeed));
}

int BoardTests::emptyCell(int nth) const {
    for (int i = 0; i < Sudoku::kCells; ++i) {
        if (!m_board.isGiven(i) && nth-- == 0)
            return i;
    }
    return -1;
}

int BoardTests::givenCell() const {
    for (int i = 0; i < Sudoku::kCells; ++i) {
        if (m_board.isGiven(i))
            return i;
    }
    return -1;
}

int BoardTests::emptyPeerOf(int index) const {
    for (int peer : Sudoku::peers(index)) {
        if (!m_board.isGiven(peer))
            return peer;
    }
    return -1;
}

int BoardTests::emptyNonPeerOf(int index) const {
    const std::vector<int> &peers = Sudoku::peers(index);
    for (int i = 0; i < Sudoku::kCells; ++i) {
        if (i == index || m_board.isGiven(i))
            continue;
        if (std::find(peers.begin(), peers.end(), i) == peers.end())
            return i;
    }
    return -1;
}

void BoardTests::fillEverythingCorrectlyExcept(int index) {
    const Sudoku::Grid &solution = m_board.puzzle().solution;
    for (int i = 0; i < Sudoku::kCells; ++i) {
        if (i == index || m_board.isGiven(i))
            continue;
        m_board.setValue(i, solution[size_t(i)]);
    }
}

void BoardTests::givensAreImmutable() {
    const int given = givenCell();
    const int original = m_board.value(given);

    QVERIFY(m_board.setValue(given, original % 9 + 1).empty());
    QVERIFY(m_board.toggleNote(given, 5).empty());
    QVERIFY(m_board.erase(given).empty());
    QCOMPARE(m_board.value(given), original);
    QCOMPARE(m_board.notes(given), quint16(0));
    QVERIFY(!m_board.canUndo());
}

void BoardTests::entryClearsOnlyItsOwnNotes() {
    const int cell = emptyCell();
    const int peer = emptyPeerOf(cell);
    const int elsewhere = emptyNonPeerOf(cell);

    m_board.toggleNote(cell, 4);
    m_board.toggleNote(cell, 7);
    m_board.toggleNote(peer, 7);
    m_board.toggleNote(elsewhere, 7);

    QCOMPARE(m_board.setValue(cell, 7), std::vector<int>({cell}));
    QCOMPARE(m_board.value(cell), 7);
    QCOMPARE(m_board.notes(cell), quint16(0));  // own notes cleared

    // Pencil marks elsewhere are the player's own bookkeeping: a placement
    // never tidies them up, peer or not.
    QVERIFY(m_board.notes(peer) & quint16(1u << 6));
    QVERIFY(m_board.notes(elsewhere) & quint16(1u << 6));
}

void BoardTests::notesToggleOnlyInEmptyCells() {
    const int cell = emptyCell();
    m_board.toggleNote(cell, 3);
    QCOMPARE(m_board.notes(cell), quint16(1u << 2));
    m_board.toggleNote(cell, 9);
    QCOMPARE(m_board.notes(cell), quint16((1u << 2) | (1u << 8)));
    m_board.toggleNote(cell, 3);
    QCOMPARE(m_board.notes(cell), quint16(1u << 8));

    m_board.setValue(cell, 1);
    QVERIFY(m_board.toggleNote(cell, 5).empty());
    QCOMPARE(m_board.notes(cell), quint16(0));
    QVERIFY(m_board.toggleNote(cell, 0).empty());
}

void BoardTests::eraseClearsValueAndNotes() {
    const int cell = emptyCell();
    m_board.setValue(cell, 6);
    QCOMPARE(m_board.erase(cell), std::vector<int>({cell}));
    QCOMPARE(m_board.value(cell), 0);

    m_board.toggleNote(cell, 2);
    m_board.erase(cell);
    QCOMPARE(m_board.notes(cell), quint16(0));
    QVERIFY(m_board.erase(cell).empty());  // nothing left to erase
}

void BoardTests::undoRestoresValuesAndNotes() {
    const int cell = emptyCell();
    const int other = emptyCell(1);

    m_board.toggleNote(other, 8);
    m_board.toggleNote(cell, 3);
    m_board.setValue(cell, 8);
    QVERIFY(m_board.canUndo());

    m_board.undo();
    QCOMPARE(m_board.value(cell), 0);
    QCOMPARE(m_board.notes(cell), quint16(1u << 2));  // the cleared note came back

    m_board.undo();
    m_board.undo();
    QCOMPARE(m_board.notes(cell), quint16(0));
    QCOMPARE(m_board.notes(other), quint16(0));
    QVERIFY(!m_board.canUndo());
    QVERIFY(m_board.undo().empty());
}

void BoardTests::undoKeepsAtLeastAHundredLevels() {
    const int cell = emptyCell();
    for (int i = 0; i < 120; ++i)
        m_board.toggleNote(cell, i % 9 + 1);
    for (int i = 0; i < 120; ++i)
        m_board.undo();
    QCOMPARE(m_board.notes(cell), quint16(0));
    QVERIFY(!m_board.canUndo());
}

void BoardTests::validateAsYouGoFlagsWrongEntries() {
    const int cell = emptyCell();
    const Sudoku::Grid &solution = m_board.puzzle().solution;

    m_board.setValue(cell, wrongDigitFor(solution, cell));
    QVERIFY(m_board.isWrong(cell));

    m_board.setValue(cell, solution[size_t(cell)]);
    QVERIFY(!m_board.isWrong(cell));

    // Notes are never validated.
    const int other = emptyCell(1);
    m_board.toggleNote(other, wrongDigitFor(solution, other));
    QVERIFY(!m_board.isWrong(other));
}

void BoardTests::checkWhenFullDefersFlagging() {
    m_board.setValidateAsYouGo(false);
    const int cell = emptyCell();
    const Sudoku::Grid &solution = m_board.puzzle().solution;

    m_board.setValue(cell, wrongDigitFor(solution, cell));
    QVERIFY(!m_board.isWrong(cell));

    fillEverythingCorrectlyExcept(cell);
    QCOMPARE(m_board.filledCount(), Sudoku::kCells);
    QVERIFY(m_board.isWrong(cell));   // the whole grid is validated at once
    QVERIFY(!m_board.isWrong(emptyCell(1)));

    // Turning the setting back on validates immediately.
    m_board.erase(cell);
    QVERIFY(!m_board.isWrong(cell));
    m_board.setValue(cell, wrongDigitFor(solution, cell));
    m_board.setValidateAsYouGo(true);
    QVERIFY(m_board.isWrong(cell));
}

void BoardTests::solvedWhenEveryCellMatches() {
    const int last = emptyCell();
    fillEverythingCorrectlyExcept(last);
    QVERIFY(!m_board.isSolved());
    m_board.setValue(last, m_board.puzzle().solution[size_t(last)]);
    QVERIFY(m_board.isSolved());
}

void BoardTests::restartClearsEntriesAndHistory() {
    const int cell = emptyCell();
    const int other = emptyCell(1);
    m_board.setValue(cell, 5);
    m_board.toggleNote(other, 5);

    const std::vector<int> changed = m_board.restart();
    QCOMPARE(int(changed.size()), 2);
    QCOMPARE(m_board.value(cell), 0);
    QCOMPARE(m_board.notes(other), quint16(0));
    QVERIFY(!m_board.canUndo());
    QCOMPARE(m_board.filledCount(), m_board.digitCount(1) + m_board.digitCount(2) + m_board.digitCount(3)
                 + m_board.digitCount(4) + m_board.digitCount(5) + m_board.digitCount(6)
                 + m_board.digitCount(7) + m_board.digitCount(8) + m_board.digitCount(9));
}

void BoardTests::countsFilledCellsAndDigits() {
    const int before = m_board.filledCount();
    const int cell = emptyCell();
    m_board.setValue(cell, 3);
    QCOMPARE(m_board.filledCount(), before + 1);

    int threes = 0;
    for (int i = 0; i < Sudoku::kCells; ++i) {
        if (m_board.value(i) == 3)
            ++threes;
    }
    QCOMPARE(m_board.digitCount(3), threes);
}

void BoardTests::countsOnlyPlayerEntries() {
    QCOMPARE(m_board.entryCount(), 0);  // a fresh puzzle is all givens
    const int cell = emptyCell();
    m_board.setValue(cell, 8);
    QCOMPARE(m_board.entryCount(), 1);

    const int noted = emptyCell(1);
    m_board.toggleNote(noted, 4);
    QCOMPARE(m_board.entryCount(), 2);  // pencil marks are work too

    m_board.restart();
    QCOMPARE(m_board.entryCount(), 0);
}

void BoardTests::jsonRoundTripsBoardState() {
    const int cell = emptyCell();
    const int other = emptyCell(1);
    m_board.setValue(cell, 4);
    m_board.toggleNote(other, 2);
    m_board.toggleNote(other, 6);

    SudokuBoard restored;
    QVERIFY(SudokuBoard::fromJson(m_board.toJson(), &restored));
    QCOMPARE(restored.puzzle().givens, m_board.puzzle().givens);
    QCOMPARE(restored.puzzle().solution, m_board.puzzle().solution);
    QCOMPARE(int(restored.puzzle().difficulty), int(Difficulty::Medium));
    QCOMPARE(restored.puzzle().seed, kSeed);
    QCOMPARE(restored.value(cell), 4);
    QCOMPARE(restored.notes(other), m_board.notes(other));
    QVERIFY(restored.isGiven(givenCell()));

    SudokuBoard broken;
    QVERIFY(!SudokuBoard::fromJson(QJsonObject(), &broken));
    QJsonObject truncated = m_board.toJson();
    truncated.remove(QStringLiteral("values"));
    QVERIFY(!SudokuBoard::fromJson(truncated, &broken));
}

void BoardTests::jsonKeepsTheLevelAndTechnique() {
    const Puzzle puzzle = SudokuGenerator::generate(Difficulty::ExtraHard, 5u);
    m_board.setPuzzle(puzzle);
    SudokuBoard restored;
    QVERIFY(SudokuBoard::fromJson(m_board.toJson(), &restored));
    QCOMPARE(restored.puzzle().difficulty, Difficulty::ExtraHard);
    QCOMPARE(restored.puzzle().hardest, puzzle.hardest);
    QCOMPARE(restored.puzzle().seed, puzzle.seed);

    // A save from before puzzles were graded names no technique and knows
    // only three levels: it is graded on the way in, and an unknown level
    // lands on Easy rather than being refused.
    QJsonObject old = m_board.toJson();
    old.remove(QStringLiteral("technique"));
    old.insert(QStringLiteral("difficulty"), 7);
    QVERIFY(SudokuBoard::fromJson(old, &restored));
    QCOMPARE(restored.puzzle().hardest, SudokuGrader::grade(puzzle.givens).hardest);
    QCOMPARE(restored.puzzle().difficulty, Difficulty::Easy);
}
