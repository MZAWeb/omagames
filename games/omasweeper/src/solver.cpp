#include "solver.h"

#include <algorithm>
#include <map>

#include "frontier.h"

namespace {

// Deduction accumulator that keeps each cell at most once.
class Found {
public:
    explicit Found(int cells) : m_mark(size_t(cells), 0) {}

    void safe(int cell) { add(cell, m_deduction.safe); }
    void mine(int cell) { add(cell, m_deduction.mines); }
    bool empty() const { return m_deduction.empty(); }
    Deduction take(Technique technique) {
        m_deduction.technique = technique;
        return std::move(m_deduction);
    }

private:
    void add(int cell, std::vector<int> &into) {
        if (!m_mark[size_t(cell)]) {
            m_mark[size_t(cell)] = 1;
            into.push_back(cell);
        }
    }

    std::vector<char> m_mark;
    Deduction m_deduction;
};

std::vector<Constraint> constraints(const Board &board) {
    std::vector<Constraint> result;
    for (int i = 0; i < board.cellCount(); ++i) {
        const Cell &cell = board.cell(i);
        if (cell.state != CellState::Revealed || cell.adjacent == 0)
            continue;
        Constraint constraint;
        constraint.needed = cell.adjacent;
        for (int n : board.neighbours(i)) {
            const CellState state = board.cell(n).state;
            if (state == CellState::Hidden)
                constraint.cells.push_back(n);
            else if (state == CellState::Flagged)
                --constraint.needed;
        }
        if (!constraint.cells.empty())
            result.push_back(std::move(constraint));
    }
    return result;
}

void singles(const std::vector<Constraint> &constraints, Found &found) {
    for (const Constraint &c : constraints) {
        if (c.needed == 0) {
            for (int cell : c.cells)
                found.safe(cell);
        } else if (c.needed == int(c.cells.size())) {
            for (int cell : c.cells)
                found.mine(cell);
        }
    }
}

void subsets(const std::vector<Constraint> &constraints, Found &found) {
    std::map<int, std::vector<int>> byCell;
    for (size_t ci = 0; ci < constraints.size(); ++ci) {
        for (int c : constraints[ci].cells)
            byCell[c].push_back(int(ci));
    }
    for (size_t a = 0; a < constraints.size(); ++a) {
        const Constraint &inner = constraints[a];
        // Candidates for a superset are the constraints sharing the first
        // cell of `inner`; they are ordered, so a cell's list has no dupes.
        for (int bi : byCell[inner.cells.front()]) {
            const Constraint &outer = constraints[size_t(bi)];
            if (bi == int(a) || outer.cells.size() <= inner.cells.size())
                continue;
            if (!std::includes(outer.cells.begin(), outer.cells.end(), inner.cells.begin(), inner.cells.end()))
                continue;
            const int diffNeeded = outer.needed - inner.needed;
            const int diffSize = int(outer.cells.size() - inner.cells.size());
            if (diffNeeded != 0 && diffNeeded != diffSize)
                continue;
            for (int cell : outer.cells) {
                if (std::binary_search(inner.cells.begin(), inner.cells.end(), cell))
                    continue;
                if (diffNeeded == 0)
                    found.safe(cell);
                else
                    found.mine(cell);
            }
        }
    }
}

// Sums of mines the groups other than `skip` (or all of them, skip = -1) can
// take together, as a membership table indexed by sum.
std::vector<bool> reachableSums(const std::vector<FrontierGroup> &groups, int skip, int limit) {
    std::vector<bool> sums(size_t(limit) + 1, false);
    sums[0] = true;
    for (size_t g = 0; g < groups.size(); ++g) {
        if (int(g) == skip)
            continue;
        std::vector<bool> next(sums.size(), false);
        const int lo = groups[g].complete ? groups[g].minMines() : 0;
        const int hi = groups[g].complete ? groups[g].maxMines() : int(groups[g].cells.size());
        for (int s = 0; s <= limit; ++s) {
            if (!sums[size_t(s)])
                continue;
            for (int k = lo; k <= hi && s + k <= limit; ++k) {
                if (!groups[g].complete || groups[g].total[size_t(k)] > 0)
                    next[size_t(s + k)] = true;
            }
        }
        sums = std::move(next);
    }
    return sums;
}

void enumeration(const Board &board, const std::vector<Constraint> &constraints, int nodeBudget, Found &found) {
    const std::vector<FrontierGroup> groups = Frontier::enumerate(constraints, nodeBudget);
    int hidden = 0;
    for (const Cell &cell : board.cells())
        hidden += cell.state == CellState::Hidden ? 1 : 0;
    size_t frontierCells = 0;
    for (const FrontierGroup &g : groups)
        frontierCells += g.cells.size();
    const int behind = hidden - int(frontierCells);
    const int remaining = board.remainingMines();
    // Mines the frontier must hold overall: everything left minus what fits
    // behind it, and never more than everything left.
    const int frontierMin = std::max(0, remaining - behind);
    const int frontierMax = remaining;

    for (size_t g = 0; g < groups.size(); ++g) {
        const FrontierGroup &group = groups[g];
        if (!group.complete)
            continue;
        const std::vector<bool> others = reachableSums(groups, int(g), frontierMax);
        std::vector<int> feasible;
        for (int k = group.minMines(); k <= group.maxMines(); ++k) {
            if (group.total[size_t(k)] == 0)
                continue;
            for (int s = std::max(0, frontierMin - k); s + k <= frontierMax; ++s) {
                if (others[size_t(s)]) {
                    feasible.push_back(k);
                    break;
                }
            }
        }
        if (feasible.empty())
            continue;
        for (size_t c = 0; c < group.cells.size(); ++c) {
            bool alwaysMine = true;
            bool neverMine = true;
            for (int k : feasible) {
                alwaysMine = alwaysMine && group.mineCount[c][size_t(k)] == group.total[size_t(k)];
                neverMine = neverMine && group.mineCount[c][size_t(k)] == 0;
            }
            if (alwaysMine)
                found.mine(group.cells[c]);
            else if (neverMine)
                found.safe(group.cells[c]);
        }
    }

    if (behind == 0)
        return;
    const std::vector<bool> all = reachableSums(groups, -1, frontierMax);
    int lo = -1;
    int hi = -1;
    for (int s = frontierMin; s <= frontierMax; ++s) {
        if (!all[size_t(s)])
            continue;
        if (lo < 0)
            lo = s;
        hi = s;
    }
    if (lo < 0)
        return;
    const int behindMax = remaining - lo;
    const int behindMin = remaining - hi;
    if (behindMax != 0 && behindMin != behind)
        return;
    std::map<int, bool> frontier;
    for (const FrontierGroup &g : groups) {
        for (int c : g.cells)
            frontier[c] = true;
    }
    for (int i = 0; i < board.cellCount(); ++i) {
        if (board.cell(i).state != CellState::Hidden || frontier.count(i))
            continue;
        if (behindMax == 0)
            found.safe(i);
        else
            found.mine(i);
    }
}

}  // namespace

