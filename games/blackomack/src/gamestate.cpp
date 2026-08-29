#include "gamestate.h"

#include <QJsonArray>
#include <QJsonDocument>

QJsonObject GameState::toJson() const {
    QJsonArray botArray;
    for (const Bot &b : bots) {
        botArray.append(QJsonObject{
            {QStringLiteral("name"), b.personality.name},
            {QStringLiteral("skill"), b.personality.skill},
            {QStringLiteral("aggression"), b.personality.aggression},
            {QStringLiteral("seed"), double(b.personality.seed)},
            {QStringLiteral("bankroll"), b.bankroll},
        });
    }
    return {
        {QStringLiteral("bankroll"), bankroll},
        {QStringLiteral("bestBankroll"), bestBankroll},
        {QStringLiteral("bots"), botArray},
        {QStringLiteral("handsPlayed"), handsPlayed},
        {QStringLiteral("netResult"), netResult},
    };
}

GameState GameState::fromJson(const QJsonObject &json) {
    GameState state;
    state.bankroll = json.value(QStringLiteral("bankroll")).toInt(state.bankroll);
    // A save from before the high score existed still held a record: whatever
    // was on the table, or the stake everyone starts with.
    state.bestBankroll = json.value(QStringLiteral("bestBankroll"))
                             .toInt(qMax(state.bankroll, BlackjackRules::kStartingBankroll));
    state.handsPlayed = json.value(QStringLiteral("handsPlayed")).toInt();
    state.netResult = json.value(QStringLiteral("netResult")).toInt();
    const QJsonArray botArray = json.value(QStringLiteral("bots")).toArray();
    for (const QJsonValue &v : botArray) {
        const QJsonObject o = v.toObject();
        Bot b;
        b.personality.name = o.value(QStringLiteral("name")).toString();
        b.personality.skill = o.value(QStringLiteral("skill")).toDouble();
        b.personality.aggression = o.value(QStringLiteral("aggression")).toDouble();
        b.personality.seed = quint32(o.value(QStringLiteral("seed")).toDouble());
        b.bankroll = o.value(QStringLiteral("bankroll")).toInt();
        if (!b.personality.name.isEmpty())
            state.bots.append(b);
    }
    return state;
}

QString GameState::toString() const {
    return QString::fromUtf8(QJsonDocument(toJson()).toJson(QJsonDocument::Compact));
}

GameState GameState::fromString(const QString &text) {
    return fromJson(QJsonDocument::fromJson(text.toUtf8()).object());
}
