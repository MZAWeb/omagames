#pragma once

#include <optional>
#include <vector>

#include "env.h"
#include "omatrisobservation.h"

// Omatris for an agent (docs/AGENT-ENV.md). It owns a Game and nothing else
// of the app: no bridge, no settings, no high scores. Every call it makes on
// the game goes through Calls, so the replay it keeps is exactly the game.
//
// Three ways to act, picked by the `actions` config key:
//   placement  pick one of the landings Placements::find() offers, hold
//              included; the piece goes there, is hard-dropped, and time
//              runs until the next piece is in play.
//   drop       hold or not × rotation × leftmost column, then a hard drop:
//              a fixed set of 80 for a fixed-size output head.
//   raw        one input (or none), then `frame_skip` ticks.
// The game plays with an instant soft drop, which is what the placement
// search assumes; a raw soft drop is therefore a sonic drop too.
class OmatrisEnv : public OmaGames::Env {
public:
    static constexpr int kRulesVersion = Rules::kVersion;

    enum class Space { Placement, Drop, Raw };
    enum RawAction { None, Left, Right, RotateCW, RotateCCW, SoftDrop, HardDrop, Hold, kRawActions };
    static constexpr int kDropActions = 2 * Piece::kStates * Board::kWidth;

    void configure(const QJsonObject &config) override;
    const OmaGames::ObservationLayout &observationLayout() const override { return m_observation->layout(); }
    int actionCount() const override;
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
    // What the placement space offers now, in action order.
    const std::vector<Landing> &landings() const { return m_landings; }
    // The drop space's action for hold or not, a rotation and a column.
    static int dropAction(bool hold, int rotation, int column);

private:
    std::vector<Event> call(Call call, int *ticks);
    void land(const Landing &landing, std::vector<Event> &events, int *ticks);
    void offerActions();

    QJsonObject m_config;
    Space m_space = Space::Placement;
    Mode m_mode = Mode::Marathon;
    bool m_holdAllowed = true;
    int m_candidates = 0;
    int m_frameSkip = 1;
    std::optional<OmatrisObservation> m_observation;
    std::optional<Game> m_game;
    std::vector<Landing> m_landings;
    // Landings the search found beyond the `candidates` the agent is shown.
    int m_unoffered = 0;
    std::vector<std::optional<Landing>> m_drops;
    OmaGames::Replay m_replay;
};
