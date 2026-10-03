// The C ABI of omagames_env.h, forwarded onto the game's OmaGames::Env.
// Everything a caller could get wrong is checked here, once for every game:
// the config, the action, stepping out of turn. The game's Env only ever sees
// calls that make sense.
#include "omagames_env.h"

#include <QJsonArray>
#include <QJsonDocument>

#include <cstring>
#include <vector>

#include "env.h"
#include "envconfig.h"

struct OgEnv {
    std::unique_ptr<OmaGames::Env> env;
    QJsonObject config;
    qint64 maxSteps = 0;
    qint64 steps = 0;
    bool started = false;
    bool over = false;
    // Backs the strings handed out for this env.
    QByteArray text;
};

namespace {

using OmaGames::Env;
using OmaGames::EnvStep;

const auto kMaxStepsKey = QStringLiteral("max_steps");

thread_local QByteArray t_lastError;

void fail(const QString &message) {
    t_lastError = message.toUtf8();
}

// The game's schema plus the one key every env has: an episode cap the ABI
// enforces, so no game has to count steps.
QJsonObject schema() {
    QJsonObject config = OmaGames::envGameSpec().value(QStringLiteral("config")).toObject();
    config.insert(kMaxStepsKey, QJsonObject{{QStringLiteral("default"), 0}, {QStringLiteral("min"), 0}});
    return config;
}

const char *hold(OgEnv *env, const QJsonObject &json) {
    env->text = QJsonDocument(json).toJson(QJsonDocument::Compact);
    return env->text.constData();
}

QJsonArray toArray(const QStringList &strings) {
    QJsonArray array;
    for (const QString &s : strings)
        array.append(s);
    return array;
}

// Empty when `action` may be stepped now, the reason otherwise.
QString refusal(const OgEnv *env, int action) {
    if (!env->started)
        return QStringLiteral("reset the env before stepping it");
    if (env->over)
        return QStringLiteral("the episode is over; reset the env");
    const int count = env->env->actionCount();
    if (action < 0 || action >= count)
        return QStringLiteral("action %1 is out of range 0..%2").arg(action).arg(count - 1);
    std::vector<quint8> mask(static_cast<size_t>(count));
    env->env->actionMask(mask.data());
    if (!mask[size_t(action)])
        return QStringLiteral("action %1 is masked").arg(action);
    return {};
}

void stepChecked(OgEnv *env, int action, OgStepResult *out) {
    const EnvStep step = env->env->step(action);
    ++env->steps;
    const bool truncated = !step.terminated && env->maxSteps > 0 && env->steps >= env->maxSteps;
    env->over = step.terminated || truncated;
    out->reward = step.reward;
    out->terminated = step.terminated;
    out->truncated = truncated;
    out->ticks = step.ticks;
    std::memcpy(out->signal_values, step.signalValues.data(), sizeof(out->signal_values));
}

// All zeros before the first reset: there is no game to show yet, and the
// game's Env never has to wonder whether it has one.
void observeInto(const OgEnv *env, void *buffer) {
    std::memset(buffer, 0, size_t(env->env->observationLayout().size()));
    if (env->started)
        env->env->observe(static_cast<std::byte *>(buffer));
}

}  // namespace

