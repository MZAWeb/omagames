#pragma once

#include <QObject>

// Omatris as an agent plays it (env/omatrisenv.h): the three action spaces,
// what is observed, the replay it keeps, and what it must never touch.
class EnvTests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    void theSpecDescribesEveryActionSpace();
    void placementOffersEveryLanding();
    void aPlacementLandsThePieceAndRunsToTheNext();
    void dropMasksWhatCannotBeReached();
    void rawInputsMoveThePiece();
    void frameSkipLetsTimePass();
    void placingTakesTimeAtAnInputRate();
    void aRunEndsWhenItTopsOut();
    void theModeIsTheOneConfigured();
    void theSameSeedAndActionsPlayTheSameGame();
    void theReplayIsTheGame();
    void aReplayIsDrawnPieceByPiece();
    void aReseededCloneKeepsWhatIsVisible();
    void theEnvLeavesSettingsAlone();
    void theAbiPlaysOmatris();
};
