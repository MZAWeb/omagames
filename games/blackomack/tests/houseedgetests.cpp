#include "houseedgetests.h"

#include <QtTest>

#include "basicstrategy.h"
#include "botplayer.h"
#include "hand.h"
#include "table.h"
#include "testhelpers.h"

namespace {

constexpr int kEdgeRounds = 80000;

// Plays `rounds` seeded rounds with a perfect-basic-strategy human and
// returns the net result per unit staked on the initial bet. The human's
// stack is topped back up every round so a losing streak never clamps a bet.
double houseEdge(quint32 seed, int bots, int rounds) {
    constexpr int kBet = 10;
    constexpr int kStack = 1'000'000;
    Table t(seed);
    QRandomGenerator rng(seed);
    for (int i = 0; i < bots; ++i)
        t.addBot(BotPersonality::roll(t.takenNames(), rng), 1000, rng.generate());
    qint64 net = 0;
    for (int r = 0; r < rounds; ++r) {
        t.setHumanBankroll(kStack);
        t.placeBets(kBet);
        while (!t.roundOver()) {
            if (t.waitingForHuman()) {
                const Hand &h = t.human().hands[t.currentHand()];
                t.act(BasicStrategy::decide(h, t.dealerUpCard(),
                                            t.canAct(t.humanSeat(), Table::Action::Double),
                                            t.canAct(t.humanSeat(), Table::Action::Split)));
            } else if (t.waitingForInsurance()) {
                t.declineInsurance();       // the book never insures
            } else if (t.advance().isEmpty()) {
                break;
            }
        }
        net += t.human().bankroll - kStack;
        t.nextRound(rng);
    }
    return double(net) / (double(rounds) * kBet);
}

}

void HouseEdgeTests::houseEdgeStaysInTheKnownBand() {
    QElapsedTimer clock;
    clock.start();
    const double solo = houseEdge(20240815, 0, kEdgeRounds);
    qInfo("house edge, heads-up: %.3f%%", 100 * solo);
    QVERIFY2(solo >= -0.015 && solo <= 0.005, qPrintable(QString::number(solo)));
    const double crowded = houseEdge(19700101, 3, kEdgeRounds);
    qInfo("house edge, three table mates: %.3f%%", 100 * crowded);
    QVERIFY2(crowded >= -0.015 && crowded <= 0.005, qPrintable(QString::number(crowded)));
    qInfo("%lld rounds in %lld ms", 2 * qint64(kEdgeRounds), clock.elapsed());
}
