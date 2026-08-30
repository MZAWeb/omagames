#pragma once

#include <QObject>

// BlackjackGame as QML sees it: the properties, the invokables and the
// commands they enable (games/blackomack/src/blackjackgame.h).
class BridgeTests : public QObject {
    Q_OBJECT
private slots:
    // The bridge saves as it plays, so each test starts on empty settings.
    void cleanup();

    void bridgeBetting();
    void bridgeSeatsBotsOnAFreshTable();
    void bridgeCoachDescribesKnownHandsOnlyOnYourTurn();
    void bridgeCoachSaysNothingWhileItIsOff();
    void bridgeSkipPacingStopsBeforeTheHumanDecision();
    void bridgeLocksTheStakeOnceDealt();
    void bridgeReportsWhatEachHandPaid();
    void bridgeReportsCumulativeNetPerSeat();
    void bridgeBrokePlayerMustStartOver();
    void bridgeCapsTheTableWhileTheWindowIsCompact();
    void bridgeKeepsASavedTableBiggerThanACompactWindow();
    void bridgeBetPresets();
    void bridgeBetPresetsCollapseOnAThinBankroll();
};
