// OmasweeperGame: the preset table and the best times as the lists and maps
// QML reads.
#include "omasweepergame.h"

QString OmasweeperGame::presetLabel() const {
    return Presets::label(m_preset);
}

QVariantList OmasweeperGame::presets() {
    QVariantList list;
    for (const PresetSpec &s : Presets::kAll) {
        list.append(QVariantMap {
            {QStringLiteral("id"), Presets::id(s.id)},
            {QStringLiteral("label"), Presets::label(s.id)},
            {QStringLiteral("width"), s.width},
            {QStringLiteral("height"), s.height},
            {QStringLiteral("mines"), s.mines},
        });
    }
    return list;
}

QVariantList OmasweeperGame::bestTimes() const {
    QVariantList list;
    for (const PresetSpec &s : Presets::kAll) {
        for (const QVariant &entry : m_times.toVariantList(Presets::id(s.id))) {
            QVariantMap row = entry.toMap();
            row.insert(QStringLiteral("preset"), Presets::id(s.id));
            row.insert(QStringLiteral("label"), Presets::label(s.id));
            list.append(row);
        }
    }
    return list;
}

QVariantMap OmasweeperGame::bests() const {
    QVariantMap map;
    for (const PresetSpec &s : Presets::kAll)
        map.insert(Presets::id(s.id), m_times.best(Presets::id(s.id)));
    return map;
}
