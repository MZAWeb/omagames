#pragma once

#include <array>
#include <vector>

#include "piece.h"

// The playfield: ten columns, twenty visible rows, and four hidden rows above
// them where pieces enter and where a wall kick has room to lift one. Cells
// remember which tetromino filled them, so the renderer keeps the guideline's
// colour per piece.
class Board {
public:
    static constexpr int kWidth = 10;
    static constexpr int kVisibleHeight = 20;
    static constexpr int kHiddenRows = 4;
    static constexpr int kHeight = kVisibleHeight + kHiddenRows;
    static constexpr int kCellCount = kWidth * kHeight;

    Board();

    static int index(QPoint p) { return p.y() * kWidth + p.x(); }
    static QPoint point(int index) { return {index % kWidth, index / kWidth}; }
    static bool inside(QPoint p);
    // A row the player can see.
    static bool visible(QPoint p) { return p.y() >= kHiddenRows; }

    PieceType at(QPoint p) const { return m_cells[size_t(index(p))]; }
    PieceType at(int index) const { return m_cells[size_t(index)]; }
    void set(QPoint p, PieceType type) { m_cells[size_t(index(p))] = type; }
    void clear();
    bool empty() const;

    // Off the field or already filled: the only test collision needs.
    bool blocked(QPoint p) const;
    bool fits(const Placement &placement) const;
    void lock(const Placement &placement);

    // The full rows, top to bottom.
    std::vector<int> fullRows() const;
    // Removes them; everything above each one falls a row.
    void clearRows(const std::vector<int> &rows);

private:
    std::array<PieceType, size_t(kCellCount)> m_cells;
};
