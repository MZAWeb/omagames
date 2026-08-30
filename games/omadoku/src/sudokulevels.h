#pragma once

#include <QString>
#include <QVariantList>

#include "scoretable.h"
#include "sudokugenerator.h"

// The four levels as a player meets them: the ids QML and the best-time tables
// use, the label and description shown for each, and the rungs of the
// technique ladder the level introduces.
//
// The ids are stable identifiers, never shown to the player, so relabelling or
// reordering the levels leaves a stored table intact.
namespace SudokuLevels {

QString id(Difficulty difficulty);
// False, and `difficulty` untouched, for an id no level answers to — all a
// stale setting or a stale QML caller can produce.
bool fromId(const QString &id, Difficulty *difficulty);

// The label shown for a level id, empty when no level answers to it.
QString label(const QString &id);
// Every level as {id, label, techniques, description}, easiest first, where
// `techniques` names the rungs the level introduces in the order a player
// would learn them.
QVariantList all();

QString techniqueName(SudokuGrader::Technique technique);

// The five fastest solves per level. Faster is better, and a solve of no time
// at all is a hand-edited file, not a record.
OmaGames::ScoreTable timesTable();

}  // namespace SudokuLevels
