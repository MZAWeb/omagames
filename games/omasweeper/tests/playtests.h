#pragma once

#include <QObject>

// A board played through the invokables a player would use: the cursor, the
// first reveal, flags, chords, the clock, and the win and the loss.
class PlayTests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cursorMovesAndStopsAtTheEdges();
    void firstRevealOpensASafeZeroRegion();
    void flagsCountDownAndMayGoNegative();
    void revealOnANumberChords();
    void chordThroughTheBridge();
    void theClockRunsFromTheFirstRevealToTheEnd();
    void aSolvedBoardIsWonAndTimed();
    void hittingAMineLoses();
};
