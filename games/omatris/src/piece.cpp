#include "piece.h"

namespace {

using Shape = std::array<PieceCells, Piece::kStates>;
using KickTable = std::array<QPoint, Piece::kMaxKicks>;
// One row per quarter turn, in the order transitionIndex() numbers them:
// 0->R, 0->L, R->2, R->0, 2->L, 2->R, L->0, L->2.
using KickSet = std::array<KickTable, 2 * Piece::kStates>;

// The four rotation states of every piece, drawn inside its own box. These
// are the guideline's spawn orientations: I and S/Z flat, J with its nub on
// the left, L on the right, T pointing up, O in the middle two columns.
const Shape kShapes[kPieceCount] = {
    // I
    {{{{{0, 1}, {1, 1}, {2, 1}, {3, 1}}},
      {{{2, 0}, {2, 1}, {2, 2}, {2, 3}}},
      {{{0, 2}, {1, 2}, {2, 2}, {3, 2}}},
      {{{1, 0}, {1, 1}, {1, 2}, {1, 3}}}}},
    // J
    {{{{{0, 0}, {0, 1}, {1, 1}, {2, 1}}},
      {{{1, 0}, {2, 0}, {1, 1}, {1, 2}}},
      {{{0, 1}, {1, 1}, {2, 1}, {2, 2}}},
      {{{1, 0}, {1, 1}, {0, 2}, {1, 2}}}}},
    // L
    {{{{{2, 0}, {0, 1}, {1, 1}, {2, 1}}},
      {{{1, 0}, {1, 1}, {1, 2}, {2, 2}}},
      {{{0, 1}, {1, 1}, {2, 1}, {0, 2}}},
      {{{0, 0}, {1, 0}, {1, 1}, {1, 2}}}}},
    // O: the same two-by-two block whichever way it is turned, so it never
    // shifts and never needs a kick.
    {{{{{1, 0}, {2, 0}, {1, 1}, {2, 1}}},
      {{{1, 0}, {2, 0}, {1, 1}, {2, 1}}},
      {{{1, 0}, {2, 0}, {1, 1}, {2, 1}}},
      {{{1, 0}, {2, 0}, {1, 1}, {2, 1}}}}},
    // S
    {{{{{1, 0}, {2, 0}, {0, 1}, {1, 1}}},
      {{{1, 0}, {1, 1}, {2, 1}, {2, 2}}},
      {{{1, 1}, {2, 1}, {0, 2}, {1, 2}}},
      {{{0, 0}, {0, 1}, {1, 1}, {1, 2}}}}},
    // T
    {{{{{1, 0}, {0, 1}, {1, 1}, {2, 1}}},
      {{{1, 0}, {1, 1}, {2, 1}, {1, 2}}},
      {{{0, 1}, {1, 1}, {2, 1}, {1, 2}}},
      {{{1, 0}, {0, 1}, {1, 1}, {1, 2}}}}},
    // Z
    {{{{{0, 0}, {1, 0}, {1, 1}, {2, 1}}},
      {{{2, 0}, {1, 1}, {2, 1}, {1, 2}}},
      {{{0, 1}, {1, 1}, {1, 2}, {2, 2}}},
      {{{1, 0}, {0, 1}, {1, 1}, {0, 2}}}}},
};

// The guideline wall-kick tables. They are published with y pointing up; the
// board's y grows downward, so every y below is the published one negated —
// the comment on each row is the published pair, for checking against the
// table as it is written everywhere else.
const KickSet kJlstzKicks = {{
    {{{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}}},   // 0->R: (0,0)(-1,0)(-1,+1)(0,-2)(-1,-2)
    {{{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}}},      // 0->L: (0,0)(+1,0)(+1,+1)(0,-2)(+1,-2)
    {{{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}}},     // R->2: (0,0)(+1,0)(+1,-1)(0,+2)(+1,+2)
    {{{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}}},     // R->0: (0,0)(+1,0)(+1,-1)(0,+2)(+1,+2)
    {{{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}}},      // 2->L: (0,0)(+1,0)(+1,+1)(0,-2)(+1,-2)
    {{{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}}},   // 2->R: (0,0)(-1,0)(-1,+1)(0,-2)(-1,-2)
    {{{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}}},  // L->0: (0,0)(-1,0)(-1,-1)(0,+2)(-1,+2)
    {{{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}}},  // L->2: (0,0)(-1,0)(-1,-1)(0,+2)(-1,+2)
}};

const KickSet kIKicks = {{
    {{{0, 0}, {-2, 0}, {1, 0}, {-2, 1}, {1, -2}}},   // 0->R: (0,0)(-2,0)(+1,0)(-2,-1)(+1,+2)
    {{{0, 0}, {-1, 0}, {2, 0}, {-1, -2}, {2, 1}}},   // 0->L: (0,0)(-1,0)(+2,0)(-1,+2)(+2,-1)
    {{{0, 0}, {-1, 0}, {2, 0}, {-1, -2}, {2, 1}}},   // R->2: (0,0)(-1,0)(+2,0)(-1,+2)(+2,-1)
    {{{0, 0}, {2, 0}, {-1, 0}, {2, -1}, {-1, 2}}},   // R->0: (0,0)(+2,0)(-1,0)(+2,+1)(-1,-2)
    {{{0, 0}, {2, 0}, {-1, 0}, {2, -1}, {-1, 2}}},   // 2->L: (0,0)(+2,0)(-1,0)(+2,+1)(-1,-2)
    {{{0, 0}, {1, 0}, {-2, 0}, {1, 2}, {-2, -1}}},   // 2->R: (0,0)(+1,0)(-2,0)(+1,-2)(-2,+1)
    {{{0, 0}, {1, 0}, {-2, 0}, {1, 2}, {-2, -1}}},   // L->0: (0,0)(+1,0)(-2,0)(+1,-2)(-2,+1)
    {{{0, 0}, {-2, 0}, {1, 0}, {-2, 1}, {1, -2}}},   // L->2: (0,0)(-2,0)(+1,0)(-2,-1)(+1,+2)
}};

const KickTable kNoKicks = {{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}};

// Only quarter turns happen, so a transition is its starting state plus which
// way it went.
int transitionIndex(int from, int to) {
    return from * 2 + (to == Piece::turn(from, 1) ? 0 : 1);
}

}  // namespace

PieceCells Placement::cells() const {
    PieceCells out = Piece::cells(type, rotation);
    for (QPoint &cell : out)
        cell += origin;
    return out;
}

Placement Placement::moved(int dx, int dy) const {
    Placement out = *this;
    out.origin += QPoint(dx, dy);
    return out;
}

int Piece::boxSize(PieceType type) {
    return (type == PieceType::I || type == PieceType::O) ? 4 : 3;
}

const PieceCells &Piece::cells(PieceType type, int rotation) {
    return kShapes[int(type)][size_t(turn(rotation, 0))];
}

int Piece::spawnColumn(PieceType type, int boardWidth) {
    return (boardWidth - boxSize(type)) / 2;
}

int Piece::turn(int rotation, int quarters) {
    return ((rotation + quarters) % kStates + kStates) % kStates;
}

const std::array<QPoint, Piece::kMaxKicks> &Piece::kicks(PieceType type, int from, int to) {
    if (type == PieceType::O)
        return kNoKicks;
    const KickSet &set = type == PieceType::I ? kIKicks : kJlstzKicks;
    return set[size_t(transitionIndex(from, to))];
}
