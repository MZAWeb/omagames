# omagym

A lab for teaching programs to play omagames, and for finding out which way
of teaching works best. **It is not a game.** It is a Python project that
plays the games' real engines, headless and far faster than real time,
through the agent environment libraries described in `../docs/AGENT-ENV.md`.

You write a strategy (an *agent*), test it on a fixed set of games, train it
if it learns, and compare it with everything you tried before. Every run is
recorded with the code it ran, so a number in a table can always be traced
back to the change that produced it.

Today it plays **Omatris** and **Omasnake**. The agents, in the order worth
reading them (each file explains its method where it happens):

| Agent | Games | Learns | What |
|---|---|---|---|
| `random` | any | no | Any legal action. The floor |
| `greedy` | Omatris | no | Rates the board each landing leaves with four weighted features and takes the best. **Read this first**: every Tetris agent below is this loop with a better rating |
| `greedy` | Omasnake | no | Steps toward the food, avoiding walls and its body one move ahead |
| `cem` | Omatris | yes | The cross-entropy method: evolves the weights of 15 board features. No PyTorch |
| `dqn` | Omatris | yes | A neural network that learns how good a board is (deep Q-learning on afterstates) |
| `lookahead` | Omatris | no | Beam search over the next pieces on copies of the game, judging boards with greedy's weights, a `cem` run's or a `dqn` run's network |
| `mcts` | Omatris | yes | Monte Carlo tree search with a value network that learns from the search (AlphaZero-style) |
| `ppo` | any | yes | Proximal policy optimisation: learns the policy itself from the raw observation. The general one, and the road to Trackmania |

Each with its default training, on the standard games: 20 games of Tetris
cut at 2,500 pieces (1,000 lines if every one is cleared, about level 100),
placed at ten key presses a second:

