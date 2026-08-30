#pragma once

#include <QObject>

// What Black Omack keeps between launches: the saved table, the session
// stats, the best bankroll and the coach switch.
class PersistenceTests : public QObject {
    Q_OBJECT
private slots:
    // The bridge saves as it plays, so each test starts on empty settings.
    void cleanup();

    void gameStateRoundTrips();
    void bridgeTrimsOversizedSavedTableOnLoad();
    void bridgeLoadsSavedTableInPlayOrder();
    void bridgeCoachDefaultsOffAndPersists();
    void bridgePlaysAndPersists();
    void bridgeBestBankrollIsAHighScore();
};
