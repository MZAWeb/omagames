#include "difficulty.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace Difficulty {
namespace {

// The estimate is in pieces: every four cells to fill is one piece, and a
// buried cell costs this many more, for the clears it takes to open it.
constexpr double kCellsPerPiece = 4.0;
constexpr double kBuriedCost = 0.75;
// Height costs nothing while there is room and a lot once there is not.
constexpr int kSafeHeight = 12;
constexpr double kDangerCost = 0.5;
// Rating points per piece of work. Linear, so a buried gap moves the number
// as much as it moves the work. Tuned so a fresh deal rates about 10 to 70,
// which leaves room above for a player who makes it worse.
constexpr double kPointsPerPiece = 2.2;

bool filled(const Board &board, int x, int y) {
    return board.at({x, y}) != PieceType::None;
}

}  // namespace

Features measure(const Board &board, const std::vector<int> &dealtRows) {
    std::array<bool, size_t(Board::kHeight)> required {};
    for (int y : dealtRows)
        required[size_t(y)] = true;
    // A row with a block over a gap in a required row is required too; a row
    // taken in that way can bury gaps of its own, so go until nothing changes.
    for (bool grew = true; grew;) {
        grew = false;
        for (int y = 0; y < Board::kHeight; ++y) {
            if (!required[size_t(y)])
                continue;
            for (int x = 0; x < Board::kWidth; ++x) {
                if (filled(board, x, y))
                    continue;
                for (int above = 0; above < y; ++above) {
                    if (filled(board, x, above) && !required[size_t(above)]) {
                        required[size_t(above)] = true;
                        grew = true;
                    }
                }
            }
        }
    }

    Features features;
    for (int x = 0; x < Board::kWidth; ++x) {
        bool covered = false;
        for (int y = 0; y < Board::kHeight; ++y) {
            if (filled(board, x, y)) {
                if (!covered)
                    features.height = std::max(features.height, Board::kHeight - y);
                covered = true;
            } else if (required[size_t(y)]) {
                ++features.cellsToFill;
                features.buried += covered ? 1 : 0;
            }
        }
    }
    for (bool row : required)
        features.rowsToClear += row ? 1 : 0;
    return features;
}

int rate(const Features &f) {
    if (f.rowsToClear == 0)
        return kMin;
    const int danger = std::max(0, f.height - kSafeHeight);
    const double pieces = f.cellsToFill / kCellsPerPiece + kBuriedCost * f.buried + kDangerCost * danger * danger;
    return std::clamp(int(std::lround(kPointsPerPiece * pieces)), kMin, kMax);
}

}  // namespace Difficulty
