#pragma once

#include <QObject>
#include <QString>

// The OmasnakeGame bridge driven with stepInterval 0: screen flow, scripted
// runs end to end, high scores and settings.
class BridgeTests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void startsOnTheStartScreenWithModesAndDifficulties();
    void newGameExposesEngineState();
    void turnsAndPauseGoThroughTheBridge();
    void readyBeatIsExposed();
    void eatingReportsScoreLengthAndSpeed();
    void scriptedGameOverRecordsAHighScore();
    void selfCrashAndWrapModeAreReported();
    void restartKeepsModeAndDifficulty();
    void modeOnlyChangesBetweenGames();
    void highScoresOrderAndCap();
    void highScoresAreSeparatePerModeAndDifficulty();
    void highScoresRoundTripThroughSettings();
    void lastModeAndDifficultyAreRemembered();
    void windowGeometryRoundTrips();

private:
    QString m_settingsDir;
};
