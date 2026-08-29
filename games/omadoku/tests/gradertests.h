#pragma once

#include <QObject>

// The technique ladder as a grader (games/omadoku/src/sudokugrader.h).
class GraderTests : public QObject {
    Q_OBJECT
private slots:
    void completeGridNeedsNothing();
    void stepTakesTheEasiestRungUnderTheCeiling();
    void gradeReportsTheHardestRungNeeded();
    void ceilingDecidesWhatIsSolvable();
    void neverGuessesOnAPuzzleBeyondTheLadder();
    void ladderAgreesWithBacktracking();
};