Deduction Solver::deduce(const Board &board, int nodeBudget) {
    Found found(board.cellCount());
    if (board.status() == Status::Won || board.status() == Status::Lost || !board.minesPlaced())
        return found.take(Technique::Singles);
    const std::vector<Constraint> all = constraints(board);
    singles(all, found);
    if (!found.empty())
        return found.take(Technique::Singles);
    subsets(all, found);
    if (!found.empty())
        return found.take(Technique::Subsets);
    enumeration(board, all, nodeBudget, found);
    return found.take(Technique::Enumeration);
}

SolveStats Solver::solve(Board &board, int nodeBudget) {
    SolveStats stats;
    while (board.status() == Status::Playing) {
        const Deduction step = deduce(board, nodeBudget);
        if (step.empty()) {
            stats.stuck = true;
            break;
        }
        ++stats.rounds;
        const int count = int(step.safe.size() + step.mines.size());
        switch (step.technique) {
        case Technique::Singles:
            stats.singles += count;
            break;
        case Technique::Subsets:
            stats.subsets += count;
            break;
        case Technique::Enumeration:
            stats.enumerations += count;
            break;
        }
        for (int cell : step.mines)
            board.toggleFlag(board.point(cell));
        for (int cell : step.safe) {
            if (board.reveal(board.point(cell)).lost())
                break;
        }
    }
    stats.status = board.status();
    return stats;
}

bool Solver::isSolvableWithoutGuessing(Board board, QPoint firstClick, int nodeBudget) {
    if (!board.minesPlaced() || board.reveal(firstClick).lost())
        return false;
    return solve(board, nodeBudget).status == Status::Won;
}
