#pragma once

#include <QObject>
#include <QString>

// The OmatrisGame bridge driven with stepInterval 0: screen flow, auto shift,
// scripted games end to end, high scores and settings.
class BridgeTests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void startsOnTheStartScreenWithModes();
    void newGameExposesEngineState();
    void autoShiftWaitsThenRepeats();
    void rotationHoldAndDropsGoThroughTheBridge();
    void pauseFreezesEverything();
    void scriptedMarathonTopsOutAndRecordsAScore();
    void scriptedSprintRecordsItsTime();
    void pieceShapesFeedTheBoxes();
    void savedHighScoresReachQml();
    void lastModeIsRemembered();
    void ghostToggleIsRemembered();
    void windowGeometryRoundTrips();

private:
    QString m_settingsDir;
};
