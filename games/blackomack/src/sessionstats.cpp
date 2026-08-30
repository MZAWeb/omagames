#include "sessionstats.h"

void SessionStats::restore(int handsPlayed, int netResult, int bestBankroll) {
    m_handsPlayed = handsPlayed;
    m_netResult = netResult;
    m_bestBankroll = bestBankroll;
}

void SessionStats::reset() {
    m_handsPlayed = 0;
    m_netResult = 0;
    m_newBest = false;
    m_roundStake = 0;
}

void SessionStats::settle(int returned, int bankroll) {
    ++m_handsPlayed;
    m_netResult += returned - m_roundStake;
    if (bankroll > m_bestBankroll) {
        m_bestBankroll = bankroll;
        m_newBest = true;
    }
}
