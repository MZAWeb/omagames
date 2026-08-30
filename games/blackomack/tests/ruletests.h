#pragma once

#include <QObject>

// The house rules, the basic-strategy chart the coach and the mates play by,
// and the personalities that bend it (blackjackrules.h, basicstrategy.h,
// botplayer.h).
class RuleTests : public QObject {
    Q_OBJECT
private slots:
    void payouts();
    void dealerStandsOnSoft17();
    void betValidation();
    void betPresetsFollowTheBankroll();
    void basicStrategySpotChecks();
    void dealerPeekCards();
    void perfectBotFollowsBasicStrategy();
    void cluelessBotDeviatesDeterministically();
    void mixedSkillBotIsSeedStable();
    void launchSaltVariesDecisionsButNotPersonality();
    void botBetsStayInBand();
    void personalityRollIsUniqueAndLabelled();
};
