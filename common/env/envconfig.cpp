#include "envconfig.h"

#include <QJsonArray>
#include <QStringList>

#include <cmath>

namespace OmaGames::EnvConfig {

namespace {

bool isInteger(const QJsonValue &value) {
    return value.isDouble() && std::floor(value.toDouble()) == value.toDouble();
}

// Empty when `value` is acceptable for `rule`, the reason otherwise.
QString check(const QString &key, const QJsonObject &rule, const QJsonValue &value) {
    const QJsonValue fallback = rule.value(QStringLiteral("default"));
    if (fallback.isString()) {
        QStringList choices;
        for (const QJsonValue &choice : rule.value(QStringLiteral("choices")).toArray())
            choices << choice.toString();
        if (!value.isString() || !choices.contains(value.toString()))
            return QStringLiteral("\"%1\" must be one of: %2").arg(key, choices.join(QStringLiteral(", ")));
        return {};
    }
    if (fallback.isBool())
        return value.isBool() ? QString() : QStringLiteral("\"%1\" must be true or false").arg(key);
    if (!isInteger(value))
        return QStringLiteral("\"%1\" must be an integer").arg(key);
    const double number = value.toDouble();
    const QJsonValue min = rule.value(QStringLiteral("min"));
    const QJsonValue max = rule.value(QStringLiteral("max"));
    if ((min.isDouble() && number < min.toDouble()) || (max.isDouble() && number > max.toDouble())) {
        return QStringLiteral("\"%1\" must be between %2 and %3")
            .arg(key, min.isDouble() ? QString::number(min.toInteger()) : QStringLiteral("-inf"),
                 max.isDouble() ? QString::number(max.toInteger()) : QStringLiteral("inf"));
    }
    return {};
}

}  // namespace

std::optional<QJsonObject> resolve(const QJsonObject &schema, const QJsonObject &given,
                                   QString *error) {
    for (auto it = given.begin(); it != given.end(); ++it) {
        if (!schema.contains(it.key())) {
            *error = QStringLiteral("unknown config key \"%1\"; known: %2")
                         .arg(it.key(), schema.keys().join(QStringLiteral(", ")));
            return std::nullopt;
        }
    }
    QJsonObject resolved;
    for (auto it = schema.begin(); it != schema.end(); ++it) {
        const QJsonObject rule = it.value().toObject();
        const QJsonValue value = given.contains(it.key()) ? given.value(it.key())
                                                          : rule.value(QStringLiteral("default"));
        const QString problem = check(it.key(), rule, value);
        if (!problem.isEmpty()) {
            *error = problem;
            return std::nullopt;
        }
        resolved.insert(it.key(), value);
    }
    return resolved;
}

}  // namespace OmaGames::EnvConfig
