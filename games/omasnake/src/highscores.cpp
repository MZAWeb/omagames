#include "highscores.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QSettings>
#include <algorithm>

namespace {

const auto kScoresKey = QStringLiteral("scores/v1");
const auto kClassicId = QStringLiteral("classic");
const auto kWrapId = QStringLiteral("wrap");
const auto kSlowId = QStringLiteral("slow");
const auto kNormalId = QStringLiteral("normal");
const auto kFastId = QStringLiteral("fast");

}  // namespace

QString HighScores::modeId(Mode mode) {
    switch (mode) {
    case Mode::Wrap:
        return kWrapId;
    case Mode::Classic:
        break;
    }
    return kClassicId;
}

QString HighScores::difficultyId(Difficulty difficulty) {
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

QString HighScores::idFor(Mode mode, Difficulty difficulty) {
    return modeId(mode) + QLatin1Char('-') + difficultyId(difficulty);
}

bool HighScores::modeFromId(const QString &id, Mode *mode) {
    for (int i = 0; i < kModeCount; ++i) {
        if (modeId(Mode(i)) == id) {
            *mode = Mode(i);
            return true;
        }
    }
    return false;
}

bool HighScores::difficultyFromId(const QString &id, Difficulty *difficulty) {
    for (int i = 0; i < kDifficultyCount; ++i) {
        if (difficultyId(Difficulty(i)) == id) {
            *difficulty = Difficulty(i);
            return true;
        }
    }
    return false;
}

int HighScores::table(Mode mode, Difficulty difficulty) {
    return int(mode) * kDifficultyCount + int(difficulty);
}

void HighScores::load() {
    const QByteArray raw = QSettings().value(kScoresKey).toString().toUtf8();
    fromJson(QJsonDocument::fromJson(raw).object());
}

void HighScores::save() const {
    QSettings().setValue(kScoresKey, QString::fromUtf8(QJsonDocument(toJson()).toJson(QJsonDocument::Compact)));
}

int HighScores::insert(Mode mode, Difficulty difficulty, const ScoreEntry &entry) {
    std::vector<ScoreEntry> &list = m_tables[table(mode, difficulty)];
    const auto slot = std::find_if(list.begin(), list.end(), [&entry](const ScoreEntry &e) { return e.score < entry.score; });
    const int rank = int(slot - list.begin());
    if (rank >= kMaxEntries)
        return -1;
    list.insert(slot, entry);
    if (int(list.size()) > kMaxEntries)
        list.resize(kMaxEntries);
    return rank;
}

const std::vector<ScoreEntry> &HighScores::entries(Mode mode, Difficulty difficulty) const {
    return m_tables[table(mode, difficulty)];
}

int HighScores::best(Mode mode, Difficulty difficulty) const {
    const std::vector<ScoreEntry> &list = m_tables[table(mode, difficulty)];
    return list.empty() ? 0 : list.front().score;
}

QJsonObject HighScores::toJson() const {
    QJsonObject json;
    for (int m = 0; m < kModeCount; ++m) {
        for (int d = 0; d < kDifficultyCount; ++d) {
            QJsonArray list;
            for (const ScoreEntry &e : m_tables[table(Mode(m), Difficulty(d))]) {
                list.append(QJsonObject {
                    {QStringLiteral("score"), e.score},
                    {QStringLiteral("length"), e.length},
                    {QStringLiteral("date"), e.date.toString(Qt::ISODate)},
                });
            }
            json.insert(idFor(Mode(m), Difficulty(d)), list);
        }
    }
    return json;
}

void HighScores::fromJson(const QJsonObject &json) {
    for (int m = 0; m < kModeCount; ++m) {
        for (int d = 0; d < kDifficultyCount; ++d) {
            std::vector<ScoreEntry> &list = m_tables[table(Mode(m), Difficulty(d))];
            list.clear();
            for (const QJsonValue &value : json.value(idFor(Mode(m), Difficulty(d))).toArray()) {
                const QJsonObject o = value.toObject();
                list.push_back({o.value(QStringLiteral("score")).toInt(), o.value(QStringLiteral("length")).toInt(),
                                QDate::fromString(o.value(QStringLiteral("date")).toString(), Qt::ISODate)});
            }
            std::stable_sort(list.begin(), list.end(), [](const ScoreEntry &a, const ScoreEntry &b) { return a.score > b.score; });
            if (int(list.size()) > kMaxEntries)
                list.resize(kMaxEntries);
        }
    }
}
