#pragma once

#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "rules.h"
#include "scoretable.h"

class Game;

// What a mode is called and how its results are kept. A mode crosses to QML
// as a lowercase id, and that same id names its score table, so the words the
// player reads and the key its scores are filed under stay in one place.
namespace Modes {

// "marathon" | "sprint" | "zen" | "challenge".
QString id(Mode mode);
// The mode an id names; `mode` is left as it was when it names none.
bool fromId(const QString &wanted, Mode *mode);
QString label(Mode mode);
// Whether the mode keeps a table. A Challenge does not: how it goes depends
// on the stack it dealt, so two results are never the same contest.
bool ranked(Mode mode);
// {id, label, description, goal, ranked} for the start screen, in play order.
QVariantList list();
// The top ten per mode. A run keeps its score, lines, level and clock
// whatever the mode; which of them ranks is the mode's business, and Sprint
// is the one raced against the clock.
OmaGames::ScoreTable scoreTable();
// Every kept run as {mode, label, score, lines, level, millis, date}, best
// first, mode by mode.
QVariantList scoreRows(const OmaGames::ScoreTable &scores);
// Mode id -> the number that mode is judged on.
QVariantMap bests(const OmaGames::ScoreTable &scores);
// Files a run that has just ended in its mode's table and returns its rank,
// or -1 when it did not place or does not count: a Challenge keeps no table,
// and a Sprint that topped out never crossed the line, so it has no time.
int record(OmaGames::ScoreTable &scores, const Game &game);

}  // namespace Modes
