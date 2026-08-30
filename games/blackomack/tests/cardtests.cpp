#include "cardtests.h"

#include <QtTest>

#include "blackjackrules.h"
#include "cards.h"
#include "hand.h"
#include "table.h"
#include "testhelpers.h"

void CardTests::cardValues() {
    QCOMPARE(c(1).value(), 1);
    QCOMPARE(c(10).value(), 10);
    QCOMPARE(c(11).value(), 10);
    QCOMPARE(c(13).value(), 10);
    QCOMPARE(c(12, Suit::Hearts).rankText(), QStringLiteral("Q"));
    QVERIFY(c(12, Suit::Hearts).isRed());
    QVERIFY(!c(12, Suit::Clubs).isRed());
}

void CardTests::handTotals() {
    QCOMPARE(hand({1, 6}).total(), 17);
    QVERIFY(hand({1, 6}).isSoft());
    QCOMPARE(hand({1, 6, 10}).total(), 17);
    QVERIFY(!hand({1, 6, 10}).isSoft());
    QCOMPARE(hand({1, 1}).total(), 12);
    QCOMPARE(hand({1, 1, 9}).total(), 21);
    QCOMPARE(hand({10, 9, 5}).total(), 24);
    QVERIFY(hand({10, 9, 5}).isBust());
    QVERIFY(!hand({10, 9}).isBust());
}

void CardTests::blackjackVsSplitTwentyOne() {
    QVERIFY(hand({1, 13}).isBlackjack());
    QVERIFY(!hand({7, 7, 7}).isBlackjack());
    Hand split = hand({1, 13});
    split.fromSplit = true;
    QVERIFY(!split.isBlackjack());
    QCOMPARE(split.total(), 21);
}

void CardTests::splitAndDoubleEligibility() {
    QVERIFY(hand({8, 8}).canSplit());
    QVERIFY(hand({10, 13}).canSplit());   // equal value, different rank
    QVERIFY(!hand({8, 9}).canSplit());
    QVERIFY(!hand({8, 8, 8}).canSplit());
    Hand again = hand({8, 8});
    again.fromSplit = true;
    QVERIFY(again.canSplit());            // pairs re-split, up to the table's cap
    Hand aces = hand({1, 1});
    aces.fromSplit = true;
    QVERIFY(!aces.canSplit());            // except split aces, which stand pat
    QVERIFY(hand({5, 6}).canDouble());
    QVERIFY(!hand({5, 6, 2}).canDouble());
}

void CardTests::splitAcesFinishAfterOneCard() {
    Hand h = hand({1, 9});
    h.fromSplit = true;
    QVERIFY(h.splitAces());
    QVERIFY(h.isFinished());
    Hand tens = hand({10, 5});
    tens.fromSplit = true;
    QVERIFY(!tens.isFinished());
}

void CardTests::shoeHasSixDecks() {
    Shoe shoe(6, 42);
    QCOMPARE(shoe.remaining(), 312);
    int aces = 0;
    for (int i = 0; i < 312; ++i)
        if (shoe.draw().isAce())
            ++aces;
    QCOMPARE(aces, 24);

    Table table(42);
    QCOMPARE(table.shoeRemaining(), BlackjackRules::kDecks * 52);
    table.placeBets(10);
    table.advance();
    QCOMPARE(table.shoeRemaining(), BlackjackRules::kDecks * 52 - 1);
}

void CardTests::shoeIsDeterministicPerSeed() {
    Shoe a(6, 7), b(6, 7), other(6, 8);
    bool differs = false;
    for (int i = 0; i < 20; ++i) {
        const Card x = a.draw();
        QVERIFY(x == b.draw());
        if (!(x == other.draw()))
            differs = true;
    }
    QVERIFY(differs);
}

void CardTests::shoeReshuffleThreshold() {
    Shoe shoe(6, 1);
    for (int i = 0; i < 234; ++i)
        shoe.draw();
    QVERIFY(!shoe.needsShuffle());     // 78 left = exactly 25%
    shoe.draw();
    QVERIFY(shoe.needsShuffle());
    shoe.shuffle();
    QCOMPARE(shoe.remaining(), 312);
    QVERIFY(!shoe.needsShuffle());
}
