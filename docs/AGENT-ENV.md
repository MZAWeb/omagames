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

Built in step 1 (`common/env/`):

```
common/env/
  omagames_env.h          the C ABI: the only header a trainer depends on
  env.h                   OmaGames::Env, the C++ interface a game implements,
                          and the two functions each env library defines
  envabi.cpp              the C shim: checks every call, then forwards it to the Env
  observationlayout.h/.cpp named, typed, aligned tensors in one flat buffer
  envconfig.h/.cpp        resolves a config against the game's schema
  replay.h/.cpp           replay/v1: seed + engine inputs, written and read
  env.pri                 included by a game's env.pro and by test suites
common/tests/walkenv.*    a toy game the common suite drives the ABI through
bin/build-env             bin/build-env omatris → build-env/omatris/libomatris_env.so
```

Each game adds:

```
games/<game>/env/
  <game>env.h/.cpp        the game's Env: observation, actions, signals
  env.pro                 include(../../../common/env/env.pri), TEMPLATE = lib,
                          CONFIG += plugin (a plain lib<game>_env.so), QT = core;
                          lists the engine sources the same way tests/tests.pro does
```

The env's tests live in the game's existing suite (`bin/test omatris`) and
drive its `Env` directly. The ABI itself is tested once, in `common/tests`,
over the toy `WalkEnv`. CI builds `env/` for every game that has one and
includes it in the `-Werror` gate. Env libraries are not shipped in the
PKGBUILDs.

Adding an env to another game means writing one class, about 200 lines,
plus its tests. The ABI, batching, replays and the Python side are shared.

### `OmaGames::Env` (C++)

`common/env/env.h` is the reference. In short:

```cpp
class Env {
public:
    virtual void configure(const QJsonObject &config) = 0;   // already resolved and valid
    virtual const ObservationLayout &observationLayout() const = 0;
    virtual int actionCount() const = 0;
    virtual QStringList actionLabels() const;                // optional
    virtual QStringList signalNames() const = 0;             // at most OG_MAX_SIGNALS
    virtual void reset(quint32 seed) = 0;
    virtual EnvStep step(int action) = 0;                    // only legal actions arrive
    virtual void observe(std::byte *buffer) const = 0;       // the buffer arrives zeroed
    virtual void actionMask(quint8 *mask) const = 0;
    virtual std::unique_ptr<Env> clone(bool reseedHidden, quint32 seed) const = 0;
    virtual Replay replay() const = 0;
    virtual QJsonObject info() const;                        // optional
};

// Defined once in each env library.
QJsonObject envGameSpec();          // {game, rules_version, config: schema}
std::unique_ptr<Env> createEnv();
```

A config schema maps each key to `{default, choices}` (a string),
`{default, min, max}` (an integer) or `{default}` (a bool). The type of the
default is the type of the key. Unknown keys are refused, so a typo in a
trainer config fails instead of quietly training on the default.

### The C ABI (`omagames_env.h`)

`common/env/omagames_env.h` is the reference. The functions:

| Function | What it does |
|---|---|
| `og_abi_version()` | `OG_ABI_VERSION` (1) |
| `og_game_spec()` | `{abi, game, rules_version, config: schema}`; needs no env |
| `og_create(config_json)` | a new env, or NULL and `og_last_error()` |
| `og_env_spec(env)` | `{config, observation: {size, tensors: [{name, dtype, shape, offset}]}, actions: {count, labels}, signals}` |
| `og_reset(env, seed)` | starts an episode |
| `og_step(env, action, &result)` | 0, or -1 when the action is out of range or masked, or the episode has not started or is over |
| `og_observe(env, buffer)` | writes `size` bytes |
| `og_action_mask(env, mask)` | writes `count` bytes; all zero outside an episode |
| `og_clone(env, reseed_hidden, seed)` | a deep copy, optionally with the hidden randomness redrawn |
| `og_replay_json(env)`, `og_info_json(env)` | the episode as replay/v1; free-form debug state |
| `og_step_batch(envs, n, actions, seeds, results, obs, final_obs, masks)` | steps n envs in one call; resets ended ones in place from `seeds` |
| `og_destroy(env)`, `og_last_error()` | |

`OgStepResult` is `{reward, terminated, truncated, ticks,
signal_values[16]}`. The field isn't called `signals` because that's a Qt
keyword.

What the shim does, so no game has to:

