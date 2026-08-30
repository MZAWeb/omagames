#include "fieldtests.h"

#include <QtTest>

#include "field.h"

void FieldTests::fieldStartsAsFrameAroundOpenSea() {
    Field field;
    QCOMPARE(field.width(), 64);
    QCOMPARE(field.height(), 40);
    QCOMPARE(field.interiorCells(), 62 * 38);
    QCOMPARE(field.claimedInterior(), 0);
    QCOMPARE(field.at({0, 0}), Cell::Claimed);
    QCOMPARE(field.at({1, 39}), Cell::Claimed);
    // One cell deep: the second ring in is already open sea.
    QCOMPARE(field.at({1, 1}), Cell::Open);
    QCOMPARE(field.at({62, 38}), Cell::Open);
    QVERIFY(field.isBorder({63, 20}));
    QVERIFY(!field.isBorder({1, 20}));
    QVERIFY(!field.contains({64, 0}));
    // The whole frame is edge, corners included, and nothing inside it is.
    QVERIFY(field.isEdge({0, 0}));
    QVERIFY(field.isEdge({30, 39}));
    QVERIFY(!field.isEdge({1, 1}));
}

void FieldTests::claimTakesTrailAndBallFreeRegions() {
    Field field(12, 10);
    // Vertical trail down column 5 splits the interior in two.
    for (int y = 1; y < 9; ++y)
        field.set({5, y}, Cell::Trail);
    const std::vector<int> claimed = field.claim({QPoint(8, 5)});
    // Trail (8) plus the left region: columns 1..4 × rows 1..8 = 32.
    QCOMPARE(int(claimed.size()), 8 + 32);
    QCOMPARE(field.at({3, 4}), Cell::Claimed);
    QCOMPARE(field.at({5, 4}), Cell::Claimed);
    QCOMPARE(field.at({8, 4}), Cell::Open);
    QCOMPARE(field.trailCells().size(), size_t(0));
}

void FieldTests::claimKeepsEveryRegionWithABall() {
    Field field(12, 10);
    for (int y = 1; y < 9; ++y)
        field.set({5, y}, Cell::Trail);
    const std::vector<int> claimed = field.claim({QPoint(3, 3), QPoint(8, 5)});
    QCOMPARE(int(claimed.size()), 8);
    QCOMPARE(field.at({3, 4}), Cell::Open);
    QCOMPARE(field.at({8, 4}), Cell::Open);

    // Three regions, one ball: the two empty ones go.
    Field three(12, 10);
    for (int y = 1; y < 9; ++y) {
        three.set({4, y}, Cell::Trail);
        three.set({7, y}, Cell::Trail);
    }
    three.claim({QPoint(5, 5)});
    QCOMPARE(three.at({2, 5}), Cell::Claimed);
    QCOMPARE(three.at({5, 5}), Cell::Open);
    QCOMPARE(three.at({9, 5}), Cell::Claimed);
    QCOMPARE(three.claimedInterior(), 8 + 8 + 3 * 8 + 3 * 8);
}

void FieldTests::claimMergesWithExistingGround() {
    Field field(12, 10);
    // Ground already claimed on the left; a trail hugging it closes a pocket.
    for (int y = 2; y < 8; ++y)
        field.set({2, y}, Cell::Claimed);
    field.set({3, 2}, Cell::Trail);
    field.set({3, 3}, Cell::Trail);
    field.set({4, 3}, Cell::Trail);
    field.set({4, 2}, Cell::Trail);
    field.claim({QPoint(8, 6)});
    // The trail cells become ground; no pocket was enclosed (the trail touches
    // the top frame on both ends with nothing inside), so 6 + 4 claimed.
    QCOMPARE(field.claimedInterior(), 10);
    // Now a loop around (6,5): trail cells all around it.
    Field pocket(12, 10);
    const QPoint ring[] = {{5, 4}, {6, 4}, {7, 4}, {7, 5}, {7, 6}, {6, 6}, {5, 6}, {5, 5}};
    for (QPoint p : ring)
        pocket.set(p, Cell::Trail);
    pocket.claim({QPoint(2, 2)});
    QCOMPARE(pocket.at({6, 5}), Cell::Claimed);
    QCOMPARE(pocket.claimedInterior(), 9);
}
