#pragma once

#include <QJsonObject>
#include <QString>

#include <optional>
#include <vector>

namespace OmaGames {

// One episode as replay/v1: the game, its rules version, the config and
// seed it was dealt from, and every engine input in order with the ticks
// between them. The engines are deterministic, so that is the whole game,
// however the agent chose its actions. It is recorded below any action
// space, so the app can play back an episode without knowing about one.
//
// On disk the inputs are one token string, "t60 L L CW t1 HD": `tN` lets N
// ticks pass and every other token is an input the game defines. Inputs
// never start with 't' and never hold whitespace.
class Replay {
public:
    static constexpr const char *kFormat = "replay/v1";

    // Exactly one of the two is set: a run of ticks, or one input.
    struct Step {
        int ticks = 0;
        QString input;
    };

    Replay() = default;
    Replay(QString game, int rulesVersion, QJsonObject config, quint32 seed);

    const QString &game() const { return m_game; }
    int rulesVersion() const { return m_rulesVersion; }
    const QJsonObject &config() const { return m_config; }
    quint32 seed() const { return m_seed; }
    // Who played it, for the app's header; the trainer fills it in.
    const QString &agent() const { return m_agent; }
    void setAgent(const QString &agent) { m_agent = agent; }

    const std::vector<Step> &steps() const { return m_steps; }
    // Ticks in a row merge into one step, so an idle stretch costs one token.
    void advance(int ticks);
    void input(const QString &token);

    QString calls() const;
    QJsonObject toJson() const;
    // std::nullopt and `error` for anything that is not a replay/v1 this
    // code could play back.
    static std::optional<Replay> fromJson(const QJsonObject &json, QString *error);

    static bool validInput(const QString &token);

private:
    QString m_game;
    int m_rulesVersion = 0;
    QJsonObject m_config;
    quint32 m_seed = 0;
    QString m_agent;
    std::vector<Step> m_steps;
};

}  // namespace OmaGames
