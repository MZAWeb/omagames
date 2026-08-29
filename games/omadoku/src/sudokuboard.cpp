#include "sudokuboard.h"

#include <QJsonArray>

#include <algorithm>

namespace {

// Deep enough that no realistic session runs out, small enough to serialize.
constexpr size_t kMaxUndoSteps = 200;

bool inRange(int index) { return index >= 0 && index < Sudoku::kCells; }

QJsonArray gridToJson(const Sudoku::Grid &grid) {
    QJsonArray array;
    for (int value : grid)
        array.append(value);
    return array;
}

bool gridFromJson(const QJsonValue &value, Sudoku::Grid *grid) {
    const QJsonArray array = value.toArray();
    if (array.size() != Sudoku::kCells)
        return false;
    for (int i = 0; i < Sudoku::kCells; ++i) {
        const int digit = array.at(i).toInt(-1);
        if (digit < 0 || digit > 9)
            return false;
        (*grid)[size_t(i)] = digit;
    }
    return true;
}

}  // namespace

void SudokuBoard::setPuzzle(const Puzzle &puzzle) {
    m_puzzle = puzzle;
    m_values = puzzle.givens;
    m_notes.fill(0);
    m_undo.clear();
    refreshWrong();
}

bool SudokuBoard::isGiven(int index) const {
    return inRange(index) && m_puzzle.givens[size_t(index)] != 0;
}

int SudokuBoard::value(int index) const {
    return inRange(index) ? m_values[size_t(index)] : 0;
}

quint16 SudokuBoard::notes(int index) const {
    return inRange(index) ? m_notes[size_t(index)] : quint16(0);
}

bool SudokuBoard::isWrong(int index) const {
    return inRange(index) && m_wrong[size_t(index)];
}

int SudokuBoard::filledCount() const {
    return int(std::count_if(m_values.begin(), m_values.end(), [](int value) { return value != 0; }));
}

int SudokuBoard::entryCount() const {
    int entries = 0;
    for (int i = 0; i < Sudoku::kCells; ++i) {
        if (!isGiven(i) && (m_values[size_t(i)] != 0 || m_notes[size_t(i)] != 0))
            ++entries;
    }
    return entries;
}

int SudokuBoard::digitCount(int digit) const {
    return int(std::count(m_values.begin(), m_values.end(), digit));
}

bool SudokuBoard::isSolved() const {
    return m_values == m_puzzle.solution;
}

void SudokuBoard::setCheckAsYouGo(bool checkAsYouGo) {
    if (m_checkAsYouGo == checkAsYouGo)
        return;
    m_checkAsYouGo = checkAsYouGo;
    refreshWrong();
}

std::vector<int> SudokuBoard::setValue(int index, int value) {
    if (!inRange(index) || isGiven(index) || value < 1 || value > 9)
        return {};
    if (m_values[size_t(index)] == value)
        return {};

    // Placing a digit also retracts it from the pencil marks of every peer,
    // which is what players do by hand anyway.
    std::vector<int> changed {index};
    const quint16 bit = quint16(1u << (value - 1));
    for (int peer : Sudoku::peers(index)) {
        if (m_notes[size_t(peer)] & bit)
            changed.push_back(peer);
    }
    pushUndo(changed);

    m_values[size_t(index)] = value;
    m_notes[size_t(index)] = 0;
    for (int peer : Sudoku::peers(index))
        m_notes[size_t(peer)] &= quint16(~bit);
    refreshWrong();
    return changed;
}

std::vector<int> SudokuBoard::toggleNote(int index, int digit) {
    // Notes only make sense in an empty cell; filled cells must be erased first.
    if (!inRange(index) || isGiven(index) || digit < 1 || digit > 9 || m_values[size_t(index)] != 0)
        return {};
    pushUndo({index});
    m_notes[size_t(index)] ^= quint16(1u << (digit - 1));
    return {index};
}

