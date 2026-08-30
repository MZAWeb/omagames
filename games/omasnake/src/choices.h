#pragma once

#include <QString>
#include <QVector>

#include "rules.h"

// What the two start-screen choices are called outside the engine: the ids
// that name them in the settings, in the score tables and in QML, and the
// words shown beside them.
struct ModeInfo {
    Mode mode;
    QString id;
    QString label;
    QString description;
};

struct DifficultyInfo {
    Difficulty difficulty;
    QString id;
    QString label;
    QString description;
};

namespace Modes {

// Both walls rules, in start-screen order.
QVector<ModeInfo> all();
ModeInfo info(Mode mode);
QString id(Mode mode);
// False, leaving `mode` untouched, when the id is not one of ours.
bool fromId(const QString &id, Mode *mode);

}  // namespace Modes

namespace Difficulties {

// All three speeds, slowest first.
QVector<DifficultyInfo> all();
DifficultyInfo info(Difficulty difficulty);
QString id(Difficulty difficulty);
bool fromId(const QString &id, Difficulty *difficulty);

}  // namespace Difficulties

// A score table is keyed by both choices at once, "classic-normal".
QString tableId(Mode mode, Difficulty difficulty);