extern "C" {

int og_abi_version(void) {
    return OG_ABI_VERSION;
}

const char *og_game_spec(void) {
    static const QByteArray spec = [] {
        QJsonObject json = OmaGames::envGameSpec();
        json.insert(QStringLiteral("abi"), OG_ABI_VERSION);
        json.insert(QStringLiteral("config"), schema());
        return QJsonDocument(json).toJson(QJsonDocument::Compact);
    }();
    return spec.constData();
}

OgEnv *og_create(const char *config_json) {
    QJsonObject given;
    if (config_json && *config_json) {
        QJsonParseError parse;
        const QJsonDocument doc = QJsonDocument::fromJson(QByteArray(config_json), &parse);
        if (!doc.isObject()) {
            fail(parse.error == QJsonParseError::NoError ? QStringLiteral("the config must be a JSON object")
                                                         : parse.errorString());
            return nullptr;
        }
        given = doc.object();
    }
    QString error;
    const auto config = OmaGames::EnvConfig::resolve(schema(), given, &error);
    if (!config) {
        fail(error);
        return nullptr;
    }
    auto env = std::make_unique<OgEnv>();
    env->config = *config;
    env->maxSteps = config->value(kMaxStepsKey).toInteger();
    QJsonObject gameConfig = *config;
    gameConfig.remove(kMaxStepsKey);
    env->env = OmaGames::createEnv();
    env->env->configure(gameConfig);
    if (env->env->signalNames().size() > OG_MAX_SIGNALS) {
        fail(QStringLiteral("the game declares more than %1 signals").arg(OG_MAX_SIGNALS));
        return nullptr;
    }
    return env.release();
}

void og_destroy(OgEnv *env) {
    delete env;
}

const char *og_env_spec(OgEnv *env) {
    const Env &game = *env->env;
    return hold(env, {
        {QStringLiteral("config"), env->config},
        {QStringLiteral("observation"), game.observationLayout().toJson()},
        {QStringLiteral("actions"), QJsonObject{{QStringLiteral("count"), game.actionCount()},
                                                {QStringLiteral("labels"), toArray(game.actionLabels())}}},
        {QStringLiteral("signals"), toArray(game.signalNames())},
    });
}

void og_reset(OgEnv *env, uint32_t seed) {
    env->env->reset(seed);
    env->steps = 0;
    env->started = true;
    env->over = false;
}

int og_step(OgEnv *env, int32_t action, OgStepResult *out) {
    const QString problem = refusal(env, action);
    if (!problem.isEmpty()) {
        fail(problem);
        return -1;
    }
    stepChecked(env, action, out);
    return 0;
}

void og_observe(const OgEnv *env, void *buffer) {
    observeInto(env, buffer);
}

void og_action_mask(const OgEnv *env, uint8_t *mask) {
    if (!env->started || env->over)
        std::memset(mask, 0, size_t(env->env->actionCount()));
    else
        env->env->actionMask(mask);
}

OgEnv *og_clone(const OgEnv *env, int reseed_hidden, uint32_t seed) {
    auto copy = std::make_unique<OgEnv>();
    // Before a reset there is nothing hidden to redraw yet.
    copy->env = env->env->clone(reseed_hidden != 0 && env->started, seed);
    copy->config = env->config;
    copy->maxSteps = env->maxSteps;
    copy->steps = env->steps;
    copy->started = env->started;
    copy->over = env->over;
    return copy.release();
}

const char *og_replay_json(OgEnv *env) {
    return hold(env, env->env->replay().toJson());
}

const char *og_info_json(OgEnv *env) {
    return hold(env, env->started ? env->env->info() : QJsonObject());
}

const char *og_last_error(void) {
    return t_lastError.constData();
}

int og_step_batch(OgEnv *const *envs, int32_t n, const int32_t *actions, const uint32_t *seeds,
                  OgStepResult *results, void *obs, void *final_obs, uint8_t *masks) {
    if (n <= 0)
        return 0;
    const int obsSize = envs[0]->env->observationLayout().size();
    const int actionCount = envs[0]->env->actionCount();
    for (int i = 0; i < n; ++i) {
        const Env &game = *envs[i]->env;
        if (game.observationLayout().size() != obsSize || game.actionCount() != actionCount) {
            fail(QStringLiteral("env %1 has a different config from env 0").arg(i));
            return -1;
        }
        const QString problem = refusal(envs[i], actions[i]);
        if (!problem.isEmpty()) {
            fail(QStringLiteral("env %1: %2").arg(i).arg(problem));
            return -1;
        }
    }
    auto *obsRows = static_cast<std::byte *>(obs);
    auto *finalRows = static_cast<std::byte *>(final_obs);
    for (int i = 0; i < n; ++i) {
        OgEnv *env = envs[i];
        stepChecked(env, actions[i], &results[i]);
        if (env->over && seeds) {
            if (finalRows)
                observeInto(env, finalRows + qsizetype(i) * obsSize);
            og_reset(env, seeds[i]);
        }
        if (obsRows)
            observeInto(env, obsRows + qsizetype(i) * obsSize);
        if (masks)
            og_action_mask(env, masks + qsizetype(i) * actionCount);
    }
    return 0;
}

}  // extern "C"
