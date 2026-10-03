/* The C ABI every game's agent environment library exports
 * (lib<game>_env.so). It is the only header a trainer depends on, and it is
 * plain C so that ctypes, cffi or any other language can load it without a
 * compiler. docs/AGENT-ENV.md describes the contract; common/env/env.h is the
 * C++ interface a game implements behind it.
 *
 * Strings returned by the library are UTF-8 JSON. The game spec and the last
 * error live as long as the library and the thread; every other string lives
 * until the next call that returns a string for the same env.
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__)
#define OG_EXPORT __attribute__((visibility("default")))
#else
#define OG_EXPORT
#endif

/* Bumped on any change a caller could notice: a function, a struct, the
 * shape of a JSON document. */
#define OG_ABI_VERSION 1
#define OG_MAX_SIGNALS 16

typedef struct OgEnv OgEnv;

typedef struct {
    /* The game's own reward: what its score would count. */
    double reward;
    /* The rules ended the episode: a top out, a goal reached. */
    int32_t terminated;
    /* The env's own `max_steps` ended it; the game itself could go on. */
    int32_t truncated;
    /* Engine ticks the step consumed, for games that run on a clock. */
    int64_t ticks;
    /* What happened, by the names in the env spec's `signals`. */
    double signal_values[OG_MAX_SIGNALS];
} OgStepResult;

OG_EXPORT int og_abi_version(void);

/* {abi, game, rules_version, config: {key: {default, choices | min, max}}}.
 * Needs no env, so a trainer can list what is configurable first. */
OG_EXPORT const char *og_game_spec(void);

/* A new env from a JSON object of config keys; missing keys take their
 * defaults, unknown or invalid ones fail. NULL and og_last_error() on
 * failure. The env must be reset before it is stepped. */
OG_EXPORT OgEnv *og_create(const char *config_json);
OG_EXPORT void og_destroy(OgEnv *env);

/* {config, observation: {size, tensors: [{name, dtype, shape, offset}]},
 *  actions: {count, labels}, signals: [names]}, for this env's config. */
OG_EXPORT const char *og_env_spec(OgEnv *env);

OG_EXPORT void og_reset(OgEnv *env, uint32_t seed);

/* 0, or -1 and og_last_error() when the action is out of range or masked,
 * or the episode has not started or is over. */
OG_EXPORT int og_step(OgEnv *env, int32_t action, OgStepResult *out);

/* Writes the observation: `size` bytes laid out as the spec says; all zero
 * before the first reset. */
OG_EXPORT void og_observe(const OgEnv *env, void *buffer);

/* Writes `count` bytes, 1 for every action og_step would accept. All zero
 * before the first reset and after the episode ends. */
OG_EXPORT void og_action_mask(const OgEnv *env, uint8_t *mask);

/* A deep copy, steps taken included. With reseed_hidden, whatever the player
 * cannot see is redrawn from `seed`, so a planner samples the future rather
 * than reading it. */
OG_EXPORT OgEnv *og_clone(const OgEnv *env, int reseed_hidden, uint32_t seed);

/* The episode so far as replay/v1 (common/env/replay.h). */
OG_EXPORT const char *og_replay_json(OgEnv *env);

/* Whatever the game finds worth printing about its state; {} before the
 * first reset. */
OG_EXPORT const char *og_info_json(OgEnv *env);

/* Why the last call on this thread failed; "" when none has. */
OG_EXPORT const char *og_last_error(void);

/* Steps n envs that share one config, in one call. `obs`, `final_obs` and
 * `masks` hold n rows each and may be NULL. When `seeds` is not NULL an env
 * whose episode ended is reset from seeds[i] in place and its last
 * observation goes to final_obs; `obs` and `masks` then describe the new
 * episode. Every action is checked before any env moves, so on -1 nothing
 * was stepped. */
OG_EXPORT int og_step_batch(OgEnv *const *envs, int32_t n, const int32_t *actions,
                            const uint32_t *seeds, OgStepResult *results, void *obs,
                            void *final_obs, uint8_t *masks);

#ifdef __cplusplus
}
#endif
