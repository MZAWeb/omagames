#include "choices.h"

#include <QCoreApplication>

namespace {

const auto kClassicId = QStringLiteral("classic");
const auto kWrapId = QStringLiteral("wrap");
const auto kSlowId = QStringLiteral("slow");
const auto kNormalId = QStringLiteral("normal");
const auto kFastId = QStringLiteral("fast");

QString text(const char *context, const char *source) {
    return QCoreApplication::translate(context, source);
}

// Spelled out from the ladder itself, so the start screen can never drift
// from what the engine actually does.
QString speedDescription(Difficulty difficulty) {
    const SpeedParams speed = Rules::params(difficulty);
    const double from = double(Rules::kTicksPerSecond) / speed.startMoveTicks;
    const double to = double(Rules::kTicksPerSecond) / speed.minMoveTicks;
    return text("Difficulties", "%1 to %2 cells a second, one step faster every %3 foods.")
        .arg(from, 0, 'f', 1)
        .arg(to, 0, 'f', 1)
        .arg(speed.foodsPerSpeedUp);
}

}  // namespace

namespace Modes {

QString id(Mode mode) {
    return mode == Mode::Wrap ? kWrapId : kClassicId;
}

QVector<ModeInfo> all() {
    return {
        {Mode::Classic, kClassicId, text("Modes", "Classic"),
         text("Modes", "The walls kill. No grace, no second chance.")},
        {Mode::Wrap, kWrapId, text("Modes", "Wrap"),
         text("Modes", "The edges carry you across; only your own tail can stop you.")},
    };
}

ModeInfo info(Mode mode) {
    for (const ModeInfo &candidate : all()) {
        if (candidate.mode == mode)
            return candidate;
    }
    return {};
}

bool fromId(const QString &id, Mode *mode) {
    for (int i = 0; i < kModeCount; ++i) {
        if (Modes::id(Mode(i)) == id) {
            *mode = Mode(i);
            return true;
        }
    }
    return false;
}

}  // namespace Modes

namespace Difficulties {

QString id(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Slow:
        return kSlowId;
    case Difficulty::Fast:
        return kFastId;
    case Difficulty::Normal:
        break;
    }
    return kNormalId;
}

QVector<DifficultyInfo> all() {
    return {
        {Difficulty::Slow, kSlowId, text("Difficulties", "Slow"), speedDescription(Difficulty::Slow)},
        {Difficulty::Normal, kNormalId, text("Difficulties", "Normal"), speedDescription(Difficulty::Normal)},
        {Difficulty::Fast, kFastId, text("Difficulties", "Fast"), speedDescription(Difficulty::Fast)},
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

QString tableId(Mode mode, Difficulty difficulty) {
    return Modes::id(mode) + QLatin1Char('-') + Difficulties::id(difficulty);
}
