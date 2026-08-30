#include "besttimes.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QSettings>
#include <algorithm>

namespace {

const auto kScoresKey = QStringLiteral("scores/v1");

}  // namespace

QString BestTimes::idFor(Preset preset) {
    return QString::fromLatin1(Presets::spec(preset).key);
}

bool BestTimes::presetFromId(const QString &id, Preset *preset) {
    for (const PresetSpec &spec : Presets::kAll) {
        if (QLatin1String(spec.key) == id) {
            *preset = spec.id;
            return true;
        }
    }
    return false;
}

void BestTimes::load() {
    const QByteArray raw = QSettings().value(kScoresKey).toString().toUtf8();
    fromJson(QJsonDocument::fromJson(raw).object());
}

void BestTimes::save() const {
    QSettings().setValue(kScoresKey, QString::fromUtf8(QJsonDocument(toJson()).toJson(QJsonDocument::Compact)));
}

int BestTimes::insert(Preset preset, const TimeEntry &entry) {
    std::vector<TimeEntry> &table = m_tables[int(preset)];
    const auto slot = std::find_if(table.begin(), table.end(),
                                   [&entry](const TimeEntry &e) { return e.seconds > entry.seconds; });
    const int rank = int(slot - table.begin());
    if (rank >= kMaxEntries)
        return -1;
    table.insert(slot, entry);
    if (int(table.size()) > kMaxEntries)
        table.resize(kMaxEntries);
    return rank;
}

const std::vector<TimeEntry> &BestTimes::entries(Preset preset) const {
    return m_tables[int(preset)];
}

int BestTimes::best(Preset preset) const {
    const std::vector<TimeEntry> &table = m_tables[int(preset)];
    return table.empty() ? 0 : table.front().seconds;
}

QJsonObject BestTimes::toJson() const {
    QJsonObject json;
    for (int i = 0; i < kPresetCount; ++i) {
        QJsonArray list;
        for (const TimeEntry &e : m_tables[i]) {
            list.append(QJsonObject {
                {QStringLiteral("seconds"), e.seconds},
                {QStringLiteral("date"), e.date.toString(Qt::ISODate)},
            });
        }
        json.insert(idFor(Preset(i)), list);
    }
    return json;
}

void BestTimes::fromJson(const QJsonObject &json) {
    for (int i = 0; i < kPresetCount; ++i) {
        std::vector<TimeEntry> &table = m_tables[i];
        table.clear();
        const QJsonArray list = json.value(idFor(Preset(i))).toArray();
        for (const QJsonValue &value : list) {
            const QJsonObject o = value.toObject();
            table.push_back({o.value(QStringLiteral("seconds")).toInt(),
                             QDate::fromString(o.value(QStringLiteral("date")).toString(), Qt::ISODate)});
        }
        std::stable_sort(table.begin(), table.end(),
                         [](const TimeEntry &a, const TimeEntry &b) { return a.seconds < b.seconds; });
        if (int(table.size()) > kMaxEntries)
            table.resize(kMaxEntries);
    }
}
