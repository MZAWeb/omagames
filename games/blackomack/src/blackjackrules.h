#pragma once

#include <QVector>

#include "hand.h"

// Fixed v1 house rules: 6 decks, dealer stands on all 17s, blackjack pays 3:2,
// double on any two cards, splits up to four hands a seat, insurance against a
// dealer ace, no surrender.
namespace BlackjackRules {

constexpr int kDecks = 6;
constexpr int kDealerStandTotal = 17;
constexpr int kBlackjackPayoutNumerator = 3;
constexpr int kBlackjackPayoutDenominator = 2;
constexpr int kInsurancePayoutNumerator = 2;   // insurance pays 2 to 1
constexpr int kInsuranceDivisor = 2;           // and costs half the original bet
constexpr int kMaxHandsPerSeat = 4;            // three splits, aces excepted
constexpr int kMinBet = 10;
constexpr int kBetStep = 10;
constexpr int kStartingBankroll = 1000;
constexpr int kMaxBots = 6;       // seats at the table besides the human
constexpr int kDefaultBots = 4;   // how many sit down at a fresh table

enum class Outcome { Blackjack, Win, Push, Lose };
enum class Action { Hit, Stand, Double, Split };

bool dealerShouldHit(const Hand &dealer);
Outcome outcome(const Hand &player, const Hand &dealer);
// Amount returned to the player for a hand (stake + winnings), 0 on a loss.
int payout(const Hand &player, const Hand &dealer);

// Bet must be a multiple of kBetStep within [kMinBet, bankroll].
bool validBet(int bet, int bankroll);
// Nearest legal bet for a bankroll (0 when the player cannot afford the minimum).
int clampBet(int bet, int bankroll);

// The three one-tap stakes for a bankroll: roughly a tenth, a quarter and a
// half of it, each snapped to the nearest round {1, 2, 5} x 10^n amount and
// held inside [kMinBet, bankroll]. Ascending and distinct, so a bankroll too
// small to separate them yields fewer than three.
QVector<int> betPresets(int bankroll);

// The side bet against a dealer ace: half the original stake, returning the
// stake plus 2 to 1 when the dealer turns over a natural and nothing otherwise.
int insuranceStake(int bet);
int insuranceReturn(int stake, const Hand &dealer);

// Dealer peeks for a natural when showing an ace or a ten-value card.
bool dealerPeeks(const Card &upCard);

}
