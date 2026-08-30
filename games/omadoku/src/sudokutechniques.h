#pragma once

#include <QString>

#include "candidategrid.h"

namespace SudokuGrader {

// The ladder of human solving techniques, easiest first. The grader climbs it
// in this order, so the order is also what "harder" means. The rungs are
// named after the sudoku.coach lessons (https://sudoku.coach/en/learn) and
// ordered by their Sudoku Explainer rating
// (https://sudoku.coach/en/learn/sudoku-difficulty), with two deliberate
// exceptions that make each level a prefix of the ladder: naked single (2.3)
// sits ahead of the line hidden single (1.5) because Easy has the former
// and never needs the latter, and X-wing (3.2) sits after the triples
// because the fish belong to Extra hard. Locked candidates (pointing,
// claiming) are deliberately absent, so a puzzle that would need them is
// beyond the ladder rather than quietly graded harder.
enum class Technique {
    LastDigit,         // SE 1.0
    HiddenSingleBox,   // SE 1.2
    NakedSingle,       // SE 2.3
    HiddenSingleLine,  // SE 1.5
    NakedPair,         // SE 3.0
    HiddenPair,        // SE 3.4
    NakedTriple,       // SE 3.6
    HiddenTriple,      // SE 4.0
    XWing,             // SE 3.2
    Swordfish,         // SE 3.8
    XYWing,            // SE 4.2
};
constexpr int kTechniqueCount = 11;
constexpr Technique kHardestTechnique = Technique::XYWing;

// Stable ids for saved games; the names a player sees live in the bridge.
QString techniqueId(Technique technique);
bool techniqueFromId(const QString &id, Technique *technique);

// Each function applies its technique once - one placement, or every
// elimination one pattern justifies - and reports whether the grid changed.
// None of them ever removes a digit that could still be the answer.
bool lastDigit(CandidateGrid &grid);
bool hiddenSingleBox(CandidateGrid &grid);
bool nakedSingle(CandidateGrid &grid);
bool hiddenSingleLine(CandidateGrid &grid);
bool nakedPair(CandidateGrid &grid);
bool hiddenPair(CandidateGrid &grid);
bool nakedTriple(CandidateGrid &grid);
bool hiddenTriple(CandidateGrid &grid);
bool xWing(CandidateGrid &grid);
bool swordfish(CandidateGrid &grid);
bool xyWing(CandidateGrid &grid);
bool apply(Technique technique, CandidateGrid &grid);

}
