#pragma once

#include <QJsonObject>
#include <QStringList>

#include <array>
#include <cstddef>
#include <memory>
#include <optional>

#include "observationlayout.h"
#include "omagames_env.h"
#include "replay.h"

namespace OmaGames {

// What one step did. `signals` follow the env's signalNames(); the rest are
// left at zero. Truncation is not here: max_steps is the ABI's business.
struct EnvStep {
    double reward = 0;
    bool terminated = false;
    qint64 ticks = 0;
    std::array<double, OG_MAX_SIGNALS> signalValues {};
};

// A game as an agent plays it: the interface each games/<game>/env/ library
// implements and the C ABI (omagames_env.h) forwards to. It owns an engine,
// never a bridge, so nothing here reaches QSettings, a timer or QML.
//
// The ABI checks everything before it calls in: configure() only sees a
// config resolved against the game's schema, step() only actions the mask
// allows, and nothing but configure(), the spec and a plain clone() is
// asked for before the first reset.
class Env {
public:
    virtual ~Env() = default;

    virtual void configure(const QJsonObject &config) = 0;
    virtual const ObservationLayout &observationLayout() const = 0;
    virtual int actionCount() const = 0;
    // Optional names for the actions, for logs and the trainer's printouts.
    virtual QStringList actionLabels() const { return {}; }
    // At most OG_MAX_SIGNALS.
    virtual QStringList signalNames() const = 0;

    virtual void reset(quint32 seed) = 0;
    virtual EnvStep step(int action) = 0;
    // Writes every tensor of observationLayout(); the buffer arrives zeroed.
    virtual void observe(std::byte *buffer) const = 0;
    // actionCount() bytes, 1 for every action step() accepts.
    virtual void actionMask(quint8 *mask) const = 0;
    // With `reseedHidden`, whatever the player cannot see is redrawn from
    // `seed`: the bag past the next queue, a mine not yet placed.
    virtual std::unique_ptr<Env> clone(bool reseedHidden, quint32 seed) const = 0;
    virtual Replay replay() const = 0;
    virtual QJsonObject info() const { return {}; }
    // `replay` played back as frames for a viewer with no engine (see
    // og_replay_frames()); nothing and `error` if the game can't draw one.
    virtual std::optional<QJsonObject> frames(const Replay &replay, QString *error) const {
        Q_UNUSED(replay);
        *error = QStringLiteral("this game can't draw a replay outside the app");
        return std::nullopt;
    }
};

// Defined once in each env library: the game it plays.
//
// {game, rules_version, config: schema}, the schema as EnvConfig::resolve()
// reads it. rules_version goes up whenever the same inputs would play out
// differently, so stale replays and checkpoints are refused.
QJsonObject envGameSpec();
std::unique_ptr<Env> createEnv();

}  // namespace OmaGames
