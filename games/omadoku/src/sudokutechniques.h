#pragma once

#include <QString>

#include "candidategrid.h"

namespace SudokuGrader {

// The ladder of human solving techniques, easiest first. The grader climbs it
// in this order, so the order is also what "harder" means.
enum class Technique {
    NakedSingle,
    HiddenSingle,
    NakedPair,
    HiddenPair,
    PointingPair,
    Claiming,
    NakedTriple,
    XWing,
    YWing,
    Swordfish,
};
constexpr int kTechniqueCount = 10;
constexpr Technique kHardestTechnique = Technique::Swordfish;

// Stable ids for saved games; the names a player sees live in the bridge.
QString techniqueId(Technique technique);
bool techniqueFromId(const QString &id, Technique *technique);

// Each function applies its technique once - one placement, or every
// elimination one pattern justifies - and reports whether the grid changed.
// None of them ever removes a digit that could still be the answer.
bool nakedSingle(CandidateGrid &grid);
bool hiddenSingle(CandidateGrid &grid);
bool nakedPair(CandidateGrid &grid);
bool hiddenPair(CandidateGrid &grid);
bool pointingPair(CandidateGrid &grid);
bool claiming(CandidateGrid &grid);
bool nakedTriple(CandidateGrid &grid);
bool xWing(CandidateGrid &grid);
bool yWing(CandidateGrid &grid);
bool swordfish(CandidateGrid &grid);
bool apply(Technique technique, CandidateGrid &grid);

}
