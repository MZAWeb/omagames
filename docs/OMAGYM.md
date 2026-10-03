# Omagym: training agents to play omagames

Status: **built, first version, in `omagym/`** of this repository.
`omagym/README.md` is the reference for how it works and how to use it. This
file is the original design and the roadmap, kept for the reasoning behind
them. Where the two differ, the README is right. The differences so far:

- It lives here, beside the games, rather than in a repository of its own.
- Results go to one SQLite database (plus a folder per run), not JSONL and
  TensorBoard, so `compare` can query them. Every run records its exact code:
  the commit, and a patch of anything uncommitted.
- There are no TOML config files yet: settings are `--set KEY=VALUE` on the
  command line, and every run stores them all.
- Policies and learning algorithms are both *agents*. One that learns sets
  `trainable` and implements `train()`, the generator described below.
- Built so far from the roadmap below: #0 (`random`, `greedy`) and a basic
  #2 (`dqn` on afterstates). CEM, PPO and MCTS are next.

## Goal

A small, readable lab for learning machine learning on games we own:

- start with Omatris and be able to add any omagames game;
- try one algorithm against another on the same footing, with no repeated
  boilerplate: an algorithm is a plug-in;
- measure honestly, with fixed evaluation seeds, several training seeds and
  sample efficiency as well as the final score;
- watch the results in the real app.

## Stack

- **Python 3.12**, managed with `uv`.
- **numpy** and **PyTorch** with CUDA: a GPU is available, so configs take a
  `device` that defaults to `cuda` when present. CEM on features stays on the
  CPU, where it is faster; the neural algorithms train on the GPU.
- **Gymnasium** API compatibility, *not* as a framework. Our envs implement
  `gymnasium.Env` and `VectorEnv` so Stable-Baselines3 or CleanRL scripts
  can be used as a sanity baseline, but our own loop doesn't depend on them.
- **ctypes** to load `lib<game>_env.so`, so nothing needs compiling here.
- Configs in **TOML** (stdlib `tomllib`) mapped onto dataclasses. Metrics go
  to **JSONL** plus TensorBoard. No Hydra, no W&B by default; both are easy
  to add later.

## Layout

```
omagym/
  pyproject.toml
  omagym/
    native/        load lib<game>_env.so, check abi + rules_version, spec → numpy views
    envs/          OmaEnv (single), OmaVectorEnv (og_step_batch), wrappers:
                   RewardFn, TimeLimit, RecordReplay, FrameStack
    rewards/       per-game reward functions over the env's signals, by name
    features/      hand-made features (Tetris: heights, holes, wells, bumpiness,
                   row/column transitions) over boards or afterstates
    models/        networks built from the spec: MLP, BoardCNN, AfterstateValue
    policies/      things that only act: random, heuristic (Dellacherie / El-Tetris), loaded checkpoints
    algos/         things that learn, one file each: cem, dqn, ppo, (later) mcts, ga
    core/          registry, config, Algorithm/Policy protocols, Collector,
                   ReplayBuffer, Evaluator, Checkpointer, Logger, RunContext
    cli.py         omagym train | eval | watch | compare | bench
  configs/omatris/ cem_features.toml  dqn_afterstate.toml  ppo_drop.toml  ppo_raw.toml
  runs/            (gitignored) <game>/<algo>/<date>-s<seed>/
  tests/
```

## The plug-in contract

An algorithm is one file with one class and its config dataclass,
registered by name. Everything that isn't the learning rule is supplied by
the framework.

```python
class Policy(Protocol):
    def act(self, obs: Batch, mask: np.ndarray, *, explore: bool) -> np.ndarray: ...

@register_algo("cem")
class CrossEntropy(Algorithm):
    @dataclass
    class Config:
        population: int = 100
        elite_frac: float = 0.1
        noise: float = 4.0
        features: str = "dellacherie"

    def __init__(self, cfg: Config, spec: EnvSpec, ctx: RunContext): ...
    def policy(self) -> Policy: ...                 # current best guess, used for eval and watch
    def train(self) -> Iterator[Metrics]: ...       # the learning loop; yields every so often
    def state_dict(self) -> dict: ...
    def load_state_dict(self, state: dict) -> None: ...
```

**Why `train()` is a generator rather than a `step()` the framework calls.**
The interesting algorithms don't share a loop. CEM evaluates a population of
parameter vectors, PPO alternates rollouts and epochs, DQN interleaves acting
and replay updates, and MCTS self-play generates games and then fits a
network. Forcing all of them into one `step()` makes each one contort itself.
With a generator, the algorithm keeps its own loop and simply yields metrics.
At every yield the framework regains control and does the shared work:

