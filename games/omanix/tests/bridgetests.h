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
    void levelIntroAndThreatAreExposed();
    void scriptedLevelCompletesAndContinues();
    void scriptedGameOverRecordsAHighScore();
    void savedHighScoresReachQml();
    void lastDifficultyIsRemembered();
    void windowGeometryRoundTrips();

private:
    QString m_settingsDir;
};
