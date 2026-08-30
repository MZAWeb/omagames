#pragma once

#include <QDate>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include <limits>
#include <vector>

namespace OmaGames {

// One result worth keeping: the number it is ranked on, the day it happened,
// and whatever else the game shows beside it (the level reached, how long the
// snake grew, the clock a Sprint stopped at).
struct ScoreEntry {
    int value = 0;
    QDate date;
    QVariantMap extra;
};

// The top-N table every game keeps: results grouped by a category id (a
// difficulty, a mode, a board preset), ranked highest- or lowest-first, and
// persisted as one JSON object under a versioned QSettings key.
//
// The stored shape is the one the games have always written — an object of
// category id to an array of flat entries, each entry integer fields plus an
// ISO `date` — so an existing table is read as it stands, with nothing to
// migrate. `fields` names those integers; a category's `valueField` says
// which of them ranks, which is how Omatris ranks Sprint on the clock and
// Marathon on the score while both keep every field on disk.
class ScoreTable {
public:
    enum Order {
        HigherIsBetter,
        LowerIsBetter,
    };

    struct Category {
        QString id;
        Order order = HigherIsBetter;
        // Empty means the first of the table's fields.
        QString valueField;
    };

    // `fields` is every integer an entry carries, the one categories rank on
    // by default first. Categories keep their order: it is the order the
    // tables are written in and the order QML shows them in.
    ScoreTable(QStringList fields, int maxEntries, std::vector<Category> categories);

    // Every category ranked the same way on the same field, the usual case.
    static std::vector<Category> sameOrder(const QStringList &ids, Order order);

    int maxEntries() const { return m_maxEntries; }
    // Entries whose value falls below this are dropped when read back;
    // Omadoku refuses a zero-second solve from a hand-edited file.
    void setMinimumValue(int minimum) { m_minimumValue = minimum; }

    void load();
    void save() const;

    // Adds the entry if it makes the table; returns its 0-based rank, or -1.
    // Ties rank below the result already there, so a best has to be beaten.
    int insert(const QString &category, const ScoreEntry &entry);
    const std::vector<ScoreEntry> &entries(const QString &category) const;
    // The number at the top of the table, or 0 when the category has never
    // been played.
    int best(const QString &category) const;

    // The same entries as QML reads them: one map per entry, its integers
    // under their own field names and `date` as an ISO string.
    QVariantList toVariantList(const QString &category) const;

    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);

private:
    // Index of the category, or -1: an id no table was built for.
    int indexOf(const QString &id) const;
    QString valueFieldOf(const Category &category) const;
    bool better(const Category &category, const ScoreEntry &a, const ScoreEntry &b) const;

    QStringList m_fields;
    int m_maxEntries;
    int m_minimumValue = std::numeric_limits<int>::min();
    std::vector<Category> m_categories;
    std::vector<std::vector<ScoreEntry>> m_tables;
};

}  // namespace OmaGames