| Agent | Trained | Whole run | Score | Lines | Games survived (of 20) |
|---|---|---|---|---|---|
| `dqn`, `inputs=rich_hand` | 100,000 steps | 9 min | **10,516,885** | 992.6 | 19 |
| `dqn`, `inputs=rich` | 100,000 steps | 9 min | 9,608,866 | 924.5 | 18 |
| `mcts`, from `dqn`'s network | 20,000 steps | 24 min | 9,384,719 | 996.6 | 20 |
| `dqn` | 100,000 steps | 5 min | 8,462,525 | 977.9 | 19 |
| `greedy` | none | under a minute | 6,390,662 | 998.5 | 20 |
| `lookahead` (greedy's judgement) | none | 6 min | 6,244,006 | 999.0 | 20 |
| `lookahead`, judged by `cem`'s weights | none | 3 min | 4,453,274 | 569.8 | 6 |
| `cem` | 520,000 steps | 13 min | 3,672,947 | 542.7 | 6 |
| `dqn`, `inputs=cnn` or `cnn_hand` | 100,000 steps | 7 min | about 480 | 1 | 0 |
| `ppo` | 1,000,000 steps | 7 min | 435 | 0.5 | 0 |
| `random` | none | seconds | 291 | 0.1 | 0 |

"Whole run" is training plus the 20 games, on a 20-core machine with an
RTX 4090, several runs at once. The planners spend most of theirs playing:
`mcts` searches every move, 50,000 of them in the final games alone.

What it shows:

- **What the network sees matters most.** The same `dqn` with cem's 15
  features and the held and next pieces (`rich_hand`) beats plain `dqn` on
  all 20 games, and `mcts` too. Whether the hand itself helps over `rich`
  alone, these 20 games can't tell: `rich` topped out twice, which is where
  the difference lies. The CNNs learned nothing in 100,000 steps: from raw
  cells, a network needs far longer.
- **Search plus a learned value helps.** `mcts` planning with plain `dqn`'s
  network beats `dqn` alone on every one of the 20 games.
- **Score and survival are different goals.** `greedy` and `lookahead`
  clear nearly every line but score a third less: they clear lines one at a
  time and keep the stack flat. The learners build up for multi-line clears.
- **Train on the game you're tested on.** `cem` trains on 300-piece games
  (`episode_steps`), which never get past about level 12, so it never meets
  the gravity of the later levels: its weights stack high, and at 20G a tall
  stack is a death trap. Try `--set episode_steps=2500`, which is slower.
- **PPO has barely started.** A million steps is little for a policy that
  must learn what each of 80 drops does from the raw well. That is the
  lesson of its docstring, and the place to try longer runs, `--env
  actions=raw`, or reward shaping.


New to the field? `SCIENCE.md` maps it: the families of algorithms, the
techniques they share, how to compare them fairly, and which suit which game.

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
# The dumb strategy on the 20 fixed games of Tetris, each cut at 2,500 pieces (about level 100).
uv run omagym run --agent greedy --name greedy

# One that learns: it trains (100k steps for dqn), then plays the same games.
uv run omagym run --agent dqn --name dqn

# Which played better, and is the difference real?
uv run omagym compare greedy dqn
uv run omagym compare --game omatris        # every agent's best run, ranked

# Watch the trained agent's best game in the real app.
uv run omagym watch dqn

# Or watch it learn: the same game at ten points of its training, N for the next.
uv run omagym watch dqn --training
```

## Commands

Every command takes `--help`. A *run* can be named by its id, a unique prefix
of its id, its `--name`, or `last`.

A run is one agent, on the code as it is now, playing the evaluation games.
If the agent learns, the run trains it first, and keeps what it learned
(checkpoints, snapshots) in its folder. Change the code and you run again:
a result belongs to the code that produced it.

| Command | What it does |
|---|---|
| `omagym agents` | Lists agents, the games they play, and every setting with its default |
| `omagym run --game G --agent A [--steps N]` | Trains the agent if it learns (for N steps, or its default), then plays the evaluation games; recorded as a run |
| `omagym runs [--game G] [--agent A] [--trained]` | Lists runs, newest first, with how long they trained and their score |
| `omagym show R` | Everything about a run: settings, code, results, learning curve, what it trained into |
| `omagym compare [R1 R2 ...] [--by M] [--lower]` | Ranks runs by score (or `--by lines`, `ate`, `steps`...; `--lower` when less is better, as for `holes`) and says which beat which beyond doubt. With no runs, ranks each agent's best run of `--game` |
| `omagym diff R1 R2 ...` | Runs side by side, plus every setting that differs between them: for runs of one agent |
| `omagym watch R [--worst \| --training]` | Plays the run's best (or worst) evaluation game in the app; `--training`, its snapshots in order, stepped through with `N` and `B` |
| `omagym note R --name N --notes "..."` | Names a run, or writes down what it was about, afterwards |
| `omagym delete R1 R2 ... [--yes]` | Forgets runs: their results, checkpoints and replays. Asks first |

Options of `run`:

- `--set KEY=VALUE` changes an agent setting (repeatable): `--set holes=-1.0`,
  `--set lr=3e-4 --set hidden=128`. `omagym agents` lists them.
- `--env KEY=VALUE` changes a game setting: `--env mode=sprint`. The game's
  README ("Agent environment") lists them. Omatris is played at
  `input_rate=10` unless this says otherwise: ten key presses a second, a
  fast human, so gravity pulls a piece while it is placed and high levels
  are hard. `--env input_rate=0` is the infinitely fast player.
- `--episodes N` and `--max-steps N` set the evaluation games: how many, and
  where each is cut (defaults: 20 × 2,500 for Omatris, 50 × 3000 for Omasnake).
- `--name`, `--notes`: say what you were trying. Future you will thank you.
- `--seed N`: the agent's own randomness (training games, exploration).

For an agent that learns, `--steps N` is how long it trains (`omagym agents`
shows each one's default), `--eval-every N` the steps between quick
evaluations (default a tenth of the run, which draw the learning curve) and
`--eval-episodes N` the games in each quick one (default 5), cut at
`--quick-max-steps N` (default 500: the curve only needs the trend, so
they are shorter than the final evaluation's games). Ctrl+C stops
training early; what it learned so far still plays the evaluation games,
and the run is recorded as `interrupted`.

Training also records `--snapshots N` games (default 10, cut at
`--quick-max-steps` too) evenly spaced from step 0, before it learned
anything, to the end. Every snapshot
plays the same game, the first evaluation game with exploration off, so
`watch R --training` shows that one deal played better and better. The
header says how far into training each was taken.

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
    snapshots/01.json ...    a training run playing the same game as it learned
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
and none of them ever trained on those games. `compare` refuses runs whose
games were cut at different lengths, and only counts the games all of them
played.

### Reading `compare`

```
ranked by score, higher is better: the 20 games of omatris every run played, each cut at 2500 steps

    run     agent   trained        score                  vs best: won-tied-lost  difference (95% range)                 verdict
--  ------  ------  -------------  ---------------------  ----------------------  -------------------------------------  -------
1.  mcts    mcts    20,215 steps   9,384,719 ± 154,998
2.  dqn     dqn     101,239 steps  8,462,525 ± 1,033,209  0-0-20                  -922,194 (-1,425,336 to -613,467)      worse
3.  greedy  greedy  -              6,390,662 ± 59,334     0-0-20                  -2,994,057 (-3,072,794 to -2,915,761)  worse
4.  cem     cem     520,984 steps  3,672,947 ± 3,695,487  1-0-19                  -5,711,772 (-7,230,898 to -4,069,959)  worse
```

Every run is set against the best, **game by game**: on the same game,
how much more or less did it score? That cancels out how kind each game's
pieces were, so it is much sharper than comparing two averages.

- **won-tied-lost**: on how many of the games it did better than the best
  run, the same, or worse.
- **difference**: by how much, on average, with a 95% range for the true
  difference (a bootstrap over the games).
- **verdict**: `worse` when the whole range is below zero; `can't tell` when
  it straddles zero, so these games can't separate the two (try more
  `--episodes`); `same` when it played every game identically.