- **Config:** resolves it against the schema, and adds `max_steps` (0 means
  no cap) to every game's schema. Truncation is counted in the shim.
- **Stepping out of turn is an error, not undefined:** before a reset, after
  the episode ends, out of range, or masked. A masked action is refused
  rather than ignored, which catches agent bugs on the first step instead of
  after a day of training.
- **Batches are all or nothing:** every action is checked before any env
  moves, and envs with a different config from env 0 are refused.
- **Observations are deterministic:** the buffer is zeroed before
  `observe()`, so alignment padding is always zero.

The observation is one contiguous buffer. Every tensor offset, and the total
size, is a multiple of 8, so the rows of a batch stay aligned for any dtype.
Dtype names are numpy's (`uint8`, `int32`, `float32`), so Python lays
zero-copy views over a buffer it owns. Envs share no global state, so
different envs can be stepped on different threads.

`rules_version` in the spec is bumped whenever a rule constant or behaviour
changes. That covers, for example, `kMaxLockResets` and anything else that
changes what a sequence of actions does. Replays and trained models record
it, so a stale checkpoint is refused instead of quietly scoring worse.

Measured on the toy env from Python through ctypes: about 200,000 calls a
second on one thread, before batching.

### Rewards are signals, not opinions

The env reports what happened, and the trainer decides what that's worth.
`reward` is the game's own score delta, so a trainer works out of the box.
Each env also declares named signals (for Omatris: score gained, lines
cleared, pieces placed, T-spin, holes and max height after the step, game
over). Reward shaping depends on the experiment, so it lives in the trainer.
Signals that are expensive to recompute in Python, like holes, are computed
here once.

## Omatris: the first env

Built in steps 2 and 3. `games/omatris/README.md` ("Agent environment") is
the spec: config keys, tensors, action spaces and signals. This section
covers why it's shaped the way it is.

### Engine pieces it stands on

| Piece | What it is |
|---|---|
| `Calls` (`src/calls.h`) | The engine inputs a player's keys come down to (`L R CW CCW SD+ SD- HD H`) plus a tick, applied through the same `Game` methods the bridge calls. The env makes every call through it, so its replay is exactly the game. |
| `Placements` (`src/placements.h`) | Every distinct place the piece can come to rest, with the shortest calls to get there and what locking it would do: lines, top out, and the board left behind (the afterstate). |
| `BoardMetrics` (`src/boardmetrics.h`) | Column heights, holes, bumpiness: for the signals, never for the rules. |
| `Game::reseedHidden`, `Bag::reseedHidden` | Shuffle again everything the next queue doesn't show, within each 7-bag, so the bag guarantee still holds. |

The search runs breadth-first over **copies of the real `Game`**. Its moves
are a shift, a turn either way, and a *sonic drop*: one tick of an instant
soft drop, which is what a player with Instant soft drop does by tapping ↓.
Wall kicks, the lock delay and its two resets, and T-spin detection are
therefore the engine's own, with no second implementation to drift. No
other time passes between moves, which models a player of infinite speed.
That's exact at low gravity and generous near level 20; if it ever matters,
the fix belongs in the search (tick between moves), not in a separate rule
set. Because the search relies on sonic drops, the env always plays with
an instant soft drop, and its replays record that
(`config.soft_drop_factor = 0`).

Landings are distinct by the cells they fill and the spin they'd score,
so an S turned twice is one landing, not two. On an empty board that
gives 34 for a T, J or L, 17 for an S, Z or I, and 9 for an O.

### Speed

The search copies the game about a thousand times per piece, so the copy
must be cheap. Two changes made it so, neither altering a game (the same
seeds play the same games, which the existing suite confirms):

- `Bag` keeps its queue in a fixed ring instead of a `std::deque`, so a
  copy allocates nothing.
- `Board::inside` and `Board::blocked` are inline. Collision checks were
  most of the search's time.

Measured on one core of the dev container, a greedy player with hold
went from 1,150 to about 4,000 pieces a second (0.25 ms per piece, search
included). From Python, through ctypes and numpy, a four-feature greedy
heuristic picking among the afterstates cleared 799 lines in 2,000 pieces
at about 1,400 pieces a second. Each piece adds 4 cells and a line takes
10, so 800 lines is the most 2,000 pieces can clear.

### Hidden information

