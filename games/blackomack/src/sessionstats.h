#pragma once

#include "blackjackrules.h"

// What the session has come to: hands played, what they netted and the best
// bankroll ever held. The net is settled a round at a time against the stake,
// which grows with every extra chip that goes down — a double, a split, the
// insurance side bet — so what comes back is measured against what it cost.
class SessionStats {
public:
    int handsPlayed() const { return m_handsPlayed; }
    int netResult() const { return m_netResult; }
    // The best bankroll ever held: kept across a new game, so only clearing
    // the settings wipes it. `newBest` is true for the round that set it, so
    // the dock can celebrate.
    int bestBankroll() const { return m_bestBankroll; }
    bool newBest() const { return m_newBest; }

    void restore(int handsPlayed, int netResult, int bestBankroll);
    // A new game forgets the session but keeps the high score.
    void reset();

    // What this round has cost so far: set when the bets go down, added to by
    // a double, a split or insurance.
    void stake(int amount) { m_roundStake = amount; }
    void addStake(int amount) { m_roundStake += amount; }
    // One settled round: everything the seat got back, and the bankroll it left.
    void settle(int returned, int bankroll);
    void clearCelebration() { m_newBest = false; }   // it lasts exactly one round

private:
    int m_handsPlayed = 0;
    int m_netResult = 0;
    int m_bestBankroll = BlackjackRules::kStartingBankroll;
    bool m_newBest = false;
    int m_roundStake = 0;
};
