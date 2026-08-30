#pragma once

#include <QObject>

// The two bets that change the shape of a round: splitting a pair again and
// again, and the side bet against a dealer ace.
class InsuranceTests : public QObject {
    Q_OBJECT
private slots:
    // The bridge saves as it plays, so each test starts on empty settings.
    void cleanup();

    void resplitsToFourHandsAndStopsThere();
    void splitAcesAreNeverResplit();
    void basicStrategyResplitsPairs();
    void botsResplitAtTheTable();
    void insuranceStakeAndReturn();
    void insuranceIsOfferedOnlyAgainstAnAce();
    void insurancePaysTwoToOneOnADealerNatural();
    void insuranceIsLostWithoutADealerNatural();
    void insuranceIsSkippedWhenNobodyCanCoverIt();
    void botInsuranceFollowsSkillDeterministically();
    void botAnswersTheInsuranceOfferAtTheTable();
    void bridgeOffersInsuranceAgainstAnAce();
    void seatsModelCarriesTheInsuranceSideBet();
    void bridgeDeclinedInsuranceCostsNothing();
};
