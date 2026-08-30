#include "frontier.h"

#include <algorithm>
#include <deque>
#include <map>

namespace {

// Backtracking state for one group. Cells are visited in the order they were
// discovered (a breadth-first walk over shared constraints), so each
// constraint fills up early and prunes the search close to its root.
class Enumerator {
public:
    Enumerator(FrontierGroup &group, const std::vector<Constraint> &constraints, int budget)
        : m_group(group), m_budget(budget) {
        const size_t n = group.cells.size();
        for (size_t i = 0; i < n; ++i)
            m_position[group.cells[i]] = int(i);
        m_touching.resize(n);
        for (int ci : group.constraints) {
            Local local;
            local.needed = constraints[size_t(ci)].needed;
            local.size = int(constraints[size_t(ci)].cells.size());
            m_locals.push_back(local);
            for (int c : constraints[size_t(ci)].cells)
                m_touching[size_t(m_position[c])].push_back(int(m_locals.size()) - 1);
        }
        m_assigned.assign(n, false);
        group.total.assign(n + 1, 0);
        group.mineCount.assign(n, std::vector<qint64>(n + 1, 0));
    }

    bool run() { return search(0, 0); }

private:
    struct Local {
        int needed = 0;
        int size = 0;
        int mines = 0;
        int assigned = 0;
    };

    bool feasible(const Local &l) const { return l.mines <= l.needed && l.needed - l.mines <= l.size - l.assigned; }

    bool search(size_t at, int mines) {
        if (++m_nodes > m_budget)
            return false;
        if (at == m_group.cells.size()) {
            ++m_group.total[size_t(mines)];
            for (size_t c = 0; c < at; ++c) {
                if (m_assigned[c])
                    ++m_group.mineCount[c][size_t(mines)];
            }
            return true;
        }
        for (int value = 0; value <= 1; ++value) {
            bool ok = true;
            for (int li : m_touching[at]) {
                Local &l = m_locals[size_t(li)];
                l.mines += value;
                ++l.assigned;
                ok = ok && feasible(l);
            }
            m_assigned[at] = value == 1;
            if (ok && !search(at + 1, mines + value))
                return false;
            for (int li : m_touching[at]) {
                Local &l = m_locals[size_t(li)];
                l.mines -= value;
                --l.assigned;
            }
        }
        return true;
    }

    FrontierGroup &m_group;
    int m_budget;
    int m_nodes = 0;
    std::map<int, int> m_position;
    std::vector<Local> m_locals;
    std::vector<std::vector<int>> m_touching;
    std::vector<bool> m_assigned;
};

}  // namespace

int FrontierGroup::minMines() const {
    for (size_t k = 0; k < total.size(); ++k) {
        if (total[k] > 0)
            return int(k);
    }
    return 0;
}

int FrontierGroup::maxMines() const {
    for (size_t k = total.size(); k > 0; --k) {
        if (total[k - 1] > 0)
            return int(k - 1);
    }
    return 0;
}

std::vector<FrontierGroup> Frontier::enumerate(const std::vector<Constraint> &constraints, int nodeBudget) {
    std::map<int, std::vector<int>> byCell;
    for (size_t ci = 0; ci < constraints.size(); ++ci) {
        for (int c : constraints[ci].cells)
            byCell[c].push_back(int(ci));
    }

    std::vector<FrontierGroup> groups;
    std::vector<bool> seen(constraints.size(), false);
    std::map<int, bool> cellSeen;
    for (size_t start = 0; start < constraints.size(); ++start) {
        if (seen[start])
            continue;
        FrontierGroup group;
        std::deque<int> queue{int(start)};
        seen[start] = true;
        while (!queue.empty()) {
            const int ci = queue.front();
            queue.pop_front();
            group.constraints.push_back(ci);
            for (int c : constraints[size_t(ci)].cells) {
                if (cellSeen[c])
                    continue;
                cellSeen[c] = true;
                group.cells.push_back(c);
                for (int other : byCell[c]) {
                    if (!seen[size_t(other)]) {
                        seen[size_t(other)] = true;
                        queue.push_back(other);
                    }
                }
            }
        }
        Enumerator enumerator(group, constraints, nodeBudget);
        group.complete = enumerator.run();
        groups.push_back(std::move(group));
    }
    return groups;
}