std::vector<int> SudokuBoard::erase(int index) {
    if (!inRange(index) || isGiven(index))
        return {};
    if (m_values[size_t(index)] == 0 && m_notes[size_t(index)] == 0)
        return {};
    pushUndo({index});
    m_values[size_t(index)] = 0;
    m_notes[size_t(index)] = 0;
    refreshWrong();
    return {index};
}

std::vector<int> SudokuBoard::undo() {
    if (m_undo.empty())
        return {};
    const std::vector<CellState> step = m_undo.back();
    m_undo.pop_back();
    std::vector<int> changed;
    changed.reserve(step.size());
    for (const CellState &state : step) {
        applyState(state);
        changed.push_back(state.index);
    }
    refreshWrong();
    return changed;
}

std::vector<int> SudokuBoard::restart() {
    std::vector<int> changed;
    for (int i = 0; i < Sudoku::kCells; ++i) {
        if (isGiven(i))
            continue;
        if (m_values[size_t(i)] == 0 && m_notes[size_t(i)] == 0)
            continue;
        m_values[size_t(i)] = 0;
        m_notes[size_t(i)] = 0;
        changed.push_back(i);
    }
    m_undo.clear();  // a restart is a fresh start, not one more undo step
    refreshWrong();
    return changed;
}

void SudokuBoard::pushUndo(const std::vector<int> &indices) {
    std::vector<CellState> step;
    step.reserve(indices.size());
    for (int index : indices)
        step.push_back({index, m_values[size_t(index)], m_notes[size_t(index)]});
    m_undo.push_back(std::move(step));
    if (m_undo.size() > kMaxUndoSteps)
        m_undo.pop_front();
}

void SudokuBoard::applyState(const CellState &state) {
    m_values[size_t(state.index)] = state.value;
    m_notes[size_t(state.index)] = state.notes;
}

void SudokuBoard::refreshWrong() {
    m_wrong.fill(false);
    if (!m_checkAsYouGo && filledCount() < Sudoku::kCells)
        return;
    for (int index : Sudoku::wrongCells(m_values, m_puzzle.solution))
        m_wrong[size_t(index)] = true;
}

QJsonObject SudokuBoard::toJson() const {
    QJsonArray notes;
    for (quint16 mask : m_notes)
        notes.append(int(mask));

    QJsonObject json;
    json.insert(QStringLiteral("givens"), gridToJson(m_puzzle.givens));
    json.insert(QStringLiteral("solution"), gridToJson(m_puzzle.solution));
    json.insert(QStringLiteral("values"), gridToJson(m_values));
    json.insert(QStringLiteral("notes"), notes);
    json.insert(QStringLiteral("difficulty"), int(m_puzzle.difficulty));
    json.insert(QStringLiteral("seed"), qint64(m_puzzle.seed));
    return json;
}

bool SudokuBoard::fromJson(const QJsonObject &json, SudokuBoard *board) {
    Puzzle puzzle;
    if (!gridFromJson(json.value(QStringLiteral("givens")), &puzzle.givens))
        return false;
    if (!gridFromJson(json.value(QStringLiteral("solution")), &puzzle.solution))
        return false;
    if (!Sudoku::isComplete(puzzle.solution))
        return false;

    Sudoku::Grid values = Sudoku::emptyGrid();
    if (!gridFromJson(json.value(QStringLiteral("values")), &values))
        return false;
    const QJsonArray notes = json.value(QStringLiteral("notes")).toArray();
    if (notes.size() != Sudoku::kCells)
        return false;

    const int difficulty = json.value(QStringLiteral("difficulty")).toInt(int(Difficulty::Easy));
    puzzle.difficulty = difficulty >= int(Difficulty::Easy) && difficulty <= int(Difficulty::Hard)
        ? Difficulty(difficulty)
        : Difficulty::Easy;
    puzzle.seed = quint32(json.value(QStringLiteral("seed")).toInteger(0));

    board->setPuzzle(puzzle);
    board->m_values = values;
    for (int i = 0; i < Sudoku::kCells; ++i)
        board->m_notes[size_t(i)] = quint16(notes.at(i).toInt(0) & 0x1ff);
    board->refreshWrong();
    return true;
}
