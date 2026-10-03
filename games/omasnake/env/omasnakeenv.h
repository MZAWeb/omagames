#pragma once

#include <optional>

#include "env.h"
#include "game.h"

// Omasnake for an agent (docs/AGENT-ENV.md). It owns a Game and nothing else
// of the app: no bridge, no settings, no high scores.
//
// One step is one move of the snake: the turn the agent chose, then ticks
// until the head has moved, the opening "Ready" beat included. Nothing the
// agent could do between moves would change anything, so those ticks are
// not decisions. Two ways to steer, picked by the `actions` config key:
//   relative  straight on, turn left, turn right, against the heading
//   absolute  up, down, left, right; the way back onto the neck is masked
class OmasnakeEnv : public OmaGames::Env {
public:
    static constexpr int kRulesVersion = Rules::kVersion;

    enum Relative { Straight, TurnLeft, TurnRight, kRelativeActions };
    static constexpr int kAbsoluteActions = 4;

    void configure(const QJsonObject &config) override;
    const OmaGames::ObservationLayout &observationLayout() const override { return m_layout; }
    int actionCount() const override { return m_relative ? kRelativeActions : kAbsoluteActions; }
    QStringList actionLabels() const override;
    QStringList signalNames() const override;

    void reset(quint32 seed) override;
    OmaGames::EnvStep step(int action) override;
    void observe(std::byte *buffer) const override;
    void actionMask(quint8 *mask) const override;
    std::unique_ptr<OmaGames::Env> clone(bool reseedHidden, quint32 seed) const override;
    OmaGames::Replay replay() const override { return m_replay; }
    QJsonObject info() const override;

    const Game &game() const { return *m_game; }
    // The heading an action asks for; absolute actions are Direction order.
    Direction headingFor(int action) const;

private:
    QJsonObject m_config;
    bool m_relative = true;
    Mode m_mode = Mode::Classic;
    Difficulty m_difficulty = Difficulty::Normal;
    OmaGames::ObservationLayout m_layout;
    int m_grid = 0;
    int m_state = 0;
    std::optional<Game> m_game;
    OmaGames::Replay m_replay;
};
