# Agent environments: playing omagames from code

Status: **design, not built yet.** This is the omagames half of the plan. The
other half, the training project that consumes it, is in `docs/OMAGYM.md`.

## Goal

A separate project trains agents (reinforcement learning, evolutionary search,
plain heuristics) to play these games, first Omatris and later any of them.
It has to be able to:

- read the world state as numbers, without screenshots;
- act, at whatever level of abstraction an experiment wants (a key, a
  placement);
- run far faster than real time, many games in parallel, headless;
- clone a game to plan ahead (MCTS, beam search);
- hand a finished game back to the real app so a person can watch it.

## Why not drive the GUI

The Trackmania videos had no choice: they captured the screen and sent
keystrokes to a closed game running in real time. That ties every experiment
to 60 frames a second, one game per window, and flaky timing.

We do not have that limitation. `ARCHITECTURE.md` already requires every
engine to be QtCore-only, deterministic from an explicit seed, and stepped by
`tick()` or an action call with no timers. The bridge is the only thing that
paces it. So the engines *are* the simulator: an agent should call them the
way the tests do, and skip QML, the bridge and the clock.

| Option | Speed | Cost to omagames | Verdict |
|---|---|---|---|
| Drive the app (keys in, screenshots out) | real time, one game per window | none | Only for a final demo. |
| Line-based JSON server over stdin/stdout | ~10⁴ steps/s per process, any language | small binary per game | Useful later for remote or non-Python clients; not the core. |
| **C ABI shared library per game** (`libomatris_env.so`) loaded with `ctypes` | ~10⁵–10⁶ engine calls/s per core; observations written straight into numpy buffers | one small `lib` target per game, QtCore only, no new deps | **Chosen.** |
| pybind11 / nanobind module | as fast | adds a build dependency and Python headers to this repo | Rejected: omagames stays Qt + qmake only. |

The speeds are targets to measure, not measurements.

`ctypes` needs no compiler on the Python side, and it releases the GIL during
a foreign call, so Python threads stepping separate batches of environments
run in parallel.

## Ground rules

These extend `CLAUDE.md` and are not negotiable:

1. **The env never reimplements a rule.** It calls the same engine methods the
   bridge calls (`moveLeft`, `rotate`, `hardDrop`, `tick`, …). Even the search
   for reachable placements works by moving copies of the real `Game`.
2. **QtCore only, no new dependencies.** The env library links what the test
   binary links.
3. **The env never touches `QSettings`.** No high scores, no handling, no
   window geometry: a million training games must not end up in a player's
   `omatris.conf`. The env owns an engine, never a bridge.
4. **Observations show only what a player can see.** That means the next
   queue and not the bag behind it, revealed cells and not mines, the
   dealer's up-card and not the hole card. Cloning for planning has a mode
   that reseeds the hidden randomness, so a planner samples the future
   instead of reading it (see `clone`).
5. **Deterministic.** The same seed and the same actions always give the same
   game, and a test enforces it per game.
6. **Every env is tested** in the game's QtTest suite, like any rule.

## Shape

```
common/env/
  env.h            OmaGames::Env: the abstract C++ interface every game implements
  envspec.h/.cpp   builder for the JSON spec and the observation layout
  omagames_env.h   the C ABI (the only header the trainer depends on)
  envabi.cpp       the C shim: forwards the ABI onto any OmaGames::Env
  replay.h/.cpp    replay/v1: seed + engine calls, read and written
  env.pri          included by each game's env.pro
games/<game>/env/
  <game>env.h/.cpp the game's Env: observation, actions, reward signals
  env.pro          TEMPLATE = lib, CONFIG += shared, QT = core; lists the
                   engine sources the same way tests/tests.pro does
bin/build-env      bin/build-env omatris → build-env/omatris/libomatris_env.so
```

The env's tests live in the game's existing suite (`bin/test omatris`). They
test `OmatrisEnv` through its C++ interface, and the C shim gets one
round-trip test in `common/tests`. CI builds `env/` for every game that has
one, using the same discovery matrix it already uses. Env libraries are not
shipped in the PKGBUILDs.

Adding an env to another game means writing one class, about 200 lines,
plus its tests. The ABI, batching, replays and the Python side are shared.

### `OmaGames::Env` (C++)

