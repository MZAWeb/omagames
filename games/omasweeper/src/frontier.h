#pragma once

#include <QtGlobal>
#include <vector>

// One revealed number seen from the solver's side: the hidden, unflagged
// neighbours it constrains and how many of them are mines.
struct Constraint {
    std::vector<int> cells;
    int needed = 0;
};

// Exact enumeration over one connected group of constraints (constraints
// that share a cell are linked). The group's cells are assigned mine/safe by
// backtracking, pruning as soon as a constraint can no longer be met; every
// consistent assignment is counted, bucketed by how many mines it uses:
//   total[k]           assignments with k mines in the group
//   mineCount[c][k]    of those, how many have cell c as a mine
// The mine buckets let the caller cross-check groups against the board's
// remaining mine count. `complete` is false when the node budget ran out, in
// which case the counts are partial and must not be used for deductions.
struct FrontierGroup {
    std::vector<int> cells;
    std::vector<int> constraints;
    std::vector<qint64> total;
    std::vector<std::vector<qint64>> mineCount;
    bool complete = false;

    int minMines() const;
    int maxMines() const;
};

namespace Frontier {

// Cells addressed by anything in `constraints` are split into groups; each
// group is enumerated with at most `nodeBudget` search nodes.
std::vector<FrontierGroup> enumerate(const std::vector<Constraint> &constraints, int nodeBudget);

}  // namespace Frontier
