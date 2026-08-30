#include "highscores.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QSettings>
#include <algorithm>

namespace {

const auto kScoresKey = QStringLiteral("scores/v1");
const auto kEasyId = QStringLiteral("easy");
const auto kNormalId = QStringLiteral("normal");
const auto kHardId = QStringLiteral("hard");

}  // namespace

QString HighScores::idFor(Difficulty difficulty) {
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

bool HighScores::difficultyFromId(const QString &id, Difficulty *difficulty) {
    for (int i = 0; i < kDifficultyCount; ++i) {
        if (idFor(Difficulty(i)) == id) {
            *difficulty = Difficulty(i);
            return true;
        }
    }
    return false;
}

void HighScores::load() {
    const QByteArray raw = QSettings().value(kScoresKey).toString().toUtf8();
    fromJson(QJsonDocument::fromJson(raw).object());
}

void HighScores::save() const {
    QSettings().setValue(kScoresKey, QString::fromUtf8(QJsonDocument(toJson()).toJson(QJsonDocument::Compact)));
}

int HighScores::insert(Difficulty difficulty, const ScoreEntry &entry) {
    std::vector<ScoreEntry> &table = m_tables[int(difficulty)];
    const auto slot = std::find_if(table.begin(), table.end(), [&entry](const ScoreEntry &e) { return e.score < entry.score; });
    const int rank = int(slot - table.begin());
    if (rank >= kMaxEntries)
        return -1;
    table.insert(slot, entry);
    if (int(table.size()) > kMaxEntries)
        table.resize(kMaxEntries);
    return rank;
}

const std::vector<ScoreEntry> &HighScores::entries(Difficulty difficulty) const {
    return m_tables[int(difficulty)];
}

int HighScores::best(Difficulty difficulty) const {
    const std::vector<ScoreEntry> &table = m_tables[int(difficulty)];
    return table.empty() ? 0 : table.front().score;
}

QJsonObject HighScores::toJson() const {
    QJsonObject json;
    for (int i = 0; i < kDifficultyCount; ++i) {
        QJsonArray list;
        for (const ScoreEntry &e : m_tables[i]) {
            list.append(QJsonObject {
                {QStringLiteral("score"), e.score},
                {QStringLiteral("level"), e.level},
                {QStringLiteral("date"), e.date.toString(Qt::ISODate)},
            });
        }
        json.insert(idFor(Difficulty(i)), list);
    }
    return json;
}

void HighScores::fromJson(const QJsonObject &json) {
    for (int i = 0; i < kDifficultyCount; ++i) {
        std::vector<ScoreEntry> &table = m_tables[i];
        table.clear();
        const QJsonArray list = json.value(idFor(Difficulty(i))).toArray();
        for (const QJsonValue &value : list) {
            const QJsonObject o = value.toObject();
            table.push_back({o.value(QStringLiteral("score")).toInt(), o.value(QStringLiteral("level")).toInt(),
                             QDate::fromString(o.value(QStringLiteral("date")).toString(), Qt::ISODate)});
        }
        std::stable_sort(table.begin(), table.end(), [](const ScoreEntry &a, const ScoreEntry &b) { return a.score > b.score; });
        if (int(table.size()) > kMaxEntries)
            table.resize(kMaxEntries);
    }
}
