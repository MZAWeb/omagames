#pragma once

#include <QDate>
#include <QJsonObject>
#include <QString>
#include <vector>

#include "rules.h"

struct ScoreEntry {
    int score = 0;
    int length = 0;
    QDate date;
};

// The top ten of every mode and difficulty, kept in QSettings as one JSON
// object under `scores/v1`. No names: a score is its number, the length the
// snake reached and the day.
class HighScores {
public:
    static constexpr int kMaxEntries = 10;

    static QString modeId(Mode mode);
    static QString difficultyId(Difficulty difficulty);
    // "classic-normal": the key one table is kept under.
    static QString idFor(Mode mode, Difficulty difficulty);
    static bool modeFromId(const QString &id, Mode *mode);
    static bool difficultyFromId(const QString &id, Difficulty *difficulty);

    void load();
    void save() const;

    // Adds the entry if it makes the table; returns its 0-based rank, or -1.
    // Ties rank below the older score.
    int insert(Mode mode, Difficulty difficulty, const ScoreEntry &entry);
    const std::vector<ScoreEntry> &entries(Mode mode, Difficulty difficulty) const;
    int best(Mode mode, Difficulty difficulty) const;

    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);

private:
    static int table(Mode mode, Difficulty difficulty);

    std::vector<ScoreEntry> m_tables[kModeCount * kDifficultyCount];
};
