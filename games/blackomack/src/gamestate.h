#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>

#include "botplayer.h"

// What survives between launches: the human's stack, the bots at the table and
// session stats. Stored as one JSON blob under a versioned QSettings key so the
// format can migrate later.
struct GameState {
    struct Bot {
        BotPersonality personality;
        int bankroll = 0;
    };

    int bankroll = BlackjackRules::kStartingBankroll;
    QVector<Bot> bots;
    int handsPlayed = 0;
    int netResult = 0;

    static constexpr const char *kKey = "state/v1";

    QJsonObject toJson() const;
    static GameState fromJson(const QJsonObject &json);
    QString toString() const;
    static GameState fromString(const QString &text);
};
