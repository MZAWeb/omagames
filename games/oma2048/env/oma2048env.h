#pragma once

#include <optional>

#include "env.h"
#include "game.h"
#include "rules.h"

// Oma2048 for an agent (docs/AGENT-ENV.md). It owns a Game and nothing else
// of the app: no bridge, no settings, no high scores, no undo.
//
// One step is one slide: left, right, up or down, a slide that would change
// nothing masked. The new tile then spawns, from the game's seed. Besides the
// board, the observation has each slide's afterstate, the board it leaves
// before the spawn, so an agent can rate where a move leads without playing
// it; what spawns is the only part left to chance.
class Oma2048Env : public OmaGames::Env {
public:
    static constexpr int kRulesVersion = Rules::kVersion;
    static constexpr int kActions = 4;  // Direction order: left, right, up, down

    void configure(const QJsonObject &config) override;
    const OmaGames::ObservationLayout &observationLayout() const override { return m_layout; }
    int actionCount() const override { return kActions; }
    QStringList actionLabels() const override;
    QStringList signalNames() const override;

    void reset(quint32 seed) override;
    OmaGames::EnvStep step(int action) override;
    void observe(std::byte *buffer) const override;
    void actionMask(quint8 *mask) const override;
    std::unique_ptr<OmaGames::Env> clone(bool reseedHidden, quint32 seed) const override;
    OmaGames::Replay replay() const override { return m_replay; }
    QJsonObject info() const override;
    std::optional<QJsonObject> frames(const OmaGames::Replay &replay, QString *error) const override;

    const Game &game() const { return *m_game; }
    // The run reached the configured goal tile, which ends it.
    bool reachedGoal() const { return m_goal > 0 && m_game->board().highestValue() >= m_goal; }

private:
    QJsonObject m_config;
    int m_goal = 0;
    OmaGames::ObservationLayout m_layout;
    int m_board = 0;
    int m_afterstates = 0;
    int m_gains = 0;
    int m_stats = 0;
    int m_moves = 0;
    std::optional<Game> m_game;
    OmaGames::Replay m_replay;
};
