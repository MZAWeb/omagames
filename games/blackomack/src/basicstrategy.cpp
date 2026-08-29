#include "basicstrategy.h"

using BlackjackRules::Action;

namespace {

// Columns are dealer up cards 2..10 then A (index 0..9). Cell codes:
// H hit, S stand, D double (else hit), d double (else stand), P split.
constexpr const char *kHard[] = {
    "HHHHHHHHHH",  // 5
    "HHHHHHHHHH",  // 6
    "HHHHHHHHHH",  // 7
    "HHHHHHHHHH",  // 8
    "HDDDDHHHHH",  // 9
    "DDDDDDDDHH",  // 10
    "DDDDDDDDDH",  // 11
    "HHSSSHHHHH",  // 12
    "SSSSSHHHHH",  // 13
    "SSSSSHHHHH",  // 14
    "SSSSSHHHHH",  // 15
    "SSSSSHHHHH",  // 16
};
constexpr const char *kSoft[] = {
    "HHHDDHHHHH",  // A2
    "HHHDDHHHHH",  // A3
    "HHDDDHHHHH",  // A4
    "HHDDDHHHHH",  // A5
    "HDDDDHHHHH",  // A6
    "SddddSSHHH",  // A7
    "SSSSSSSSSS",  // A8
    "SSSSSSSSSS",  // A9
};
constexpr const char *kPairs[] = {
    "PPPPPPPPPP",  // A,A
    "PPPPPPHHHH",  // 2,2
    "PPPPPPHHHH",  // 3,3
    "HHHPPHHHHH",  // 4,4
    "DDDDDDDDHH",  // 5,5 (played as hard 10)
    "PPPPPHHHHH",  // 6,6
    "PPPPPPHHHH",  // 7,7
    "PPPPPPPPPP",  // 8,8
    "PPPPPSPPSS",  // 9,9
    "SSSSSSSSSS",  // 10,10
};

int column(const Card &dealerUp) {
    return dealerUp.isAce() ? 9 : dealerUp.value() - 2;
}

Action fromCode(char code, bool canDouble) {
    switch (code) {
    case 'S': return Action::Stand;
    case 'D': return canDouble ? Action::Double : Action::Hit;
    case 'd': return canDouble ? Action::Double : Action::Stand;
    case 'P': return Action::Split;
    default: return Action::Hit;
    }
}

}

Action BasicStrategy::decide(const Hand &hand, const Card &dealerUp, bool canDouble, bool canSplit) {
    const int col = column(dealerUp);
    if (canSplit && hand.canSplit()) {
        const Action a = fromCode(kPairs[hand.cards[0].value() - 1][col], canDouble);
        if (a == Action::Split)
            return a;
        // Non-split pairs fall through to the total tables (e.g. 5,5 doubles as 10).
    }
    if (hand.isSoft() && hand.cards.size() >= 2) {
        const int other = hand.total() - 11;   // the non-ace part, 2..9
        if (other < 2)
            return Action::Hit;     // soft 12 (unsplit aces)
        if (other <= 9)
            return fromCode(kSoft[other - 2][col], canDouble);
        return Action::Stand;       // soft 21
    }
    const int total = hand.total();
    if (total <= 5)
        return Action::Hit;
    if (total >= 17)
        return Action::Stand;
    return fromCode(kHard[total - 5][col], canDouble);
}
