#include "blackjackrules.h"

#include <QtGlobal>

namespace BlackjackRules {
namespace {

// A bet bar reads best in the amounts a chip tray comes in: 1, 2 and 5 at
// every order of magnitude. Both helpers work on that ladder.
constexpr int kNiceLeads[] = {1, 2, 5};
constexpr int kNiceCeiling = 100000000;

int niceMagnitude(double value) {
    int magnitude = 1;
    while (magnitude < kNiceCeiling && value >= 10.0 * magnitude)
        magnitude *= 10;
    return magnitude;
}

// Nearest rung of the ladder, rounding a tie up so a quarter of Ø 60 reads
// Ø 20 rather than the table minimum again.
int niceNearest(double value) {
    const int magnitude = niceMagnitude(value);
    int best = magnitude;
    double bestDelta = qAbs(value - magnitude);
    for (int candidate : {2 * magnitude, 5 * magnitude, 10 * magnitude}) {
        const double delta = qAbs(value - candidate);
        if (delta <= bestDelta) {
            best = candidate;
            bestDelta = delta;
        }
    }
    return best;
}

// The next rung strictly above a value, used to break a tie between presets.
int niceAbove(int value) {
    const int magnitude = niceMagnitude(value);
    for (int lead : kNiceLeads) {
        if (lead * magnitude > value)
            return lead * magnitude;
    }
    return 10 * magnitude;
}

}

bool dealerShouldHit(const Hand &dealer) {
    return dealer.total() < kDealerStandTotal;
}

Outcome outcome(const Hand &player, const Hand &dealer) {
    if (player.isBust())
        return Outcome::Lose;
    if (player.isBlackjack())
        return dealer.isBlackjack() ? Outcome::Push : Outcome::Blackjack;
    if (dealer.isBlackjack() || (!dealer.isBust() && dealer.total() > player.total()))
        return Outcome::Lose;
    if (dealer.isBust() || player.total() > dealer.total())
        return Outcome::Win;
    return Outcome::Push;
}

int payout(const Hand &player, const Hand &dealer) {
    switch (outcome(player, dealer)) {
    case Outcome::Blackjack:
        return player.bet + player.bet * kBlackjackPayoutNumerator / kBlackjackPayoutDenominator;
    case Outcome::Win: return player.bet * 2;
    case Outcome::Push: return player.bet;
    case Outcome::Lose: return 0;
    }
    return 0;
}

int insuranceStake(int bet) {
    return bet / kInsuranceDivisor;
}

int insuranceReturn(int stake, const Hand &dealer) {
    return dealer.isBlackjack() ? stake * (1 + kInsurancePayoutNumerator) : 0;
}

bool validBet(int bet, int bankroll) {
    return bet >= kMinBet && bet <= bankroll && bet % kBetStep == 0;
}

int clampBet(int bet, int bankroll) {
    if (bankroll < kMinBet)
        return 0;
    bet -= bet % kBetStep;
    return qBound(kMinBet, bet, bankroll - bankroll % kBetStep);
}

QVector<int> betPresets(int bankroll) {
    // Shares of the bankroll, in percent: a nibble, a quarter and half of it.
    constexpr int kShares[] = {10, 25, 50};
    const int cap = clampBet(bankroll, bankroll);
    QVector<int> presets;
    if (cap < kMinBet)
        return presets;
    for (int share : kShares) {
        int amount = qBound(kMinBet, niceNearest(bankroll * share / 100.0), cap);
        if (!presets.isEmpty() && amount <= presets.last())
            amount = qMin(niceAbove(presets.last()), cap);   // a collision steps up
        if (presets.isEmpty() || amount > presets.last())
            presets.append(amount);
    }
    return presets;
}

bool dealerPeeks(const Card &upCard) {
    return upCard.isAce() || upCard.value() == 10;
}

}
