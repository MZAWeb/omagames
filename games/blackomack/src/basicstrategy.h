#pragma once

#include "blackjackrules.h"
#include "hand.h"

// Standard basic strategy for 6 decks, dealer stands on soft 17, double after
// split allowed, no surrender. When the preferred action is not available
// (doubling after a hit, re-splitting) the usual fallback applies.
namespace BasicStrategy {

BlackjackRules::Action decide(const Hand &hand, const Card &dealerUp, bool canDouble, bool canSplit);

}
