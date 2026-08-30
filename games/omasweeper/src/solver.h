#pragma once

#include <QPoint>
#include <vector>

#include "board.h"

enum class Technique { Singles, Subsets, Enumeration };

// Cells the solver can prove from what is visible, and the cheapest
// technique that proved them.
struct Deduction {
    std::vector<int> safe;
    std::vector<int> mines;
    Technique technique = Technique::Singles;

    bool empty() const { return safe.empty() && mines.empty(); }
};

struct SolveStats {
    // Rounds of deduce-and-apply, and how many each technique settled.
    int rounds = 0;
    int singles = 0;
    int subsets = 0;
    int enumerations = 0;
    // True when the game did not finish: the position needs a guess.
    bool stuck = false;
    Status status = Status::Ready;
};

// A logical Minesweeper solver that only sees what a player sees: revealed
// numbers, its own flags, and the remaining mine count. It never looks at
// hidden cells. Three techniques, tried cheapest first each round:
//
// 1. Singles. A number whose flags already satisfy it makes its other hidden
//    neighbours safe; a number whose hidden neighbours are exactly the mines
//    it still needs makes them all mines.
// 2. Subsets. For two numbers A ⊂ B (every hidden cell of A is also in B),
//    the cells only in B hold exactly needed(B) − needed(A) mines: zero means
//    they are safe, all of them means they are mines. This is the classic
//    1-2-1 / 1-2 pattern family.
// 3. Enumeration. The frontier (every hidden cell next to a number) is split
//    into connected groups and each group's consistent mine assignments are
//    enumerated exactly (see frontier.h), within a node budget. A cell that
//    is a mine in every assignment is a mine; safe in every one is safe.
//    Assignments are bucketed by mine count and cross-checked against the
//    mines still unaccounted for, so the endgame is covered too: when the
//    frontier must take every remaining mine, the cells behind it are safe,
//    and when it can take none, they are mines. A group over budget is
//    treated as "anything goes" for the count check and yields no deductions
//    — the solver stays sound, it just may report stuck.
//
// Techniques 1 and 2 are strictly implied by 3; they exist because they are
// orders of magnitude cheaper and settle most of a board.
class Solver {
public:
    // Search nodes allowed per frontier group before it is given up on.
    static constexpr int kNodeBudget = 1 << 20;

    // What can be proved right now; nothing is changed.
    static Deduction deduce(const Board &board, int nodeBudget = kNodeBudget);

    // Plays `board` (flagging proven mines, revealing proven safe cells)
    // until it is won or no deduction remains. Deterministic.
    static SolveStats solve(Board &board, int nodeBudget = kNodeBudget);

    // Whether logic alone wins `board` after opening `firstClick`. Works on
    // a copy; the board must have its mines placed.
    static bool isSolvableWithoutGuessing(Board board, QPoint firstClick, int nodeBudget = kNodeBudget);
};
