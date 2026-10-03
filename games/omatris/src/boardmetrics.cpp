#include "boardmetrics.h"

#include <algorithm>
#include <cstdlib>

BoardMetrics BoardMetrics::of(const Board &board) {
    BoardMetrics metrics;
    for (int x = 0; x < Board::kWidth; ++x) {
        int height = 0;
        for (int y = 0; y < Board::kHeight; ++y) {
            if (board.at(QPoint(x, y)) == PieceType::None) {
                if (height > 0)
                    ++metrics.holes;
            } else if (height == 0) {
                height = Board::kHeight - y;
            }
        }
        metrics.heights[size_t(x)] = height;
        metrics.maxHeight = std::max(metrics.maxHeight, height);
        if (x > 0)
            metrics.bumpiness += std::abs(height - metrics.heights[size_t(x - 1)]);
    }
    return metrics;
}
