#pragma once

#include <QObject>

// The OmasweeperGame bridge driven with stepInterval 0: the screens, the
// presets, and everything remembered between runs.
class BridgeTests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void startsOnTheStartScreenWithPresets();
    void newGameExposesThePreset();
    void generatorGivingUpClearsTheNoGuessFlag();
    void restartKeepsThePresetAndDrawsANewBoard();
    void backToStartClearsTheBoard();
    void savedBestTimesReachQml();
    void lastPresetIsRemembered();
    void windowGeometryRoundTrips();
};
