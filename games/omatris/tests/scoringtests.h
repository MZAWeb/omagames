#pragma once

#include <QObject>

// What a clear is worth: the guideline table, back-to-back and combo, the two
// kinds of T-spin, the level ramp and where a mode ends.
class ScoringTests : public QObject {
    Q_OBJECT

private slots:
    void scoringPaysTheGuidelineTable();
    void backToBackAndComboStack();
    void tSpinTripleScoresAsAFullSpin();
    void tSpinMiniIsToldFromAFullOne();
    void levelRisesEveryTenLines();
    void sprintFinishesAtFortyLinesAndSprintZenNeverRamp();
    void popupsNameTheClearAndTheStreaks();
};
