#pragma once

#include <QObject>

// Puzzle generation (games/omadoku/src/sudokugenerator.h).
class GeneratorTests : public QObject {
    Q_OBJECT
private slots:
    void producesUniquePuzzles_data();
    void producesUniquePuzzles();
    void isDeterministicPerSeed();
    void differentSeedsGiveDifferentPuzzles();
    void randomSeedIsRecorded();
    void hardGenerationIsFast();
};
