#include "scoretable.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QSettings>

#include <algorithm>

namespace OmaGames {

namespace {

const auto kScoresKey = QStringLiteral("scores/v1");
const auto kDateField = QStringLiteral("date");

}  // namespace

ScoreTable::ScoreTable(QStringList fields, int maxEntries, std::vector<Category> categories)
    : m_fields(std::move(fields)), m_maxEntries(maxEntries), m_categories(std::move(categories)),
      m_tables(m_categories.size()) {}

std::vector<ScoreTable::Category> ScoreTable::sameOrder(const QStringList &ids, Order order) {
    std::vector<Category> categories;
    categories.reserve(size_t(ids.size()));
    for (const QString &id : ids)
        categories.push_back({id, order, QString()});
    return categories;
}

int ScoreTable::indexOf(const QString &id) const {
    const auto it = std::find_if(m_categories.begin(), m_categories.end(),
                                 [&id](const Category &c) { return c.id == id; });
    return it == m_categories.end() ? -1 : int(it - m_categories.begin());
}

QString ScoreTable::valueFieldOf(const Category &category) const {
    return category.valueField.isEmpty() ? m_fields.value(0) : category.valueField;
}

bool ScoreTable::better(const Category &category, const ScoreEntry &a, const ScoreEntry &b) const {
    return category.order == LowerIsBetter ? a.value < b.value : a.value > b.value;
}

void ScoreTable::load() {
    const QByteArray raw = QSettings().value(kScoresKey).toString().toUtf8();
    fromJson(QJsonDocument::fromJson(raw).object());
}

void ScoreTable::save() const {
    QSettings().setValue(kScoresKey,
                         QString::fromUtf8(QJsonDocument(toJson()).toJson(QJsonDocument::Compact)));
}

int ScoreTable::insert(const QString &id, const ScoreEntry &entry) {
    const int index = indexOf(id);
    if (index < 0)
        return -1;
    const Category &cat = m_categories[size_t(index)];
    std::vector<ScoreEntry> &table = m_tables[size_t(index)];
    const auto slot = std::find_if(table.begin(), table.end(),
                                   [this, &cat, &entry](const ScoreEntry &e) { return better(cat, entry, e); });
    const int rank = int(slot - table.begin());
    if (rank >= m_maxEntries)
        return -1;
    table.insert(slot, entry);
    if (int(table.size()) > m_maxEntries)
        table.resize(size_t(m_maxEntries));
    return rank;
}

const std::vector<ScoreEntry> &ScoreTable::entries(const QString &id) const {
    static const std::vector<ScoreEntry> empty;
    const int index = indexOf(id);
    return index < 0 ? empty : m_tables[size_t(index)];
}

int ScoreTable::best(const QString &id) const {
    const std::vector<ScoreEntry> &table = entries(id);
    return table.empty() ? 0 : table.front().value;
}

QVariantList ScoreTable::toVariantList(const QString &id) const {
    const int index = indexOf(id);
    if (index < 0)
        return {};
    const QString valueField = valueFieldOf(m_categories[size_t(index)]);
    QVariantList list;
    for (const ScoreEntry &entry : entries(id)) {
        QVariantMap map;
        for (const QString &field : m_fields)
            map.insert(field, field == valueField ? entry.value : entry.extra.value(field).toInt());
        map.insert(kDateField, entry.date.toString(Qt::ISODate));
        list.append(map);
    }
    return list;
}

QJsonObject ScoreTable::toJson() const {
    QJsonObject json;
    for (size_t i = 0; i < m_categories.size(); ++i) {
        const QString valueField = valueFieldOf(m_categories[i]);
        QJsonArray list;
        for (const ScoreEntry &entry : m_tables[i]) {
            QJsonObject o;
            for (const QString &field : m_fields)
                o.insert(field, field == valueField ? entry.value : entry.extra.value(field).toInt());
            o.insert(kDateField, entry.date.toString(Qt::ISODate));
            list.append(o);
        }
        json.insert(m_categories[i].id, list);
    }
    return json;
}

void ScoreTable::fromJson(const QJsonObject &json) {
    for (size_t i = 0; i < m_categories.size(); ++i) {
        const Category &cat = m_categories[i];
        const QString valueField = valueFieldOf(cat);
        std::vector<ScoreEntry> &table = m_tables[i];
        table.clear();
        for (const QJsonValue &value : json.value(cat.id).toArray()) {
            const QJsonObject o = value.toObject();
            ScoreEntry entry;
            entry.value = o.value(valueField).toInt();
            if (entry.value < m_minimumValue)
                continue;  // a hand-edited or truncated file is not worth trusting
            for (const QString &field : m_fields) {
                if (field != valueField)
                    entry.extra.insert(field, o.value(field).toInt());
            }
            entry.date = QDate::fromString(o.value(kDateField).toString(), Qt::ISODate);
            table.push_back(entry);
        }
        // A stored table is sorted, but nothing guarantees the file was ours.
        std::stable_sort(table.begin(), table.end(),
                         [this, &cat](const ScoreEntry &a, const ScoreEntry &b) { return better(cat, a, b); });
        if (int(table.size()) > m_maxEntries)
            table.resize(size_t(m_maxEntries));
    }
}

}  // namespace OmaGames
