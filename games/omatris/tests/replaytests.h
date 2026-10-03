#pragma once

#include <QObject>

// Watching an agent's game: ReplayPlayer reading replay/v1 and pacing it in
// beats, the bridge's replay mode, and the sample in games/omatris/replays.
class ReplayTests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void aReplayPlaysBackTheGameItRecorded();
    void replaysOfAnotherGameOrRulesAreRefused();
    void beatsShowEveryMoveOfAPlacement();
    void nextPieceStopsAtTheLock();

    void theSampleReplayPlaysToItsEnd();
    void theKeysDoNotMoveAReplay();
    void speedsPauseAndStepping();
    void popupsOnlyAtSpeedsThatCanReadThem();
    void watchAgainAndLeave();
    void aReplayIsNeverAHighScore();
    void aReplayLeavesTheChosenModeAlone();
    void aFileThatIsNotAReplayDoesNotLoad();
    void severalReplaysAreSteppedThrough();
    void oneBadReplayAmongSeveralOpensNone();
};
