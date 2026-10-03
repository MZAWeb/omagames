#pragma once

#include <QJsonObject>
#include <QString>

#include <optional>

namespace OmaGames::EnvConfig {

// Checks `given` against a game's config schema and fills in the defaults,
// so a game's Env::configure() only ever sees a complete, valid config.
//
// A schema maps each key to {default, ...}, and the default's type is the
// key's type:
//   string  {"default": "marathon", "choices": ["marathon", "sprint"]}
//   integer {"default": 0, "min": 0, "max": 100}  (both bounds optional)
//   bool    {"default": true}
//
// An unknown key is an error rather than ignored: a typo in a trainer's
// config must not silently train on the default.
std::optional<QJsonObject> resolve(const QJsonObject &schema, const QJsonObject &given,
                                   QString *error);

}  // namespace OmaGames::EnvConfig
