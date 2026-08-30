// OmanixGame: the difficulty table and the score tables as the lists and maps
// QML reads.
#include "omanixgame.h"

#include "difficulties.h"

QString OmanixGame::difficultyLabel() const {
    return Difficulties::info(m_difficulty).label;
}

QVariantList OmanixGame::difficulties() {
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

QVariantList OmanixGame::highScores() const {
    QVariantList list;
    for (const DifficultyInfo &info : Difficulties::all()) {
        for (const QVariant &entry : m_scores.toVariantList(info.id)) {
            QVariantMap row = entry.toMap();
            row.insert(QStringLiteral("difficulty"), info.id);
            row.insert(QStringLiteral("label"), info.label);
            list.append(row);
        }
    }
    return list;
}

QVariantMap OmanixGame::bests() const {
    QVariantMap map;
    for (const DifficultyInfo &info : Difficulties::all())
        map.insert(info.id, m_scores.best(info.id));
    return map;
}

QVariantMap OmanixGame::levelStats() const {
    if (!m_game)
        return {};
    const LevelStats &stats = m_game->lastLevel();
    return {
        {QStringLiteral("percent"), stats.percent},
        {QStringLiteral("bonus"), stats.livesBonus},
        {QStringLiteral("seconds"), stats.ticks / Level::kTicksPerSecond},
    };
}
