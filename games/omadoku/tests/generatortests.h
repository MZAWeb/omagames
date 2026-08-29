#pragma once

#include <QObject>

// Puzzle generation and the technique bands that define the levels
// (games/omadoku/src/sudokugenerator.h).
class GeneratorTests : public QObject {
    Q_OBJECT
private slots:
    void ceilingsClimbTheLadder();
    void everyLevelNeedsExactlyItsTechniques_data();
    void everyLevelNeedsExactlyItsTechniques();
    void isDeterministicPerSeed();
    void differentSeedsGiveDifferentPuzzles();
    void randomSeedIsRecorded();
    void everyLevelGeneratesQuickly();
};
