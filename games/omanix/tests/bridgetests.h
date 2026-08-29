#pragma once

#include <QObject>
#include <QString>

// The OmanixGame bridge driven with stepInterval 0: screen flow, scripted
// levels end to end, high scores and settings.
class BridgeTests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void startsOnTheStartScreenWithDifficulties();
    void newGameExposesEngineState();
    void directionsAndPauseGoThroughTheBridge();
    void scriptedLevelCompletesAndContinues();
    void scriptedGameOverRecordsAHighScore();
    void highScoresOrderAndCap();
    void highScoresRoundTripThroughSettings();
    void lastDifficultyIsRemembered();
    void windowGeometryRoundTrips();

private:
    QString m_settingsDir;
};
