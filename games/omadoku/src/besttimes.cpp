#include "besttimes.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QSettings>

#include <algorithm>

namespace {

const auto kScoresKey = QStringLiteral("scores/v1");

}  // namespace

QString BestTimes::idFor(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Easy:
        return QStringLiteral("easy");
    case Difficulty::Medium:
        return QStringLiteral("medium");
    case Difficulty::Hard:
        return QStringLiteral("hard");
    case Difficulty::ExtraHard:
        break;
    }
    return QStringLiteral("extrahard");
}

bool BestTimes::difficultyFromId(const QString &id, Difficulty *difficulty) {
    for (int i = 0; i < kDifficultyCount; ++i) {
        if (idFor(Difficulty(i)) == id) {
            *difficulty = Difficulty(i);
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
    QSettings().setValue(kScoresKey,
                         QString::fromUtf8(QJsonDocument(toJson()).toJson(QJsonDocument::Compact)));
}

int BestTimes::insert(Difficulty difficulty, const TimeEntry &entry) {
    std::vector<TimeEntry> &table = m_tables[int(difficulty)];
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

const std::vector<TimeEntry> &BestTimes::entries(Difficulty difficulty) const {
    return m_tables[int(difficulty)];
}

int BestTimes::best(Difficulty difficulty) const {
    const std::vector<TimeEntry> &table = m_tables[int(difficulty)];
    return table.empty() ? 0 : table.front().seconds;
}

QJsonObject BestTimes::toJson() const {
    QJsonObject json;
    for (int i = 0; i < kDifficultyCount; ++i) {
        QJsonArray list;
        for (const TimeEntry &entry : m_tables[i]) {
            list.append(QJsonObject {
                {QStringLiteral("seconds"), entry.seconds},
                {QStringLiteral("date"), entry.date.toString(Qt::ISODate)},
            });
        }
        json.insert(idFor(Difficulty(i)), list);
    }
    return json;
}

void BestTimes::fromJson(const QJsonObject &json) {
    for (int i = 0; i < kDifficultyCount; ++i) {
        std::vector<TimeEntry> &table = m_tables[i];
        table.clear();
        const QJsonArray list = json.value(idFor(Difficulty(i))).toArray();
        for (const QJsonValue &value : list) {
            const QJsonObject entry = value.toObject();
            const int seconds = entry.value(QStringLiteral("seconds")).toInt();
            if (seconds <= 0)
                continue;  // a hand-edited or truncated file is not worth trusting
            table.push_back({seconds,
                             QDate::fromString(entry.value(QStringLiteral("date")).toString(),
                                               Qt::ISODate)});
        }
        // A stored table is sorted, but nothing guarantees the file was ours.
        std::stable_sort(table.begin(), table.end(),
                         [](const TimeEntry &a, const TimeEntry &b) { return a.seconds < b.seconds; });
        if (int(table.size()) > kMaxEntries)
            table.resize(kMaxEntries);
    }
}
