#pragma once

#include <QObject>
#include <QString>

// The OmasweeperGame bridge driven with stepInterval 0: screen flow, whole
// games played to a win and to a loss through the invokables, the clock, the
// counters, the no-guess flag and the best-time table.
class BridgeTests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void startsOnTheStartScreenWithPresets();
    void newGameExposesThePreset();
    void cursorMovesAndStopsAtTheEdges();
    void firstRevealOpensASafeZeroRegion();
    void flagsCountDownAndMayGoNegative();
    void revealOnANumberChords();
    void chordThroughTheBridge();
    void theClockRunsFromTheFirstRevealToTheEnd();
    void aSolvedBoardIsWonAndTimed();
    void hittingAMineLoses();
    void generatorGivingUpClearsTheNoGuessFlag();
    void restartKeepsThePresetAndDrawsANewBoard();
    void backToStartClearsTheBoard();
    void savedBestTimesReachQml();
    void lastPresetIsRemembered();
    void windowGeometryRoundTrips();

private:
    QString m_settingsDir;
};
