#pragma once

#include <QDate>
#include <QJsonObject>
#include <QString>
#include <vector>

#include "presets.h"

struct TimeEntry {
    int seconds = 0;
    QDate date;
};

// The five fastest wins per preset, kept in QSettings as one JSON object
// under `scores/v1`. Faster is better; no names, just the clock and the day.
class BestTimes {
public:
    static constexpr int kMaxEntries = 5;

    static QString idFor(Preset preset);
    static bool presetFromId(const QString &id, Preset *preset);

    void load();
    void save() const;

    // Adds the time if it makes the table; returns its 0-based rank, or -1.
    // Ties rank below the older time.
    int insert(Preset preset, const TimeEntry &entry);
    const std::vector<TimeEntry> &entries(Preset preset) const;
    // The fastest time, or 0 when the preset has never been won.
    int best(Preset preset) const;

    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);

private:
    std::vector<TimeEntry> m_tables[kPresetCount];
};
