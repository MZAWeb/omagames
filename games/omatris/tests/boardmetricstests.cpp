#include "boardmetricstests.h"

#include <QtTest>

#include "boardmetrics.h"
#include "enginefixture.h"

using namespace EngineFixture;

void BoardMetricsTests::anEmptyBoardMeasuresNothing() {
    const BoardMetrics metrics = BoardMetrics::of(Board());
    QCOMPARE(metrics.maxHeight, 0);
    QCOMPARE(metrics.holes, 0);
    QCOMPARE(metrics.bumpiness, 0);
    for (int height : metrics.heights)
        QCOMPARE(height, 0);
}

void BoardMetricsTests::heightsHolesAndBumpiness() {
    Board board;
    // Column 0 three high over a hole, column 1 one high, column 5 two high
    // with two holes under it.
    board.set({0, kBottom - 2}, PieceType::I);
    board.set({0, kBottom - 1}, PieceType::I);
    board.set({1, kBottom}, PieceType::O);
    board.set({5, kBottom - 1}, PieceType::T);
    const BoardMetrics metrics = BoardMetrics::of(board);
    QCOMPARE(metrics.heights[0], 3);
    QCOMPARE(metrics.heights[1], 1);
    QCOMPARE(metrics.heights[2], 0);
    QCOMPARE(metrics.heights[5], 2);
    QCOMPARE(metrics.maxHeight, 3);
    QCOMPARE(metrics.holes, 2);
    // |3-1| + |1-0| + 0 + 0 + |0-2| + |2-0| + 0 + 0 + 0
    QCOMPARE(metrics.bumpiness, 7);
}
