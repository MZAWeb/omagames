// OmasnakeGame: the start-screen choices and the score tables as the lists
// and maps QML reads.
#include "omasnakegame.h"

#include "choices.h"

QString OmasnakeGame::modeLabel() const {
    return Modes::info(m_mode).label;
}

QString OmasnakeGame::difficultyLabel() const {
    return Difficulties::info(m_difficulty).label;
}

QVariantList OmasnakeGame::modes() {
    QVariantList list;
    for (const ModeInfo &info : Modes::all()) {
        list.append(QVariantMap {
            {QStringLiteral("id"), info.id},
            {QStringLiteral("label"), info.label},
            {QStringLiteral("description"), info.description},
        });
    }
    return list;
}

QVariantList OmasnakeGame::difficulties() {
    QVariantList list;
    for (const DifficultyInfo &info : Difficulties::all()) {
        list.append(QVariantMap {
            {QStringLiteral("id"), info.id},
            {QStringLiteral("label"), info.label},
            {QStringLiteral("description"), info.description},
        });
    }
    return list;
}

QVariantList OmasnakeGame::highScores() const {
    QVariantList list;
    for (const ModeInfo &mode : Modes::all()) {
        for (const DifficultyInfo &difficulty : Difficulties::all()) {
            const QString table = tableId(mode.mode, difficulty.difficulty);
            for (const QVariant &entry : m_scores.toVariantList(table)) {
                QVariantMap row = entry.toMap();
                row.insert(QStringLiteral("table"), table);
                row.insert(QStringLiteral("mode"), mode.id);
                row.insert(QStringLiteral("difficulty"), difficulty.id);
                row.insert(QStringLiteral("label"), difficulty.label);
                list.append(row);
            }
        }
    }
    return list;
}

QVariantMap OmasnakeGame::bests() const {
    QVariantMap map;
    for (const ModeInfo &mode : Modes::all()) {
        for (const DifficultyInfo &difficulty : Difficulties::all()) {
            const QString table = tableId(mode.mode, difficulty.difficulty);
            map.insert(table, m_scores.best(table));
        }
    }
    return map;
}
