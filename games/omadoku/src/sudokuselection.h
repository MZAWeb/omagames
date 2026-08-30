#pragma once

#include <QVariantList>

#include <vector>

// The cell the keyboard acts on and the cells that came along with it.
//
// It knows the shape of the grid and the rules of picking cells, and nothing
// about screens, boards or signals: no QObject, so every rule below is
// testable headlessly. Each mutator answers whether anything actually
// changed, which is what lets the bridge emit exactly once.
class SudokuSelection {
public:
    // -1 when nothing is selected. The cursor is always one of indices().
    int cursor() const { return m_cursor; }
    // The whole selection in the order the cells joined it — a single cell
    // most of the time.
    const std::vector<int> &indices() const { return m_indices; }
    QVariantList toVariantList() const;

    // Collapse onto one cell: a plain click, a plain arrow. -1 selects
    // nothing; anything off the grid is ignored.
    bool select(int index);
    // Ctrl+click: add the cell to the selection, or drop it out again.
    bool toggle(int index);
    bool moveCursor(int deltaRow, int deltaColumn);
    // Shift+arrows: move the cursor and take every cell it passes over along.
    bool extend(int deltaRow, int deltaColumn);
    // Back to the cursor alone, which stays where it is.
    bool collapse();

private:
    // The cell `deltaRow`/`deltaColumn` away, clamped to the grid: movement
    // stops at the edges because wrapping makes arrow keys feel lost.
    static int stepped(int index, int deltaRow, int deltaColumn);

    std::vector<int> m_indices;
    int m_cursor = -1;
};
