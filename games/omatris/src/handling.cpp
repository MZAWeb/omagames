#include "handling.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QSettings>
#include <QVariantMap>
#include <algorithm>
#include <array>

#include "autoshift.h"
#include "rules.h"

namespace {

const auto kSettingsKey = QStringLiteral("handling/v1");

const auto kDasId = QStringLiteral("das");
const auto kArrId = QStringLiteral("arr");
const auto kSoftDropId = QStringLiteral("softDrop");

// Soft drop moves in the steps players compare, slowest first, so "instant"
// is one press past the fastest multiple rather than a number to scroll to.
constexpr std::array<int, 5> kSoftDropSteps = {5, 10, Rules::kSoftDropFactor, 40, Handling::kInstantSoftDrop};

QString tr(const char *text) {
    return QCoreApplication::translate("Handling", text);
}

int softDropIndex(int factor) {
    const auto it = std::find(kSoftDropSteps.begin(), kSoftDropSteps.end(), factor);
    return it == kSoftDropSteps.end() ? -1 : int(it - kSoftDropSteps.begin());
}

bool inRange(int value, int low, int high) {
    return value >= low && value <= high;
}

QString msText(int ticks) {
    return tr("%1 ms").arg(Handling::ticksToMs(ticks));
}

}  // namespace

Handling Handling::defaults() {
    return {AutoShift::kDelayTicks, AutoShift::kRepeatTicks, Rules::kSoftDropFactor};
}

bool Handling::isDefault() const {
    return *this == defaults();
}

bool Handling::operator==(const Handling &other) const {
    return dasTicks == other.dasTicks && arrTicks == other.arrTicks && softDropFactor == other.softDropFactor;
}

bool Handling::canStep(Setting setting, int delta) const {
    switch (setting) {
    case Das:
        return inRange(dasTicks + delta, kMinDasTicks, kMaxDasTicks);
    case Arr:
        return inRange(arrTicks + delta, kMinArrTicks, kMaxArrTicks);
    case SoftDrop:
        return inRange(softDropIndex(softDropFactor) + delta, 0, int(kSoftDropSteps.size()) - 1);
    }
    return false;
}

bool Handling::step(Setting setting, int delta) {
    if (delta == 0 || !canStep(setting, delta))
        return false;
    switch (setting) {
    case Das:
        dasTicks += delta;
        break;
    case Arr:
        arrTicks += delta;
        break;
    case SoftDrop:
        softDropFactor = kSoftDropSteps[size_t(softDropIndex(softDropFactor) + delta)];
        break;
    }
    return true;
}

QJsonObject Handling::toJson() const {
    return {{kDasId, dasTicks}, {kArrId, arrTicks}, {kSoftDropId, softDropFactor}};
}

Handling Handling::fromJson(const QJsonObject &json) {
    Handling handling = defaults();
    const int das = json.value(kDasId).toInt(-1);
    if (inRange(das, kMinDasTicks, kMaxDasTicks))
        handling.dasTicks = das;
    const int arr = json.value(kArrId).toInt(-1);
    if (inRange(arr, kMinArrTicks, kMaxArrTicks))
        handling.arrTicks = arr;
    const int softDrop = json.value(kSoftDropId).toInt(-1);
    if (softDropIndex(softDrop) >= 0)
        handling.softDropFactor = softDrop;
    return handling;
}

Handling Handling::load() {
    const QString stored = QSettings().value(kSettingsKey).toString();
    return fromJson(QJsonDocument::fromJson(stored.toUtf8()).object());
}

void Handling::save() const {
    QSettings settings;
    if (isDefault())
        settings.remove(kSettingsKey);
    else
        settings.setValue(kSettingsKey, QString::fromUtf8(QJsonDocument(toJson()).toJson(QJsonDocument::Compact)));
}

QVariantList Handling::rows() const {
    const Handling fallback = defaults();
    struct Row {
        Setting setting;
        QString label;
        QString description;
        QString value;
        bool isDefault;
    };
    const Row rows[] = {
        {Das, tr("Auto-shift delay"), tr("How long ← or → is held before the piece starts sliding."),
         msText(dasTicks), dasTicks == fallback.dasTicks},
        {Arr, tr("Auto-repeat rate"), tr("The gap between cells once it slides. Instant goes straight to the wall."),
         arrTicks == 0 ? tr("Instant") : msText(arrTicks), arrTicks == fallback.arrTicks},
        {SoftDrop, tr("Soft drop speed"), tr("How much faster ↓ falls than gravity. Instant reaches the floor without locking."),
         softDropFactor == kInstantSoftDrop ? tr("Instant") : tr("%1×").arg(softDropFactor),
         softDropFactor == fallback.softDropFactor},
    };
    QVariantList list;
    for (const Row &row : rows) {
        list.append(QVariantMap {{QStringLiteral("id"), id(row.setting)},
                                 {QStringLiteral("label"), row.label},
                                 {QStringLiteral("description"), row.description},
                                 {QStringLiteral("value"), row.value},
                                 {QStringLiteral("canLower"), canStep(row.setting, -1)},
                                 {QStringLiteral("canRaise"), canStep(row.setting, 1)},
                                 {QStringLiteral("isDefault"), row.isDefault}});
    }
    return list;
}

QString Handling::id(Setting setting) {
    switch (setting) {
    case Arr:
        return kArrId;
    case SoftDrop:
        return kSoftDropId;
    case Das:
        break;
    }
    return kDasId;
}

bool Handling::fromId(const QString &wanted, Setting *setting) {
    for (int i = 0; i < kSettingCount; ++i) {
        if (id(Setting(i)) == wanted) {
            *setting = Setting(i);
            return true;
        }
    }
    return false;
}

int Handling::ticksToMs(int ticks) {
    return (ticks * 1000 + Rules::kTicksPerSecond / 2) / Rules::kTicksPerSecond;
}
