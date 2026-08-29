#pragma once

#include <QDate>
#include <QJsonObject>
#include <QString>
#include <vector>

#include "level.h"

struct ScoreEntry {
    int score = 0;
    int level = 0;
    QDate date;
};

// The top ten per difficulty, kept in QSettings as one JSON object under
// `scores/v1`. No names: a score is its number, level, difficulty and day.
class HighScores {
public:
    static constexpr int kMaxEntries = 10;

    static QString idFor(Difficulty difficulty);
    static bool difficultyFromId(const QString &id, Difficulty *difficulty);

    void load();
    void save() const;

    // Adds the entry if it makes the table; returns its 0-based rank, or -1.
    // Ties rank below the older score.
    int insert(Difficulty difficulty, const ScoreEntry &entry);
    const std::vector<ScoreEntry> &entries(Difficulty difficulty) const;
    int best(Difficulty difficulty) const;

    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);

private:
    std::vector<ScoreEntry> m_tables[kDifficultyCount];
};
