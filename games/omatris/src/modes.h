#pragma once

#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "rules.h"
#include "scoretable.h"

// What a mode is called and how its results are kept. A mode crosses to QML
// as a lowercase id, and that same id names its score table, so the words the
// player reads and the key its scores are filed under stay in one place.
namespace Modes {

// "marathon" | "sprint" | "zen".
QString id(Mode mode);
// The mode an id names; `mode` is left as it was when it names none.
bool fromId(const QString &wanted, Mode *mode);
QString label(Mode mode);
// {id, label, description, goal} for the start screen, in play order.
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

}  // namespace Modes
