#pragma once

#include <QDate>
#include <QJsonObject>
#include <QString>
#include <vector>

#include "rules.h"

struct ScoreEntry {
    int score = 0;
    int lines = 0;
    int level = 0;
    int millis = 0;
    QDate date;
};

// The top ten per mode, kept in QSettings as one JSON object under
// `scores/v1`. No names: a run is its number, its lines, its clock and the
// day. Marathon and Zen rank on the score, Sprint on the clock.
class HighScores {
public:
    static constexpr int kMaxEntries = 10;

    static QString idFor(Mode mode);
    static bool modeFromId(const QString &id, Mode *mode);
    // True when `a` belongs above `b` in that mode's table.
    static bool better(Mode mode, const ScoreEntry &a, const ScoreEntry &b);

    void load();
    void save() const;

    // Adds the entry if it makes the table; returns its 0-based rank, or -1.
    // Ties rank below the older run.
    int insert(Mode mode, const ScoreEntry &entry);
    const std::vector<ScoreEntry> &entries(Mode mode) const;
    // The number the mode is judged on at the top of its table, or 0 when it
    // has never been played: the best score, or the fastest run in ms.
    int best(Mode mode) const;

    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);

private:
    std::vector<ScoreEntry> m_tables[kModeCount];
};
