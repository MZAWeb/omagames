#include "challenge.h"

#include <QRandomGenerator>
#include <algorithm>

#include "bag.h"

namespace Challenge {
namespace {

// How much of the covered rows the careless pieces fill before stopping, in
// percent. Much less reads as a few stray pieces; much more leaves no room
// for the next one to fall into.
constexpr int kMinFill = 55;
constexpr int kMaxFill = 70;
// A careless player is not a random one: of a few spots that came to mind,
// the piece goes to the lowest. That keeps the stack a jagged heap with holes
// in it rather than a row of towers.
constexpr int kSpotsConsidered = 3;
constexpr int kTriesPerPiece = 24;
constexpr int kPiecesPerAttempt = 80;

// Where a piece dropped from the top of the well at this column and turn
// comes to rest, if it fits there at all.
bool land(const Board &board, PieceType type, int rotation, int column, Placement *out) {
    Placement placement {type, rotation, {column, 0}};
    if (!board.fits(placement))
        return false;
    while (board.fits(placement.moved(0, 1)))
        placement = placement.moved(0, 1);
    *out = placement;
    return true;
}

int highestCell(const Placement &placement) {
    int top = Board::kHeight;
    for (QPoint cell : placement.cells())
        top = std::min(top, cell.y());
    return top;
}

// A spot that stays inside the rows the stack may cover and completes none of
// them: a careless player still never clears a line by accident here, or the
// mess would not be the one that was dealt.
bool acceptable(const Board &board, const Placement &placement, int topRow) {
    if (highestCell(placement) < topRow)
        return false;
    Board after = board;
    after.lock(placement);
    return after.fullRows().empty();
}

int filledCells(const Board &board, int topRow) {
    int filled = 0;
    for (int y = topRow; y < Board::kHeight; ++y) {
        for (int x = 0; x < Board::kWidth; ++x)
            filled += board.at({x, y}) != PieceType::None ? 1 : 0;
    }
    return filled;
}

bool rowHasCells(const Board &board, int y) {
    for (int x = 0; x < Board::kWidth; ++x) {
        if (board.at({x, y}) != PieceType::None)
            return true;
    }
    return false;
}

// One careless piece: the lowest of a few spots that pass. False when none did.
bool dropOne(Board &board, PieceType type, int topRow, QRandomGenerator &rng) {
    Placement best;
    int found = 0;
    for (int i = 0; i < kTriesPerPiece && found < kSpotsConsidered; ++i) {
        Placement spot;
        const int rotation = int(rng.bounded(Piece::kStates));
        // The widest box hangs three columns past either wall.
        const int column = rng.bounded(-3, Board::kWidth);
        if (!land(board, type, rotation, column, &spot) || !acceptable(board, spot, topRow))
            continue;
        if (found == 0 || highestCell(spot) > highestCell(best))
            best = spot;
        ++found;
    }
    if (found == 0)
        return false;
    board.lock(best);
    return true;
}

}  // namespace

std::vector<int> build(Board &board, quint32 seed) {
    QRandomGenerator rng(seed);
    const int rows = rng.bounded(kMinRows, kMaxRows + 1);
    const int topRow = Board::kHeight - rows;
    const int target = rows * Board::kWidth * rng.bounded(kMinFill, kMaxFill + 1) / 100;
    // An unlucky run of pieces can wall off the top row before it is reached;
    // the next attempt starts over with the same size of mess.
    for (;;) {
        board.clear();
        Bag bag(rng.generate());
        for (int piece = 0; piece < kPiecesPerAttempt; ++piece) {
            dropOne(board, bag.take(), topRow, rng);
            if (rowHasCells(board, topRow) && filledCells(board, topRow) >= target) {
                std::vector<int> covered;
                for (int y = topRow; y < Board::kHeight; ++y)
                    covered.push_back(y);
                return covered;
            }
        }
    }
}

std::vector<int> afterClear(const std::vector<int> &tracked, const std::vector<int> &cleared) {
    std::vector<int> moved;
    for (int y : tracked) {
        if (std::find(cleared.begin(), cleared.end(), y) != cleared.end())
            continue;
        const auto below = std::count_if(cleared.begin(), cleared.end(), [y](int c) { return c > y; });
        moved.push_back(y + int(below));
    }
    return moved;
}

}  // namespace Challenge
