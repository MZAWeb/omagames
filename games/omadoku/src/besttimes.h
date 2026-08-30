#pragma once

#include <QDate>
#include <QJsonObject>
#include <QString>

#include <vector>

#include "sudokugenerator.h"

struct TimeEntry {
    int seconds = 0;
    QDate date;
};

// The five fastest wins per level, kept in QSettings as one JSON object under
// `scores/v1`. Faster is better; no names, just the clock and the day. Levels
// are stored under the same ids QML sees, so a table survives their order
// changing.
class BestTimes {
public:
    static constexpr int kMaxEntries = 5;

    static QString idFor(Difficulty difficulty);
    static bool difficultyFromId(const QString &id, Difficulty *difficulty);

    void load();
    void save() const;

    // Adds the time if it makes the table; returns its 0-based rank, or -1.
    // Ties rank below the time already there, so a best time has to be beaten.
    int insert(Difficulty difficulty, const TimeEntry &entry);
    const std::vector<TimeEntry> &entries(Difficulty difficulty) const;
    // The fastest time, or 0 when the level has never been solved.
    int best(Difficulty difficulty) const;

    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);

private:
    std::vector<TimeEntry> m_tables[kDifficultyCount];
};
