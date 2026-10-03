# omagym

A lab for teaching programs to play omagames, and for finding out which way
of teaching works best. **It is not a game.** It is a Python project that
plays the games' real engines, headless and far faster than real time,
through the agent environment libraries described in `../docs/AGENT-ENV.md`.

You write a strategy (an *agent*), test it on a fixed set of games, train it
if it learns, and compare it with everything you tried before. Every run is
recorded with the code it ran, so a number in a table can always be traced
back to the change that produced it.

Today it plays **Omatris** and **Omasnake**, and ships three agents:

| Agent | Games | Learns | What |
|---|---|---|---|
| `random` | any | no | Any legal action. The floor. |
| `greedy` | Omatris | no | Scores the board each landing leaves with four weighted features, takes the best |
| `greedy` | Omasnake | no | Steps toward the food, avoiding walls and its body one move ahead |
| `dqn` | Omatris | yes | A small neural network that learns how good a board is (deep Q-learning on afterstates) |

On the default evaluation games (20 Tetris games cut at 500 pieces, 50
Snake games):

| | Omatris lines (of 200 possible) | Omasnake dots |
|---|---|---|
| `random` | 0.1, topping out after 24 pieces | 0.2 |
| `greedy` | 198.7 | 37.6, then it boxes itself in |
| `dqn`, 30,000 training steps (about 2 minutes on a CPU) | about 196 | |

The `cautious` example under "Adding a strategy" clears about 30 lines
before it tops out: a fair first agent to beat.

## Setup

You need [uv](https://docs.astral.sh/uv/) (`pacman -S uv`) and what the
games build with (Qt 6, see the main README). From this folder:

```sh
uv sync                  # once: Python packages, PyTorch included (a few GB)
uv run omagym agents     # what is there to play with
```

uv keeps everything in `omagym/.venv`; nothing is installed system-wide. The
game libraries (`build-env/<game>/lib<game>_env.so`) are built for you the
first time a command needs them, and brought up to date whenever the C++
changed, so a run always plays the engine as your checkout has it.

PyTorch comes from PyPI, whose Linux build includes CUDA: with an NVIDIA GPU
`--device auto` (the default) uses it. A network this small is often quicker
on the CPU (`--device cpu`); the GPU pays off with bigger networks and
batches. `uv sync --no-group learn` leaves PyTorch out; the agents that need
it are then listed as unavailable.

## A first session

```sh
# Test the dumb strategy: 20 fixed games of Tetris, each cut at 500 pieces.
uv run omagym eval --game omatris --agent greedy --name greedy-baseline

# Train the neural one for 100k steps, evaluating every 10k.
uv run omagym train --game omatris --agent dqn --steps 100000 --name dqn-first

# How did they do, side by side?
uv run omagym compare greedy-baseline dqn-first

# Watch the trained agent's best game in the real app.
uv run omagym watch dqn-first
```

## Commands

Every command takes `--help`. A *run* can be named by its id, a unique prefix
of its id, its `--name`, or `last`.

| Command | What it does |
|---|---|
| `omagym agents` | Lists agents, the games they play, and every setting with its default |
| `omagym eval --game G --agent A` | Tests an agent on the evaluation games and records it as a run |
| `omagym eval --run R` | Tests the best checkpoint of training run R (on more games, say) |
| `omagym train --game G --agent A --steps N` | Trains a learning agent, evaluating as it goes, and records it |
| `omagym runs [--game G] [--agent A] [--kind train\|eval]` | Lists runs, newest first, with their score |
| `omagym show R` | Everything about a run: settings, code, results, learning curve |
| `omagym compare R1 R2 ...` | Runs side by side, plus every setting that differs between them |
| `omagym watch R [--worst]` | Plays the run's best (or worst) evaluation game in the app |
| `omagym note R --name N --notes "..."` | Names a run, or writes down what it was about, afterwards |

Options shared by `train` and `eval`:

- `--set KEY=VALUE` changes an agent setting (repeatable): `--set holes=-1.0`,
  `--set lr=3e-4 --set hidden=128`. `omagym agents` lists them.
- `--env KEY=VALUE` changes a game setting: `--env mode=sprint`. The game's
  README ("Agent environment") lists them.
- `--episodes N` and `--max-steps N` set the evaluation games: how many, and
  where each is cut (defaults: 20 × 500 for Omatris, 50 × 3000 for Omasnake).
- `--name`, `--notes`: say what you were trying. Future you will thank you.
- `--seed N`: the agent's own randomness (training games, exploration).

`train` also takes `--eval-every N` (steps between quick evaluations, default
a tenth of the run) and `--eval-episodes N` (games in each quick one,
default 5). Ctrl+C stops a training run early; what it learned so far is
still evaluated and recorded, as `interrupted`.

## How experiments are kept

Everything is local, under `omagym/experiments/` (git ignores it; set
`OMAGYM_HOME` to keep it elsewhere):

```
experiments/
  experiments.sqlite         every run, queryable
  runs/<run id>/
    code.patch               uncommitted changes the run ran with, if any
    best.pt, last.pt         checkpoints, for agents that learn
    replays/best.json        the best and worst evaluation games,
    replays/worst.json       playable with `omagym watch` or `bin/run <game> --replay`
```

For each run, the database keeps:

- **What ran:** game, agent, every agent and env setting (defaults
  included), seed, device, the omagames rules version, Python, NumPy and
  PyTorch versions.
- **The code:** the git commit, and when the tree had uncommitted changes, a
  patch of them (new files included) in the run's folder. The run's `code`
  is the commit plus `+` and the patch's hash, so two runs on the same code
  show the same `code`. `git checkout <commit> && git apply code.patch` gets
  it back exactly.
- **How it did:** every evaluation game (seed, score, steps, every signal
  summed), a summary (mean, std, min, median, max of each), and for
  training runs the curves: training progress and each quick evaluation, by
  step.

It is plain SQLite, so anything the commands don't show is a query away:
`sqlite3 experiments/experiments.sqlite "select id, value from summary
where key = 'score_mean' order by value desc"`.

### Why the numbers are comparable

Evaluation game *i* is always dealt from the same seed (`1_000_000_000 + i`),
and training never draws a seed that high. So every agent evaluated with
the same game, episode count and step cap played exactly the same games,
and none of them ever trained on those games. `compare` warns when the runs
you put side by side were not tested alike.

### A way of working

1. **Get a baseline.** `eval` the agent as it is, with a `--name`.
2. **Change one thing**: a setting (`--set`), the reward, the network, a
   new agent.
3. **Commit, or don't.** A run on a dirty tree still records the exact code
   in its patch, but committing first gives the change a message and makes
   `code` easy to read.
4. **Run it** with `--name` and `--notes` saying what you changed and
   expect.
5. **`compare`** the new run with the baseline. Look at the spread
   (`± std`) as well as the mean. A difference smaller than the spread is
   probably noise.
6. **For learners, use more than one seed** before believing a difference:
   `--seed 1`, `--seed 2`, `--seed 3` and compare all of them. Training is
   noisy. Then test the winner's checkpoint on more games with
   `eval --run R --episodes 100`.

## Layout

```
pyproject.toml           dependencies and the `omagym` command (uv)
src/omagym/
  native.py              loads lib<game>_env.so through ctypes, building it first
  env.py                 Env: reset / step / observe / mask, any game
  games/                 what omagym knows per game: evaluation defaults,
    omatris.py             board features of each landing
    omasnake.py            the snake's state, safe moves
  agents/
    base.py              Agent, the interface every strategy implements
    __init__.py          the registry: register(), resolve(), the list of agent modules
    random_agent.py      random
    greedy.py            greedy, for Tetris and for Snake
    dqn.py               dqn: the neural example to copy from
  evaluation.py          the fixed evaluation games, and their summary
  training.py            the loop around an agent's training: budget, evaluations, checkpoints
  store.py               the experiment database
  provenance.py          which code a run ran
  report.py              the tables `runs`, `show` and `compare` print
  cli.py                 the `omagym` command
tests/                   `uv run pytest`
experiments/             your runs (not in git)
```

## Adding a strategy

An agent is one class in one file. To add one:

1. Create `src/omagym/agents/my_agent.py`:

   ```python
   from dataclasses import dataclass

   import numpy as np

   from . import register
   from .base import Agent


   @register
   class Cautious(Agent):
       name = "cautious"            # what --agent calls it
       games = ("omatris",)         # empty for any game
       description = "Takes the landing that leaves the lowest stack."

       @dataclass
       class Config:                # every field becomes a --set option
           lines: float = 1.0       # how much a cleared line is worth, in rows

       @classmethod
       def env_config(cls, game):   # the action space it plays in
           return {"actions": "placement"}

       def act(self, obs, mask, explore=False):
           landings = obs["candidates"][: int(mask.sum())]
           column = self.spec.tensors["candidates"].column
           # Rows count down from the top, so the biggest row is the lowest landing.
           value = landings[:, column("row")] + self.config.lines * landings[:, column("lines")]
           return int(np.argmax(value))
   ```

2. Add `"my_agent"` to the module list at the bottom of
   `agents/__init__.py`.

3. `uv run omagym agents` lists it, `uv run omagym eval --agent cautious`
   tests it, and `uv run pytest` already checks it only ever plays legal
   moves.

What an agent sees is the game's observation: a dict of NumPy arrays named
in the game's README (`board`, `candidates`, `afterstates` for Omatris;
`grid`, `state` for Omasnake). `self.spec` has their shapes and column labels.
`mask` says which actions are legal; an illegal one is refused, not ignored.