```cpp
class Env {
public:
    virtual ~Env() = default;
    // JSON: observation layout, action spaces, signals, config schema,
    // rules_version. Static per game, so the trainer can size buffers first.
    virtual QJsonObject spec() const = 0;
    virtual bool configure(const QJsonObject &config) = 0;   // mode, action space, frame skip…
    virtual void reset(quint32 seed) = 0;
    virtual StepResult step(int action) = 0;
    virtual void observe(std::byte *out) const = 0;          // spec().observation layout
    virtual void actionMask(quint8 *out) const = 0;          // 1 = legal
    // A deep copy. `reseedHidden` reshuffles whatever the player cannot see
    // (the bag past the next queue, an unplaced mine, the shoe) from `seed`,
    // so a planner's rollouts are honest.
    virtual std::unique_ptr<Env> clone(bool reseedHidden, quint32 seed) const = 0;
    virtual Replay replay() const = 0;                       // the episode so far
    virtual QJsonObject info() const = 0;                    // human-readable stats, for debugging
};
```

### The C ABI (`omagames_env.h`)

```c
#define OG_ABI_VERSION 1
#define OG_MAX_SIGNALS 16

typedef struct OgEnv OgEnv;

typedef struct {
    double  reward;                  /* the game's default reward (Omatris: score gained) */
    int32_t terminated;              /* the rules ended the run: top out, goal reached */
    int32_t truncated;               /* the env's own step limit */
    int64_t ticks;                   /* engine ticks this step consumed */
    double  signals[OG_MAX_SIGNALS]; /* named in the spec: lines, pieces, holes… */
} OgStepResult;

int          og_abi_version(void);
const char  *og_spec(void);                          /* JSON, static */
OgEnv       *og_create(const char *config_json);     /* NULL + og_last_error() on bad config */
void         og_destroy(OgEnv *env);
void         og_reset(OgEnv *env, uint32_t seed);
void         og_step(OgEnv *env, int32_t action, OgStepResult *out);
void         og_observe(const OgEnv *env, void *buffer);
void         og_action_mask(const OgEnv *env, uint8_t *mask);
OgEnv       *og_clone(const OgEnv *env, int reseed_hidden, uint32_t seed);
const char  *og_replay_json(const OgEnv *env);      /* valid until the next call on env */
const char  *og_info_json(const OgEnv *env);
const char  *og_last_error(void);

/* N envs in one call: one foreign call per batch instead of per env.
   An env that terminates is reset in place from the next of `seeds` and
   its terminal observation is written to `final_obs`, Gymnasium-style. */
void og_step_batch(OgEnv **envs, int n, const int32_t *actions,
                   const uint32_t *seeds, OgStepResult *results,
                   void *obs, void *final_obs, uint8_t *masks);
```

The observation is one contiguous buffer. The spec gives each tensor's name,
dtype, shape and byte offset, so Python builds zero-copy numpy views over a
buffer it owns. Envs share no global state, so different envs can be stepped
on different threads.

`rules_version` in the spec is bumped whenever a rule constant or behaviour
changes. That covers, for example, `kMaxLockResets` and anything else that
changes what a sequence of actions does. Replays and trained models record
it, so a stale checkpoint is refused instead of quietly scoring worse.

### Rewards are signals, not opinions

The env reports what happened, and the trainer decides what that's worth.
`reward` is the game's own score delta, so a trainer works out of the box.
Each env also declares named `signals` (for Omatris: score gained, lines
cleared, pieces placed, T-spin, holes and max height after the step, game
over). Reward shaping depends on the experiment, so it lives in the trainer.
Signals that are expensive to recompute in Python, like holes, are computed
here once.

## Omatris: the first env

### Observation

| Tensor | dtype | shape | Content |
|---|---|---|---|
| `board` | u8 | 24 × 10 | 0 empty, 1–7 the piece that filled it; four hidden rows on top |
| `piece` | i32 | 4 | type, rotation, x, y of the falling piece |
| `queue` | u8 | 3 | the next three |
| `hold` | u8 | 2 | held type (0 none), hold available |
| `stats` | i32 | 8 | level, gravity level, lines, score, combo, back-to-back, lock ticks, lock resets |
| `candidates` | i32 | K × 6 | placement action space only: rotation, x, y, uses hold, lines it clears, valid |
| `afterstates` | u8 | K × 24 × 10 | placement action space only: the board after each candidate locks and clears |

### Action spaces (chosen in the config)

| Space | One step is | Size | For |
|---|---|---|---|
| `placement` | a whole piece: one of this piece's reachable resting places, hold included, executed with real engine calls, then hard-dropped and ticked until the next piece is in play | K = 256 slots, masked; the `candidates` and `afterstates` tensors describe each slot | afterstate value learning, heuristics, CEM. **Start here.** |
| `drop` | hold or not × rotation × column, then hard drop | 80, masked | plain DQN/PPO with a fixed output head; cannot tuck or spin |
| `raw` | one engine input, then `frame_skip` ticks | 8: none, left, right, rotate CW, rotate CCW, soft drop on/off, hard drop, hold | the "Trackmania" setting: hard credit assignment, real timing |

