#include "walkenv.h"

#include <QJsonArray>

using OmaGames::EnvStep;
using OmaGames::ObservationLayout;

namespace {

const auto kGame = QStringLiteral("walk");
constexpr int kRulesVersion = 1;

}  // namespace

QJsonObject OmaGames::envGameSpec() {
    return {
        {QStringLiteral("game"), kGame},
        {QStringLiteral("rules_version"), kRulesVersion},
        {QStringLiteral("config"),
         QJsonObject{
             {QStringLiteral("length"),
              QJsonObject{{QStringLiteral("default"), 9}, {QStringLiteral("min"), 3}, {QStringLiteral("max"), 64}}},
             {QStringLiteral("start"),
              QJsonObject{{QStringLiteral("default"), QStringLiteral("middle")},
                          {QStringLiteral("choices"), QJsonArray{QStringLiteral("middle"), QStringLiteral("left")}}}},
         }},
    };
}

std::unique_ptr<OmaGames::Env> OmaGames::createEnv() {
    return std::make_unique<WalkEnv>();
}

void WalkEnv::configure(const QJsonObject &config) {
    m_config = config;
    m_length = config.value(QStringLiteral("length")).toInt();
    m_start = config.value(QStringLiteral("start")).toString() == QStringLiteral("left") ? 0 : m_length / 2;
    m_layout = ObservationLayout();
    m_positionTensor = m_layout.add(QStringLiteral("position"), ObservationLayout::DType::I32, {1});
    m_visitedTensor = m_layout.add(QStringLiteral("visited"), ObservationLayout::DType::U8, {m_length});
    m_stepsTensor = m_layout.add(QStringLiteral("steps"), ObservationLayout::DType::I32, {1});
}

QStringList WalkEnv::actionLabels() const {
    return {QStringLiteral("left"), QStringLiteral("stay"), QStringLiteral("right")};
}

QStringList WalkEnv::signalNames() const {
    return {QStringLiteral("moved")};
}

void WalkEnv::reset(quint32 seed) {
    m_position = m_start;
    m_steps = 0;
    m_visited.assign(size_t(m_length), 0);
    m_visited[size_t(m_position)] = 1;
    QRandomGenerator rng(seed);
    hideTarget(rng);
    m_replay = OmaGames::Replay(kGame, kRulesVersion, m_config, seed);
}

void WalkEnv::hideTarget(QRandomGenerator &rng) {
    // Anywhere but underfoot, so an episode always takes at least one step.
    m_target = int(rng.bounded(m_length - 1));
    if (m_target >= m_position)
        ++m_target;
}

EnvStep WalkEnv::step(int action) {
    static const QString kInputs[] = {QStringLiteral("L"), QStringLiteral("S"), QStringLiteral("R")};
    m_replay.input(kInputs[action]);
    m_replay.advance(1);
    const int from = m_position;
    m_position += action - Stay;
    m_visited[size_t(m_position)] = 1;
    ++m_steps;

    EnvStep step;
    step.terminated = m_position == m_target;
    step.reward = step.terminated ? 1 : 0;
    step.ticks = 1;
    step.signalValues[0] = m_position != from;
    return step;
}

void WalkEnv::observe(std::byte *buffer) const {
    *m_layout.at<qint32>(buffer, m_positionTensor) = m_position;
    std::copy(m_visited.begin(), m_visited.end(), m_layout.at<quint8>(buffer, m_visitedTensor));
    *m_layout.at<qint32>(buffer, m_stepsTensor) = m_steps;
}

void WalkEnv::actionMask(quint8 *mask) const {
    mask[Left] = m_position > 0;
    mask[Stay] = 1;
    mask[Right] = m_position < m_length - 1;
}

std::unique_ptr<OmaGames::Env> WalkEnv::clone(bool reseedHidden, quint32 seed) const {
    auto copy = std::make_unique<WalkEnv>(*this);
    if (reseedHidden) {
        QRandomGenerator rng(seed);
        copy->hideTarget(rng);
    }
    return copy;
}

QJsonObject WalkEnv::info() const {
    return {{QStringLiteral("position"), m_position}, {QStringLiteral("steps"), m_steps}};
}