- **trained**: steps of training behind it, so a learner that needed a
  million steps is not mistaken for one that needed ten thousand.

The ranking depends on what you rank by. `dqn` beats `greedy` on `score`
but loses on `lines` (`--by lines`): its stack stands taller (`--by
max_height --lower`), building for multi-line clears, which score far more
than the same lines cleared one at a time.

### A way of working

1. **Get a baseline.** `run` the agent as it is, with a `--name`.
2. **Change one thing**: a setting (`--set`), the reward, the network, a
   new agent.
3. **Commit, or don't.** A run on a dirty tree still records the exact code
   in its patch, but committing first gives the change a message and makes
   `code` easy to read.
4. **Run it** with `--name` and `--notes` saying what you changed and
   expect.
5. **`compare`** the new run with the baseline. Believe a `worse` or a
   lead over a `worse`; treat `can't tell` as no difference yet. `diff`
   shows what changed between them.
6. **For learners, use more than one seed** before believing a difference:
   `--seed 1`, `--seed 2`, `--seed 3` and compare all of them. Training is
   noisy.

## Layout

```
pyproject.toml           dependencies and the `omagym` command (uv)
SCIENCE.md               the field: which algorithms exist, and what to try where
src/omagym/
  native.py              loads lib<game>_env.so through ctypes, building it first
  env.py                 Env: reset / step / observe / mask, any game
  games/                 what omagym knows per game: evaluation defaults,
    omatris.py             board features of each landing: greedy's five, and the fifteen richer ones
    omasnake.py            the snake's state, safe moves
  agents/
    base.py              Agent, the interface every strategy implements
    __init__.py          the registry: register(), resolve(), the list of agent modules
    random_agent.py      random
    greedy.py            greedy, for Tetris and for Snake: start reading here
    cem.py               cem: the cross-entropy method
    afterstate_value.py  the network that rates boards, shared by dqn and mcts
    dqn.py               dqn: deep Q-learning on afterstates
    lookahead.py         lookahead: beam search on copies of the game
    mcts.py              mcts: tree search with a value network that learns from it
    ppo.py               ppo: a policy-gradient learner for any game
  evaluation.py          the fixed evaluation games, and their summary
  training.py            the loop around an agent's training: budget, evaluations, checkpoints
  store.py               the experiment database
  provenance.py          which code a run ran
  comparison.py          which run played better, game by game, and how sure that is
  report.py              the tables `runs`, `show`, `diff` and `compare` print
  cli.py                 the `omagym` command: its options, and `run`
  records.py             the commands on recorded runs: runs, show, compare, watch, delete...
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

3. `uv run omagym agents` lists it, and `uv run omagym run --agent cautious`
   plays it on the evaluation games.

What an agent sees is the game's observation: a dict of NumPy arrays named
in the game's README (`board`, `candidates`, `afterstates` for Omatris;
`grid`, `state` for Omasnake). `self.spec` has their shapes and column labels.
`mask` says which actions are legal; an illegal one is refused, not ignored.

### Making it learn

Set `trainable = True` (and `default_steps`, how long `run` trains it) and
implement three more methods. `cem.py` is the simplest worked example,
`dqn.py` the neural one.

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

An agent that plans (looks ahead on copies of the game, like `lookahead` and
`mcts`) overrides `decide(env, obs, mask)` instead of `act()`, and copies the
game with `env.clone(reseed_hidden=True)`.

Things worth trying:

- `dqn`: what the network sees (`--set inputs=rich`, `board`, `cnn`, and
  `rich_hand` or `cnn_hand`, which add the held and next pieces),
  `gamma`, a bigger network, and the improvements its docstring lists
  (n-step returns, Double DQN, the game's score as the reward).
- `cem`: `--set objective=lines` against `score`; more `games` per
  candidate; no noise (`noise=0`) to see the search stall.
- `lookahead`: `depth=3`, a wider `beam`, and judging with your best `cem`
  or `dqn` run (`--set model=<run>`).
- `mcts`: more `simulations`; starting from a `dqn` run (`model=<run>`)
  against from nothing.
- `ppo`: raw key presses (`--env actions=raw --env frame_skip=4`), and
  Snake (`--game omasnake`), where its reward is so sparse a random snake
  almost never finds it: a place to try reward shaping.

## Changing a game or its env

The engines and their envs are C++ in `../games/<game>/` (`src/` and
`env/`), built by `bin/build-env`. omagym rebuilds them before every
command, so a change there is picked up on the next run. If a change makes
the same moves play out differently, bump the game's `Rules::kVersion`.
Runs record it, so results from before and after the change are never
mistaken for each other.