- counts the budget (env steps, episodes, wall time) and stops the run;
- evaluates `policy()` on the **fixed evaluation seeds**, saving the best
  episode's replay;
- logs, checkpoints and resumes.

**What `RunContext` gives an algorithm**, so it never builds these itself:
`make_vec_env(n)` (seeded, with the configured action space and reward), a
`Collector` for rollouts, a `ReplayBuffer`, an `rng`, the device, a `log()`
call, and the `budget`.

Policies are separate from algorithms. A heuristic baseline is a policy with
no training, and `eval` and `watch` only ever need a policy. That makes "how
does PPO compare to the hand-tuned heuristic?" one command.

## Envs

`OmaEnv(game, **config)` reads `og_spec()` and builds everything from it:
the observation space (a `Dict` of the declared tensors), the action space
and the signal names. No per-game Python class is needed to *run* a game.
Per-game Python only exists where it adds knowledge: reward functions,
features, maybe a model.

Reward functions are named and composable:

```toml
[env]
game = "omatris"
mode = "marathon"
actions = "placement"
max_steps = 10000

[reward]
fn = "weighted"
weights = { lines = 1.0, game_over = -10.0, holes_delta = -0.2 }
```

Speed comes from `OmaVectorEnv`, which steps N envs per foreign call with
`og_step_batch` into preallocated numpy buffers. Several of those shards run
in threads, since ctypes drops the GIL. `omagym bench omatris` prints
steps/s for each action space, so performance regressions on either side
show up.

## Algorithms, in the order worth building

Each step teaches one new idea and has a known-good result to aim for.

| # | What | Action space | Why | Expect |
|---|---|---|---|---|
| 0 | `random`, `heuristic` policies | placement | proves the whole pipeline with zero learning; the heuristic is the bar | random: a few lines; Dellacherie: very many lines |
| 1 | **CEM** on ~8 features | placement | the classic Tetris result (Szita & Lőrincz, noisy cross-entropy); tiny, fast, teaches evolutionary search | approaches or beats the hand-tuned heuristic |
| 2 | **DQN on afterstates** (value of the board after each candidate) | placement | first neural value function; the afterstate trick removes most of Tetris's difficulty | thousands of lines with a small MLP or CNN |
| 3 | **PPO** with a fixed head | drop | policy gradients, advantage estimation, entropy | weaker than #2, and you'll see why |
| 4 | **PPO / DQN on raw inputs** | raw | the Trackmania setting: long horizons, credit assignment, frame skip | hard; a good place to try reward shaping |
| 5 | **MCTS** with honest clones (`reseed_hidden`) | placement | planning on top of a learned value | stronger per move, slower |
| 6 | Second game (2048 or Snake) using the same algos | | proves the plug-in boundary | |

## Comparing fairly

- **Evaluation seeds are fixed per game** (for example 100 seeds in
  `configs/<game>/eval_seeds.txt`) and never used for training. Every
  algorithm is evaluated on exactly the same games.
- **Train with at least 3 seeds** per configuration, and report the mean and
  a confidence interval. RL variance between seeds is often larger than the
  difference between algorithms.
- **Plot against env steps and against wall time.** CEM may lose on steps
  and win on wall time, and both views are true.
- Every run directory is self-contained: `config.toml` (resolved, including
  defaults), the git commit of both repos, `abi_version`, `rules_version`,
  `metrics.jsonl`, `checkpoints/`, `replays/`.
- `omagym compare runs/omatris/*` prints a table (final eval score, lines,
  steps to reach X, wall time) and writes the learning-curve plots.

## Watching

`omagym watch runs/omatris/cem/…/replays/best-0420.json` launches
`omatris --replay` from your omagames checkout. With `--every-checkpoint`, it
plays the best replay of each checkpoint in order, from clueless to
competent: the video series, on your desktop.

## CLI

```sh
omagym train   --config configs/omatris/cem_features.toml --seed 1
omagym eval    runs/omatris/cem/2026-10-03-s1 --episodes 100
omagym eval    --policy heuristic:dellacherie --game omatris
omagym watch   runs/omatris/cem/2026-10-03-s1 --best
omagym compare runs/omatris/*
omagym bench   omatris
```

The location of the env libraries comes from `OMAGAMES_ROOT`, which points
at an omagames checkout where `bin/build-env <game>` has been run.

## Open questions

- Should the replay viewer also show *why* (value per candidate, policy
  probabilities) as an overlay? That's a bigger app change and would need
  the live socket mode.