The `placement` search is a breadth-first search over copies of `Game`. From
the spawned piece (and the held or next piece, when hold is available) it
tries every move and rotation, keeps a state when the engine accepted the
input, and records each distinct resting place with the shortest input path
to it. Running on the real engine means wall kicks, T-spin detection and the
two lock resets are respected without a second implementation. Executing a
candidate replays its path with no ticks in between, which is what an
infinitely fast finesse player would do. That's honest at low gravity and
generous at level 15+. A `placement_timing = "realistic"` option replays the
path at the default ARR, with ticks, for when that difference matters.

`Game` is already copyable (`Bag`, `Board`, `DealtStack` and `Scoring` are all
values), so cloning is a copy. A test pins that down: copy mid-run, play the
same actions on both, get the same game.

### Engine changes

1. **`Placements`** (`src/placements.h/.cpp`): the reachable-placement
   search above, plus tests (with no hold, an empty board yields 34
   distinct resting places for a T, 17 for an S or an I, 9 for an O; a
   T-slot yields its T-spin; nothing found
   conflicts with `Board::fits`). It's engine code ("rules, generation, AI"),
   and a future in-game hint or bot could use it too.
2. **Copy test** for `Game`.
3. **Board metrics** (`holes`, column heights, bumpiness) in a small
   `BoardMetrics` helper for the signals, tested.

None of these change gameplay.

## Watching an agent play

### Replays (`replay/v1`)

Because the engine is deterministic, an episode is its seed plus the engine
calls it made, recorded below the action space. A replay therefore plays
back the same whether the agent used `placement`, `drop` or `raw`:

```json
{
  "game": "omatris", "rules_version": 3, "mode": "marathon", "seed": 1234,
  "agent": "cem-gen42",
  "calls": "t60 L L CW t1 HD t9 …"
}
```

`calls` is a compact token string: `tN` advances N ticks, and the rest are
engine inputs (`L R CW CCW SD+ SD- HD H`). A test records a run and plays it
back to the same score. The env writes one at every episode end on request.

### `--replay` in the app

`bin/run omatris --replay run.json` opens the normal game with no player
input. It shows the agent's name in the header and has its own keys, shown
in the UI as usual: `P` pause, `1`–`4` speed (1×, 2×, 4×, as fast as it
draws), `→` step one piece while paused, `R` restart the replay, `Esc` leave.
The bridge feeds calls from the replay into `Game` on its normal pacer, and
the speed is the pacer interval. Results are never written to the
high-score table.

This gives you the "generation 1 vs generation 500" style of video: the
trainer saves the best evaluation replay of each checkpoint, and you watch
them in the real app.

### Live spectating (later)

`--agent unix:/path` makes the app connect to a socket and ask an external
agent for each action, sending it the same observation bytes the ABI writes.
That lets you watch a model play live, at real speed. It's only worth
building once replays prove too slow to iterate with.

## Other games

Every env follows the same pattern. Each game keeps its spec in its README.

| Game | Natural action space | Observation (visible only) | Baseline to beat |
|---|---|---|---|
| **oma2048** | 4 slides, masked | 4 × 4 log₂ tiles; afterstates are the slide before the random spawn | expectimax; n-tuple TD learning is the known strong method |
| **omasnake** | 3 relative turns per *move* (ticks between moves skipped) | grid channels: body, head, food, bonus + timer | greedy BFS to food |
| **omasweeper** | reveal / flag × cell, masked to hidden cells | revealed numbers, flags, hidden mask; never mines | the game's own solver (boards are no-guess) |
| **omanix** | 5 per step (stay, 4 directions) with frame skip | grid of claimed/trail/balls + chaser positions | real-time and hard; the long-term "Trackmania" target |
| **blackomack** | hit / stand / double / split / insure; bet separately | hand, up-card, true count if wanted | `BasicStrategy` (already in the engine) |
| **omadoku** | poor RL fit (a constraint solver wins); skip | | |

## Plan

1. `common`: `Env`, spec builder, C ABI, `replay/v1`, `env.pri`,
   `bin/build-env`, CI job. Tests for the shim.
2. `omatris`: `Game` copy test, `BoardMetrics`, `Placements`.
3. `omatris`: `OmatrisEnv` with the three action spaces, plus env tests
   (determinism, masks match `Placements`, replay round trip, no
   `QSettings` writes).
4. `omatris`: `--replay` in the app and its README section.
5. A second game (2048 or Snake) to prove the interface is generic before it
   hardens. Bump `OG_ABI_VERSION` freely until then.

Each step is its own PR with atomic commits, per `CLAUDE.md`.
