#include "blackjackrules.h"

namespace BlackjackRules {

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

bool validBet(int bet, int bankroll) {
    return bet >= kMinBet && bet <= bankroll && bet % kBetStep == 0;
}

int clampBet(int bet, int bankroll) {
    if (bankroll < kMinBet)
        return 0;
    bet -= bet % kBetStep;
    return qBound(kMinBet, bet, bankroll - bankroll % kBetStep);
}

bool dealerPeeks(const Card &upCard) {
    return upCard.isAce() || upCard.value() == 10;
}

}
