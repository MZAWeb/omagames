#include "sudokuselection.h"

#include <algorithm>

#include "sudoku.h"

QVariantList SudokuSelection::toVariantList() const {
    QVariantList indices;
    indices.reserve(int(m_indices.size()));
    for (int index : m_indices)
        indices.append(index);
    return indices;
}

int SudokuSelection::stepped(int index, int deltaRow, int deltaColumn) {
    const int row = qBound(0, Sudoku::rowOf(index) + deltaRow, Sudoku::kSize - 1);
    const int column = qBound(0, Sudoku::colOf(index) + deltaColumn, Sudoku::kSize - 1);
    return row * Sudoku::kSize + column;
}

bool SudokuSelection::select(int index) {
    if (index < -1 || index >= Sudoku::kCells)
        return false;
    if (index == m_cursor && m_indices.size() <= 1)
        return false;
    m_indices.clear();
    if (index >= 0)
        m_indices.push_back(index);
    m_cursor = index;
    return true;
}

bool SudokuSelection::toggle(int index) {
    if (index < 0 || index >= Sudoku::kCells)
        return false;
    const auto at = std::find(m_indices.begin(), m_indices.end(), index);
    if (at == m_indices.end()) {
        // The cell you just picked is where you are, so the cursor follows.
        m_indices.push_back(index);
        m_cursor = index;
    } else {
        m_indices.erase(at);
        if (m_cursor == index)
            m_cursor = m_indices.empty() ? -1 : m_indices.back();
    }
    return true;
}

bool SudokuSelection::moveCursor(int deltaRow, int deltaColumn) {
    if (m_cursor < 0)
        return select(0);
    return select(stepped(m_cursor, deltaRow, deltaColumn));
}

bool SudokuSelection::extend(int deltaRow, int deltaColumn) {
    if (m_cursor < 0)
        return select(0);
    // One cell at a time, so everything the cursor passes over joins in and
    // not just where it lands.
    const int steps = std::max(std::abs(deltaRow), std::abs(deltaColumn));
    const int rowStep = deltaRow > 0 ? 1 : deltaRow < 0 ? -1 : 0;
    const int columnStep = deltaColumn > 0 ? 1 : deltaColumn < 0 ? -1 : 0;
    bool moved = false;
    for (int step = 0; step < steps; ++step) {
        const int next = stepped(m_cursor, rowStep, columnStep);
        if (next == m_cursor)
            break;  // the edge of the grid
        m_cursor = next;
        if (std::find(m_indices.begin(), m_indices.end(), next) == m_indices.end())
            m_indices.push_back(next);
        moved = true;
    }
    return moved;
}

bool SudokuSelection::collapse() {
    if (m_indices.size() <= 1)
        return false;
    m_indices.assign(1, m_cursor);  // the cursor stays where it is
    return true;
}
