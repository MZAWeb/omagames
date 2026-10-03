#pragma once

#include <QObject>

// Watching an agent's game: ReplayPlayer reading replay/v1 a tick at a time,
// the bridge's replay mode, and the sample in games/omasnake/replays.
class ReplayTests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();

    void aReplayPlaysBackTheGameItRecorded();
    void replaysOfAnotherGameOrRulesAreRefused();
    void nextMoveMovesTheSnakeOnce();

    void theSampleReplayPlaysToItsEnd();
    void theKeysDoNotSteerAReplay();
    void speedsPauseAndStepping();
    void popupsOnlyAtSpeedsThatCanReadThem();
    void watchAgainAndLeave();
    void aFileThatIsNotAReplayDoesNotLoad();

private:
    QString m_settingsDir;
};
