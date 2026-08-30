#include "board.h"

Board::Board() {
    clear();
}

bool Board::inside(QPoint p) {
    return p.x() >= 0 && p.x() < kWidth && p.y() >= 0 && p.y() < kHeight;
}

void Board::clear() {
    m_cells.fill(PieceType::None);
}

bool Board::empty() const {
    for (PieceType cell : m_cells) {
        if (cell != PieceType::None)
            return false;
    }
    return true;
}

bool Board::blocked(QPoint p) const {
    return !inside(p) || at(p) != PieceType::None;
}

bool Board::fits(const Placement &placement) const {
    for (QPoint cell : placement.cells()) {
        if (blocked(cell))
            return false;
    }
    return true;
}

void Board::lock(const Placement &placement) {
    for (QPoint cell : placement.cells()) {
        if (inside(cell))
            set(cell, placement.type);
    }
}

std::vector<int> Board::fullRows() const {
    std::vector<int> rows;
    for (int y = 0; y < kHeight; ++y) {
        bool full = true;
        for (int x = 0; x < kWidth && full; ++x)
            full = at({x, y}) != PieceType::None;
        if (full)
            rows.push_back(y);
    }
    return rows;
}

void Board::clearRows(const std::vector<int> &rows) {
    std::array<bool, size_t(kHeight)> gone {};
    for (int y : rows) {
        if (y >= 0 && y < kHeight)
            gone[size_t(y)] = true;
    }
    // Copy the surviving rows downward, then blank whatever is left on top.
    int write = kHeight - 1;
    for (int y = kHeight - 1; y >= 0; --y) {
        if (gone[size_t(y)])
            continue;
        if (write != y) {
            for (int x = 0; x < kWidth; ++x)
                set({x, write}, at({x, y}));
        }
        --write;
    }
    for (; write >= 0; --write) {
        for (int x = 0; x < kWidth; ++x)
            set({x, write}, PieceType::None);
    }
}
