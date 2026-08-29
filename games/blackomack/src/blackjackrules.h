#pragma once

#include "hand.h"

// Fixed v1 house rules: 6 decks, dealer stands on all 17s, blackjack pays 3:2,
// double on any two cards, one split, no insurance or surrender.
namespace BlackjackRules {

constexpr int kDecks = 6;
constexpr int kMinBet = 10;
constexpr int kBetStep = 10;
constexpr int kStartingBankroll = 1000;
constexpr int kMaxBots = 5;       // seats at the table besides the human
constexpr int kDefaultBots = 3;   // how many sit down at a fresh table

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

// Dealer peeks for a natural when showing an ace or a ten-value card.
bool dealerPeeks(const Card &upCard);

}
