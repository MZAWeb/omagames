#include "difficulty.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace Difficulty {
namespace {

// Weights in "raw" points. A hole is the thing that makes a mess a mess; each
// block over it is another cell to clear before it opens; a ragged surface
// wastes pieces; rows left is how much work remains.
constexpr double kHoleWeight = 2.5;
constexpr double kCoverWeight = 0.5;
constexpr double kBumpWeight = 0.5;
constexpr double kRowWeight = 1.5;
// Height costs nothing while there is room and a lot once there is not.
constexpr int kSafeHeight = 10;
constexpr double kDangerWeight = 3.0;
// The raw sum at which the rating reaches about 63; the curve flattens after
// that, so a hopeless board rates near 100 without a wall of equal 100s.
// Tuned so a fresh deal rates about 10 to 80, most of them in the middle,
// which leaves room above for a player who makes it worse.
constexpr double kScale = 110.0;

}  // namespace

Features measure(const Board &board, const std::vector<int> &dealtRows) {
    Features features;
    features.rowsLeft = int(dealtRows.size());
    int heights[Board::kWidth];
    for (int x = 0; x < Board::kWidth; ++x) {
        int top = Board::kHeight;
        int filledAbove = 0;
        for (int y = 0; y < Board::kHeight; ++y) {
            if (board.at({x, y}) != PieceType::None) {
                top = std::min(top, y);
                ++filledAbove;
            } else if (filledAbove > 0) {
                ++features.holes;
                features.cover += filledAbove;
            }
        }
        heights[x] = Board::kHeight - top;
        features.height = std::max(features.height, heights[x]);
    }
    for (int x = 1; x < Board::kWidth; ++x)
        features.bumpiness += std::abs(heights[x] - heights[x - 1]);
    return features;
}

int rate(const Features &f) {
    if (f.rowsLeft == 0)
        return kMin;
    const int danger = std::max(0, f.height - kSafeHeight);
    const double raw = kHoleWeight * f.holes + kCoverWeight * f.cover + kBumpWeight * f.bumpiness
                       + kRowWeight * f.rowsLeft + kDangerWeight * danger * danger;
    const double scaled = kMax * (1.0 - std::exp(-raw / kScale));
    return std::clamp(int(std::lround(scaled)), kMin, kMax);
}

}  // namespace Difficulty
