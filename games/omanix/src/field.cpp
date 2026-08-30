#include "field.h"

#include <QtGlobal>

Field::Field(int width, int height) : m_width(width), m_height(height) {
    reset();
}

bool Field::contains(QPoint p) const {
    return p.x() >= 0 && p.y() >= 0 && p.x() < m_width && p.y() < m_height;
}

bool Field::isBorder(QPoint p) const {
    return p.x() < kBorder || p.y() < kBorder || p.x() >= m_width - kBorder || p.y() >= m_height - kBorder;
}

bool Field::isEdge(QPoint p) const {
    if (!contains(p) || at(p) != Cell::Claimed)
        return false;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0)
                continue;
            const QPoint n {p.x() + dx, p.y() + dy};
            if (contains(n) && at(n) != Cell::Claimed)
                return true;
        }
    }
    return false;
}

std::vector<int> Field::openRegions() const {
    std::vector<int> region(size_t(cellCount()), kNoRegion);
    std::vector<int> stack;
    int next = 0;
    for (int i = 0; i < cellCount(); ++i) {
        if (m_cells[size_t(i)] == Cell::Claimed || region[size_t(i)] != kNoRegion)
            continue;
        const int id = next++;
        region[size_t(i)] = id;
        stack.push_back(i);
        while (!stack.empty()) {
            const QPoint p = point(stack.back());
            stack.pop_back();
            const QPoint neighbours[] = {{p.x() + 1, p.y()}, {p.x() - 1, p.y()}, {p.x(), p.y() + 1}, {p.x(), p.y() - 1}};
            for (QPoint n : neighbours) {
                if (!contains(n) || at(n) == Cell::Claimed)
                    continue;
                const int ni = index(n);
                if (region[size_t(ni)] != kNoRegion)
                    continue;
                region[size_t(ni)] = id;
                stack.push_back(ni);
            }
        }
    }
    return region;
}

void Field::reset() {
    m_cells.assign(size_t(cellCount()), Cell::Open);
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            if (isBorder({x, y}))
                set({x, y}, Cell::Claimed);
        }
    }
}

int Field::interiorCells() const {
    return (m_width - 2 * kBorder) * (m_height - 2 * kBorder);
}

int Field::claimedInterior() const {
    int count = 0;
    for (int y = kBorder; y < m_height - kBorder; ++y) {
        for (int x = kBorder; x < m_width - kBorder; ++x) {
            if (at({x, y}) == Cell::Claimed)
                ++count;
        }
    }
    return count;
}

double Field::claimedPercent() const {
    return 100.0 * claimedInterior() / interiorCells();
}

std::vector<int> Field::trailCells() const {
    std::vector<int> trail;
    for (int i = 0; i < cellCount(); ++i) {
        if (m_cells[size_t(i)] == Cell::Trail)
            trail.push_back(i);
    }
    return trail;
}

std::vector<int> Field::clearTrail() {
    std::vector<int> wiped = trailCells();
    for (int i : wiped)
        m_cells[size_t(i)] = Cell::Open;
    return wiped;
}

std::vector<int> Field::claim(const std::vector<QPoint> &balls) {
    std::vector<int> claimed = trailCells();
    for (int i : claimed)
        m_cells[size_t(i)] = Cell::Claimed;

    // Everything a ball can reach (four-connected, since balls never squeeze
    // between two diagonal blockers) stays open; the rest is ours.
    std::vector<bool> reachable(size_t(cellCount()), false);
    std::vector<int> stack;
    for (QPoint ball : balls) {
        if (!contains(ball) || at(ball) != Cell::Open)
            continue;
        const int start = index(ball);
        if (reachable[size_t(start)])
            continue;
        reachable[size_t(start)] = true;
        stack.push_back(start);
        while (!stack.empty()) {
            const int current = stack.back();
            stack.pop_back();
            const QPoint p = point(current);
            const QPoint neighbours[] = {{p.x() + 1, p.y()}, {p.x() - 1, p.y()}, {p.x(), p.y() + 1}, {p.x(), p.y() - 1}};
            for (QPoint n : neighbours) {
                if (!contains(n) || at(n) != Cell::Open)
                    continue;
                const int ni = index(n);
                if (reachable[size_t(ni)])
                    continue;
                reachable[size_t(ni)] = true;
                stack.push_back(ni);
            }
        }
    }
    for (int i = 0; i < cellCount(); ++i) {
        if (m_cells[size_t(i)] == Cell::Open && !reachable[size_t(i)]) {
            m_cells[size_t(i)] = Cell::Claimed;
            claimed.push_back(i);
        }
    }
    return claimed;
}
