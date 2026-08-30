#include "highscores.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QSettings>
#include <algorithm>

namespace {

const auto kScoresKey = QStringLiteral("scores/v1");
const auto kMarathonId = QStringLiteral("marathon");
const auto kSprintId = QStringLiteral("sprint");
const auto kZenId = QStringLiteral("zen");

}  // namespace

QString HighScores::idFor(Mode mode) {
    switch (mode) {
    case Mode::Sprint:
        return kSprintId;
    case Mode::Zen:
        return kZenId;
    case Mode::Marathon:
        break;
    }
    return kMarathonId;
}

bool HighScores::modeFromId(const QString &id, Mode *mode) {
    for (int i = 0; i < kModeCount; ++i) {
        if (idFor(Mode(i)) == id) {
            *mode = Mode(i);
            return true;
        }
    }
    return false;
}

bool HighScores::better(Mode mode, const ScoreEntry &a, const ScoreEntry &b) {
    if (Rules::params(mode).rankByTime)
        return a.millis < b.millis;
    return a.score > b.score;
}

void HighScores::load() {
    const QByteArray raw = QSettings().value(kScoresKey).toString().toUtf8();
    fromJson(QJsonDocument::fromJson(raw).object());
}

void HighScores::save() const {
    QSettings().setValue(kScoresKey, QString::fromUtf8(QJsonDocument(toJson()).toJson(QJsonDocument::Compact)));
}

int HighScores::insert(Mode mode, const ScoreEntry &entry) {
    std::vector<ScoreEntry> &table = m_tables[int(mode)];
    const auto slot = std::find_if(table.begin(), table.end(),
                                   [mode, &entry](const ScoreEntry &e) { return better(mode, entry, e); });
    const int rank = int(slot - table.begin());
    if (rank >= kMaxEntries)
        return -1;
    table.insert(slot, entry);
    if (int(table.size()) > kMaxEntries)
        table.resize(kMaxEntries);
    return rank;
}

const std::vector<ScoreEntry> &HighScores::entries(Mode mode) const {
    return m_tables[int(mode)];
}

int HighScores::best(Mode mode) const {
    const std::vector<ScoreEntry> &table = m_tables[int(mode)];
    if (table.empty())
        return 0;
    return Rules::params(mode).rankByTime ? table.front().millis : table.front().score;
}

QJsonObject HighScores::toJson() const {
    QJsonObject json;
    for (int i = 0; i < kModeCount; ++i) {
        QJsonArray list;
        for (const ScoreEntry &e : m_tables[i]) {
            list.append(QJsonObject {
                {QStringLiteral("score"), e.score},
                {QStringLiteral("lines"), e.lines},
                {QStringLiteral("level"), e.level},
                {QStringLiteral("millis"), e.millis},
                {QStringLiteral("date"), e.date.toString(Qt::ISODate)},
            });
        }
        json.insert(idFor(Mode(i)), list);
    }
    return json;
}

void HighScores::fromJson(const QJsonObject &json) {
    for (int i = 0; i < kModeCount; ++i) {
        const Mode mode = Mode(i);
        std::vector<ScoreEntry> &table = m_tables[i];
        table.clear();
        const QJsonArray list = json.value(idFor(mode)).toArray();
        for (const QJsonValue &value : list) {
            const QJsonObject o = value.toObject();
            table.push_back({o.value(QStringLiteral("score")).toInt(),
                             o.value(QStringLiteral("lines")).toInt(),
                             o.value(QStringLiteral("level")).toInt(),
                             o.value(QStringLiteral("millis")).toInt(),
                             QDate::fromString(o.value(QStringLiteral("date")).toString(), Qt::ISODate)});
        }
        std::stable_sort(table.begin(), table.end(),
                         [mode](const ScoreEntry &a, const ScoreEntry &b) { return better(mode, a, b); });
        if (int(table.size()) > kMaxEntries)
            table.resize(kMaxEntries);
    }
}
