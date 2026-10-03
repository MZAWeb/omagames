#pragma once

#include <QRandomGenerator>

#include <vector>

#include "env.h"

// The smallest game that exercises the whole env ABI, so common/ can test it
// without a real game: walk along a line to a cell the seed hid. Moving into
// a wall is masked, the hidden cell is what a reseeded clone redraws, and the
// line's length comes from the config, so the observation layout does too.
class WalkEnv : public OmaGames::Env {
public:
    enum Action { Left, Stay, Right };

    void configure(const QJsonObject &config) override;
    const OmaGames::ObservationLayout &observationLayout() const override { return m_layout; }
    int actionCount() const override { return 3; }
    QStringList actionLabels() const override;
    QStringList signalNames() const override;

    void reset(quint32 seed) override;
    OmaGames::EnvStep step(int action) override;
    void observe(std::byte *buffer) const override;
    void actionMask(quint8 *mask) const override;
    std::unique_ptr<OmaGames::Env> clone(bool reseedHidden, quint32 seed) const override;
    OmaGames::Replay replay() const override { return m_replay; }
    QJsonObject info() const override;

    int target() const { return m_target; }

private:
    void hideTarget(QRandomGenerator &rng);

    QJsonObject m_config;
    OmaGames::ObservationLayout m_layout;
    int m_positionTensor = 0;
    int m_visitedTensor = 0;
    int m_stepsTensor = 0;
    int m_length = 0;
    int m_start = 0;
    int m_position = 0;
    int m_target = 0;
    int m_steps = 0;
    std::vector<quint8> m_visited;
    OmaGames::Replay m_replay;
};