### Making it learn

Set `trainable = True` and implement three more methods. `dqn.py` is the
worked example, with comments on every step.

- `train(self, ctx)` is a generator that learns forever. Make your games
  with `ctx.make_env()`, deal them with `ctx.next_seed()`, and
  `yield {"steps": n, ...anything else to plot}` every so often (`dqn`
  yields at the end of each game). The framework stops pulling when the
  budget is spent, and runs the evaluations and checkpoints between yields,
  so your loop never has to.
- `save(path)` / `load(path)` write and read what it learned.
- Its reward is up to you: the env reports the game's score gained as the
  reward, and also a set of named signals per step (lines, holes, height,
  topped out, dots eaten...). `dqn._reward()` builds its own reward from those.

Things worth trying from `dqn`: `--set inputs=board` (learn from raw cells
rather than hand-made features), a bigger network (`hidden`, `layers`),
the reward weights, `gamma`. Or copy it to a new agent: a policy-gradient
learner (PPO) in the `drop` action space, a Snake DQN, a cross-entropy-method
search over the greedy weights.

## Changing a game or its env

The engines and their envs are C++ in `../games/<game>/` (`src/` and
`env/`), built by `bin/build-env`. omagym rebuilds them before every
command, so a change there is picked up on the next run. If a change makes
the same moves play out differently, bump the game's `Rules::kVersion`.
Runs record it, so results from before and after the change are never
mistaken for each other.

## Tests

```sh
uv run pytest
```

The tests use a throwaway experiment store. The ones that need PyTorch skip
without it.
