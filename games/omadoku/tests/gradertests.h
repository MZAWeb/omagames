#pragma once

#include <QObject>

// The naked/hidden singles grader (games/omadoku/src/sudokugrader.h).
class GraderTests : public QObject {
    Q_OBJECT
private slots:
    void completeGridNeedsNoTechnique();
    void singlesFinishAnEasyGrid();
    void rejectsPuzzleNeedingMoreThanSingles();
    void easyPuzzlesFallToSinglesButHardOnesDoNot();
};
