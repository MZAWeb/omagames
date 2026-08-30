#pragma once

#include <QObject>

// The no-guess promise: every generated board is won by the solver, from
// the same seed the same board, and Expert stays quick.
class GeneratorTests : public QObject {
    Q_OBJECT

private slots:
    void everyPresetProducesNoGuessBoards();
    void sameSeedSameBoard();
    void attemptSeedsStrideApart();
    void exhaustedAttemptsReturnAnUnguaranteedBoard();
    void generationIsFast();
};
