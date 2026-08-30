#include "difficulties.h"

#include <QCoreApplication>

namespace {

const auto kEasyId = QStringLiteral("easy");
const auto kNormalId = QStringLiteral("normal");
const auto kHardId = QStringLiteral("hard");

}  // namespace

namespace Difficulties {

QString id(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Easy:
        return kEasyId;
    case Difficulty::Hard:
        return kHardId;
    case Difficulty::Normal:
        break;
    }
    return kNormalId;
}

QVector<DifficultyInfo> all() {
    const auto text = [](const char *source) { return QCoreApplication::translate("Difficulties", source); };
    return {
        {Difficulty::Easy, kEasyId, text("Easy"), text("Two slow balls, one chaser, a gentle ramp.")},
        {Difficulty::Normal, kNormalId, text("Normal"), text("Three balls, two chasers, a short step up.")},
        {Difficulty::Hard, kHardId, text("Hard"), text("Four fast balls, three chasers, no mercy.")},
    };
}

DifficultyInfo info(Difficulty difficulty) {
    for (const DifficultyInfo &candidate : all()) {
        if (candidate.difficulty == difficulty)
            return candidate;
    }
    return {};
}

bool fromId(const QString &id, Difficulty *difficulty) {
    for (int i = 0; i < kDifficultyCount; ++i) {
        if (Difficulties::id(Difficulty(i)) == id) {
            *difficulty = Difficulty(i);
            return true;
        }
    }
    return false;
}

}  // namespace Difficulties