The observation shows the board (hidden rows included: they're part of
the well, just off screen), the falling piece, the next three and the hold
box. The bag behind the queue isn't shown. A clone with `reseed_hidden`
reshuffles it, so a planner's rollouts are samples of the future, not the
future itself. A reseeded clone's replay is empty, because its seed no
longer says what comes next.

## Watching an agent play

### Replays (`replay/v1`)

Because the engine is deterministic, an episode is its seed plus the engine
calls it made, recorded below the action space. A replay therefore plays
back the same whether the agent used `placement`, `drop` or `raw`:

```json
{
  "format": "replay/v1",
  "game": "omatris", "rules_version": 3, "seed": 1234,
  "config": {"mode": "marathon", "actions": "placement"},
  "agent": "cem-gen42",
  "calls": "t60 L L CW t1 HD t9 …"
}
```

`calls` is a compact token string: `tN` advances N ticks, and the rest are
engine inputs (`L R CW CCW SD+ SD- HD H`). A test records a run and plays it
back to the same score. The env writes one at every episode end on request.

### `--replay` in the app

Built in step 4; `games/omatris/README.md` ("Watching a replay") is the
spec, and `games/omatris/replays/greedy-marathon.json` a sample.

```sh
bin/run omatris --replay games/omatris/replays/greedy-marathon.json
```

It opens the normal game with no player input, the agent's name in the
header, and the replay's own keys shown in the legend as usual: `P` pause,
`1`–`4` speed (¼×, ½×, 1×, 8×), `→` next piece, `R` watch again, `Esc`
leave. Results are never written to the high-score table, and the player's
handling is set aside for the soft drop the replay was recorded with.

`ReplayPlayer` (engine, QtCore) reads the file and refuses another game,
another `rules_version`, an unknown mode or input. It plays the calls back
in *beats*, one per frame at 1×: a tick is a beat, a hard drop or hold is,
and so is an input made with no time after another. Without that rule, a
placement-mode game (inputs with no ticks between them) would show pieces
teleporting into place; with it, each move gets a frame, and a raw-mode
game, whose inputs each ride with a tick, plays at real speed. A placing
agent's piece is about 8 beats, so the default ½× shows three or four pieces
a second.

The app links only `common/env/replay.cpp`, not the env ABI.

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
| **omasnake** (built) | 3 relative turns, or 4 absolute directions, per *move* (ticks between moves folded in) | coded 24 × 32 grid (body, head, tail, food, bonus) + a labelled state row | greedy toward the food: average length 41 over 20 games, always dying by boxing itself in |
| **omasweeper** | reveal / flag × cell, masked to hidden cells | revealed numbers, flags, hidden mask; never mines | the game's own solver (boards are no-guess) |
| **omanix** | 5 per step (stay, 4 directions) with frame skip | grid of claimed/trail/balls + chaser positions | real-time and hard; the long-term "Trackmania" target |
| **blackomack** | hit / stand / double / split / insure; bet separately | hand, up-card, true count if wanted | `BasicStrategy` (already in the engine) |
| **omadoku** | poor RL fit (a constraint solver wins); skip | | |

## Plan

1. **Done.** `common`: `Env`, observation layout, config resolution, C ABI,
   `replay/v1`, `env.pri`, `bin/build-env`, CI. Tested over a toy env.
2. **Done.** `omatris`: `Game` copy test, `BoardMetrics`, `Placements`,
   `Calls`, hidden reseeding.
3. **Done.** `omatris`: `OmatrisEnv` with the three action spaces, plus env
   tests (determinism, masks match `Placements`, replay round trip, no
   `QSettings` writes).
4. **Done.** `omatris`: `--replay` in the app and its README section, plus a
   sample replay.
5. **Done.** `omasnake`: the second env, chosen because it shares the least
   with Tetris (real time, no placements or afterstates, a tiny action
   space) and because a correct learner is known to master it, so a flat
   learning curve points at a bug rather than at the game. It needed
   nothing new from the shared layer. No replay viewer yet: that waits until
   someone wants to watch a Snake agent.

Steps 1 to 5 went up as one PR of atomic commits. `OG_ABI_VERSION` can
still change freely until omagym depends on it.

Next: the omagym skeleton on Omatris (baselines, evaluation, CEM), then
one learner running on both games with no game-specific code in its loop.
Omanix is the third env, once both have shaken omagym out: lives, levels,
a varying number of balls and chasers, and a reward that only comes when a
cut closes.
