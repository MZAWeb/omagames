#pragma once

#include <QPoint>
#include <array>

// The seven tetrominoes, in the order the guideline lists them. `None` is
// also what an empty cell of a board holds.
enum class PieceType : quint8 { I, J, L, O, S, T, Z, None };
constexpr int kPieceCount = 7;

// How a T came to rest: an ordinary landing, a mini T-spin or a full one.
enum class Spin : quint8 { None, Mini, Full };

using PieceCells = std::array<QPoint, 4>;

// A tetromino on the board: its type, its Super Rotation System state (0 is
// spawn, then one clockwise quarter turn each) and the top-left corner of the
// box it rotates inside. Board coordinates run x right, y down.
struct Placement {
    PieceType type = PieceType::None;
    int rotation = 0;
    QPoint origin;

    PieceCells cells() const;
    Placement moved(int dx, int dy) const;
};

namespace Piece {

constexpr int kStates = 4;
constexpr int kMaxKicks = 5;
// The last kick of a table is the one that lifts a mini T-spin to a full one.
constexpr int kSpinKick = kMaxKicks - 1;

// The box a piece rotates inside: 4 for I and O, 3 for the other five.
int boxSize(PieceType type);
const PieceCells &cells(PieceType type, int rotation);
// Where a piece enters: its box centred over the columns, left of centre when
// the two cannot be halved evenly.
int spawnColumn(PieceType type, int boardWidth);
// One quarter turn: +1 clockwise, -1 counter-clockwise.
int turn(int rotation, int quarters);
// The SRS wall kicks tried, in order, when rotating `from` -> `to`.
const std::array<QPoint, kMaxKicks> &kicks(PieceType type, int from, int to);

}  // namespace Piece
