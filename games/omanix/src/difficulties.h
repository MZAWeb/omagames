#pragma once

#include <QString>
#include <QVector>

#include "level.h"

// What a difficulty is called outside the engine: the id that names it in the
// settings, in the score table and in QML, and the words the start screen
// shows beside it.
struct DifficultyInfo {
    Difficulty difficulty;
    QString id;
    QString label;
    QString description;
};

namespace Difficulties {

// Every difficulty, in play order.
QVector<DifficultyInfo> all();
DifficultyInfo info(Difficulty difficulty);
QString id(Difficulty difficulty);
// False, leaving `difficulty` untouched, when the id is not one of ours.
bool fromId(const QString &id, Difficulty *difficulty);

}  // namespace Difficulties
