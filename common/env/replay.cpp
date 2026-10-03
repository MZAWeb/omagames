#include "replay.h"

#include <QStringList>

#include <algorithm>
#include <limits>

namespace OmaGames {

namespace {

const auto kFormatKey = QStringLiteral("format");
const auto kGameKey = QStringLiteral("game");
const auto kRulesKey = QStringLiteral("rules_version");
const auto kConfigKey = QStringLiteral("config");
const auto kSeedKey = QStringLiteral("seed");
const auto kAgentKey = QStringLiteral("agent");
const auto kCallsKey = QStringLiteral("calls");

}  // namespace

Replay::Replay(QString game, int rulesVersion, QJsonObject config, quint32 seed)
    : m_game(std::move(game)), m_rulesVersion(rulesVersion), m_config(std::move(config)),
      m_seed(seed) {}

void Replay::advance(int ticks) {
    if (ticks <= 0)
        return;
    if (!m_steps.empty() && m_steps.back().input.isEmpty())
        m_steps.back().ticks += ticks;
    else
        m_steps.push_back({ticks, QString()});
}

void Replay::input(const QString &token) {
    Q_ASSERT(validInput(token));
    m_steps.push_back({0, token});
}

QString Replay::calls() const {
    QStringList tokens;
    tokens.reserve(qsizetype(m_steps.size()));
    for (const Step &step : m_steps)
        tokens << (step.input.isEmpty() ? QStringLiteral("t%1").arg(step.ticks) : step.input);
    return tokens.join(QLatin1Char(' '));
}

QJsonObject Replay::toJson() const {
    QJsonObject json{
        {kFormatKey, QString::fromLatin1(kFormat)},
        {kGameKey, m_game},
        {kRulesKey, m_rulesVersion},
        {kConfigKey, m_config},
        {kSeedKey, qint64(m_seed)},
        {kCallsKey, calls()},
    };
    if (!m_agent.isEmpty())
        json.insert(kAgentKey, m_agent);
    return json;
}

std::optional<Replay> Replay::fromJson(const QJsonObject &json, QString *error) {
    if (json.value(kFormatKey).toString() != QLatin1String(kFormat)) {
        *error = QStringLiteral("not a %1 file").arg(QLatin1String(kFormat));
        return std::nullopt;
    }
    const qint64 seed = json.value(kSeedKey).toInteger(-1);
    if (json.value(kGameKey).toString().isEmpty() || !json.value(kRulesKey).isDouble()
        || seed < 0 || seed > qint64(std::numeric_limits<quint32>::max())) {
        *error = QStringLiteral("a replay needs a game, a rules_version and a 32-bit seed");
        return std::nullopt;
    }
    Replay replay(json.value(kGameKey).toString(), json.value(kRulesKey).toInt(),
                  json.value(kConfigKey).toObject(), quint32(seed));
    replay.setAgent(json.value(kAgentKey).toString());
    const QStringList tokens = json.value(kCallsKey).toString().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (const QString &token : tokens) {
        if (token.startsWith(QLatin1Char('t'))) {
            bool ok = false;
            const int ticks = token.mid(1).toInt(&ok);
            if (!ok || ticks <= 0) {
                *error = QStringLiteral("bad tick count \"%1\"").arg(token);
                return std::nullopt;
            }
            replay.advance(ticks);
        } else {
            replay.input(token);
        }
    }
    return replay;
}

bool Replay::validInput(const QString &token) {
    if (token.isEmpty() || token.startsWith(QLatin1Char('t')))
        return false;
    return std::none_of(token.begin(), token.end(), [](QChar c) { return c.isSpace(); });
}

}  // namespace OmaGames
