#pragma once

#include <QString>
#include <vector>

#include "omasweepergame.h"

// What the bridge suites share: a seeded game, the moves a player would make
// on it through the invokables, and the throwaway settings both write to.
namespace Scenario {

constexpr quint32 kSeed = 20260830u;

inline const QString kStart = QStringLiteral("start");
inline const QString kPlaying = QStringLiteral("playing");
inline const QString kWon = QStringLiteral("won");
inline const QString kLost = QStringLiteral("lost");

// Points every suite's QSettings at one throwaway directory; false when it
// could not be made.
bool redirectSettings();
void clearSettings();

OmasweeperGame *started(OmasweeperGame &game, Preset preset, quint32 seed = kSeed);
// Opens the middle of the board, which is what generates it.
void openFirst(OmasweeperGame &game);
// Plays the board with the logical solver, but only ever through the
// invokables a player would use, until it is won or logic runs out.
void solveThroughBridge(OmasweeperGame &game);
int firstMine(const Board &board);
// Hidden cells whose flags can be toggled without touching the opening.
std::vector<int> hiddenCells(const Board &board, int wanted);

}  // namespace Scenario
