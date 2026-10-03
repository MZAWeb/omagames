# omagym

A lab for teaching programs to play omagames, and for finding out which way
of teaching works best. **It is not a game.** It is a Python project that
plays the games' real engines, headless and far faster than real time,
through the agent environment libraries described in `../docs/AGENT-ENV.md`.

New to the field? `SCIENCE.md` maps it: the families of algorithms, the
techniques they share, how to compare them fairly, which suit which game,
and what to try next with each agent here.

## The ideas, in one page

Five words carry everything else in this file.

- **An agent** is a strategy: one Python file in `src/omagym/agents/`. Some
  only play (`greedy` follows hand-written rules), some *learn* (`dqn`
  trains a neural network), some *plan* (`lookahead` tries moves on copies
  of the game before choosing).
- **A run** is one agent, on the code as it is now, being tested. If the
  agent learns, the run trains it first. Either way it then plays every
  test, and everything about it is recorded: settings, code, results,
  replays, what it learned. Change the code and you make a new run: a result
  belongs to the code that produced it.
- **A test** is a fixed set of games and what "better" means on them. Every
  run plays all of its game's tests. Omatris has two:
  - `marathon` (the main one): 20 games cut at 2,500 pieces, about level
    100. More points is better.
  - `challenge`: 20 games that start on a dealt mess of holes and
    overhangs, won when the mess is cleared. Fewer pieces is better, counted
    per point of the deal's Difficulty (the 1-100 number the app shows), so
    a hard mess is allowed more pieces than an easy one.
- **The standard**: every test game is dealt from a fixed seed that
  training never uses, so every agent plays exactly the same games (the
  same Challenge messes, the same pieces) and none has seen them before. In
  the app every game draws a new seed, which is why it feels random. Omatris pieces are placed at **ten key presses a
  second**, like a fast human: placing takes time, gravity pulls the piece
  meanwhile, and the high levels are genuinely hard.
- **compare** puts runs side by side, game by game, and says which played
  better and whether the difference is real or could be luck.

So the loop is: write or change an agent, `run` it with a name and a note,
`compare` it with what you had, `watch` it in the real app.

## Setup

You need [uv](https://docs.astral.sh/uv/) (`pacman -S uv`) and what the
games build with (Qt 6, see the main README). From this folder:

```sh
uv sync                  # once: Python packages, PyTorch included (a few GB)
uv run omagym agents     # what is there to play with
```

uv keeps everything in `omagym/.venv`; nothing is installed system-wide. The
game libraries (`build-env/<game>/lib<game>_env.so`) are built for you the
first time a command needs them, and rebuilt whenever the C++ changes, so a
run always plays the engine as your checkout has it.

PyTorch comes from PyPI, whose Linux build includes CUDA: with an NVIDIA GPU,
`--device auto` (the default) uses it. The networks here are small, so the
GPU mostly pays off for the CNN ones. `uv sync --no-group learn` leaves
PyTorch out; the agents that need it are then listed as unavailable.

## A first session

```sh
# The rule-based agent: plays every test, takes about a minute.
uv run omagym run --agent greedy --name greedy --notes "greedy, default weights"

# One that learns: it trains (100,000 steps for dqn), then plays the same tests.
uv run omagym run --agent dqn --name dqn --notes "plain dqn"

# Which played better, and is the difference real?
uv run omagym compare greedy dqn                     # on marathon, by score
uv run omagym compare greedy dqn --test challenge    # getting out of trouble
uv run omagym compare --game omatris                 # every agent's best run

# Watch it in the real app: its best game, or the same game at ten points of its training.
uv run omagym watch dqn
uv run omagym watch dqn --training
```

## The agents

In the order worth reading them. Each file explains its method where it
happens; `greedy.py` first, since every Tetris agent after it is the same
loop ("rate every landing, take the best") with a better way of rating.

| Agent | Games | Kind | What it does |
|---|---|---|---|
| `random` | any | plays | Any legal action. The floor |
| `greedy` | Omatris | plays | Rates the board each landing leaves with four hand-tuned weights (lines, holes, bumpiness, height) |
| `greedy` | Omasnake | plays | Steps toward the food, avoiding walls and its body one move ahead |
| `cem` | Omatris | learns | The cross-entropy method: evolves the weights of 15 board features. No PyTorch |
| `dqn` | Omatris | learns | A neural network that learns how good a board is (deep Q-learning on afterstates) |
| `lookahead` | Omatris | plans | Beam search over the next pieces on copies of the game, judging boards with greedy's weights or a `cem` or `dqn` run's |
| `mcts` | Omatris | plans and learns | Monte Carlo tree search with a value network that learns from the search (AlphaZero-style) |
| `ppo` | any | learns | Proximal policy optimisation: learns the policy itself from the raw observation. The general one, and the road to Trackmania |

Their settings are listed by `omagym agents` and changed with `--set`. For
`dqn` (and `mcts`), the most important one is **what the network sees**,
`--set inputs=...`:

| `inputs` | The network sees |
|---|---|
| `features` (default) | greedy's five numbers |
| `rich` | cem's fifteen: which kind of clear and spin, holes, heights, wells, transitions |
| `rich_hand` | `rich`, plus the held piece and the next piece after the landing |
| `board`, `cnn` | every cell of the board, through a plain or a convolutional network |
| `cnn_hand` | `cnn`, plus the held and next pieces |
| `hybrid` | the cells through a CNN, beside the rich features and the hand |

## Results

Every run so far, with each agent's default training unless the name says
otherwise. Marathon is 20 games cut at 2,500 pieces; Challenge is 20 dealt
messes, capped at 1,000 pieces, a loss counting as the whole cap.

| Run | Marathon score | Marathon lines (of 1,000) | Games survived (of 20) | Challenge won (of 20) | Challenge pieces per point of difficulty | Whole run |
|---|---|---|---|---|---|---|
| `dqn-rich-hand` | **10,516,885** | 992.6 | 19 | 20 | 3.7 | 9 min |
| `dqn-rich` | 9,608,866 | 924.5 | 18 | 1 | 26.3 | 9 min |
| `mcts` (from `dqn`'s network) | 9,384,719 | 996.6 | 20 | 19 | 7.5 | 27 min |
| `dqn` | 8,462,525 | 977.9 | 19 | 13 | 12.5 | 8 min |
| `greedy` | 6,390,662 | 998.5 | 20 | 20 | 1.3 | under a minute |
| `lookahead` | 6,244,006 | 999.0 | 20 | 20 | **0.8** | 7 min |
| `lookahead-cem` | 4,453,274 | 569.8 | 6 | 11 | 15.9 | 4 min |
| `cem` | 3,672,947 | 542.7 | 6 | 15 | 10.6 | 15 min |
| `dqn-hybrid` | 818 | 3.6 | 0 | 0 | 28.9 | 8 min |
| `ppo` | 435 | 0.5 | 0 | 0 | 28.9 | 10 min |
| `random` | 291 | 0.1 | 0 | 0 | 28.9 | seconds |

`runs` shows what each one is (its notes). "Whole run" is training plus
both tests, on a 20-core machine with an RTX 4090, several runs at once.

What it shows:

- **The two tests reward different players.** The learners score most on
  Marathon by stacking up for multi-line clears. On a dealt mess they are
  slow: `dqn-rich-hand` needs over four times the pieces `lookahead` does
  for the same deals. `lookahead` and `greedy`, which punish height and
  holes, dig the mess out fastest.
- **Learners learn what they're paid for.** `dqn`'s reward pays a point for
  every piece placed and squares the lines, so it learned to play long and
  build for Tetrises, not to dig, which it never saw in training (Challenge
  is test-only). A curriculum of Challenge boards, or a reward for each
  dealt row cleared, would test whether it can learn to.
- **What the network sees matters most.** With cem's features and the held
  and next pieces (`rich_hand`), `dqn` beats plain `dqn` on all 20 Marathon
  games and wins every Challenge; with the features alone (`rich`) it wins
  only one.
- **Search on a learned value helps.** `mcts` on plain `dqn`'s network beats
  `dqn` itself on both tests, at three times the time.
- **The CNNs haven't learned anything yet**, `hybrid` included: in 100,000
  steps a network over raw cells gets nowhere. In `hybrid` the features may
  be drowned out by the CNN's thousands of outputs.
- **`cem` trains on short games** (300 pieces, never past level 12), so it
  never meets the speed of later levels and tops out there.
- **`ppo` has barely started**: a million steps is little for a policy
  learning Tetris from the raw well.

## Commands

Every command takes `--help`. A run can be named by its id, a unique prefix
of its id, its `--name`, or `last`.

| Command | What it does |
|---|---|
| `omagym agents` | Lists agents, the games they play, their settings and defaults |
| `omagym run --agent A [--game G]` | Trains the agent if it learns, then plays every test; recorded as a run |
| `omagym runs [--game G] [--agent A] [--trained]` | Lists runs, newest first: name, how long it trained, score, notes |
| `omagym show R` | Everything about a run: settings, code, every test's results, learning curve, what it trained into |
| `omagym compare [R1 R2 ...] [--test T] [--by M] [--lower]` | Ranks runs on a test by its measure, or `--by lines`, `steps`... (`--lower` when less is better); with no runs, each agent's best |
| `omagym diff R1 R2 ...` | Runs side by side with every setting that differs: for runs of one agent |
| `omagym watch R [--test T] [--worst \| --training]` | Plays the run's best (or worst) game of a test in the app; `--training`, its snapshots, stepped through with `N` and `B` |
| `omagym note R [--name N] [--notes "..."]` | Names a run, or writes down what it was, afterwards |
| `omagym delete R1 R2 ... [--yes]` | Forgets runs: results, checkpoints and replays. Asks first |

### Options of `run`

- `--name`, `--notes`: what to call it and what it is. `runs` shows the
  notes, so a short name stays readable a week later.
- `--set KEY=VALUE`: an agent setting, repeatable (`--set inputs=rich
  --set lr=3e-4`).
- `--env KEY=VALUE`: a game setting (the game's README, "Agent
  environment", lists them). Omatris is played at `input_rate=10` unless
  this says otherwise; `--env input_rate=0` is a player of infinite speed.
- `--episodes N`, `--max-steps N`: the main test's games, how many and where
  each is cut (20 × 2,500 for Omatris, 50 × 3,000 for Omasnake).
- `--seed N`: the agent's own randomness (training games, exploration).

For an agent that learns:

- `--steps N`: how long it trains (`omagym agents` shows each default).
- `--eval-every N`, `--eval-episodes N`, `--quick-max-steps N`: the quick
  evaluations during training that draw its learning curve (every tenth of
  the run, 5 games, cut at 500 pieces; they only need the trend).
- `--snapshots N`: games recorded along the way (default 10, evenly spaced
  from step 0 to the end). Each plays the same game with exploration off,
  so `watch R --training` shows that one deal played better and better.
- Ctrl+C stops training early; what it learned so far still plays the
  tests, and the run is recorded as `interrupted`.

## Reading `compare`

```
ranked by pieces_per_difficulty, lower is better: the 20 games of omatris in the challenge test every run played, each cut at 1000 steps

    run            agent      trained        pieces_per_difficulty  vs best: won-tied-lost  difference (95% range)  verdict
--  -------------  ---------  -------------  ---------------------  ----------------------  ----------------------  -------
1.  lookahead      lookahead  -              0.8 ± 0.3
2.  greedy         greedy     -              1.3 ± 0.6              1-0-19                  +0.5 (+0.3 to +0.8)     worse
3.  dqn-rich-hand  dqn        101,318 steps  3.7 ± 2.0              0-0-20                  +2.9 (+2.1 to +3.9)     worse
```

Every run is set against the best **game by game**: on the same game, how
much better or worse did it do? That cancels out how kind each game's
pieces were, so it is far sharper than comparing two averages.

- **won-tied-lost**: on how many games it beat the best run, matched it, or
  lost to it.
- **difference**: by how much on average, with a 95% range for the true
  difference (a bootstrap over the games).
- **verdict**: `worse` when the whole range is on the wrong side of zero;
  `can't tell` when it straddles zero, so these games can't separate the
  two (more `--episodes`, or more seeds, would); `same` when it played every
  game identically.
- **trained**: the steps of training behind it, so a learner that needed a
  million steps isn't mistaken for one that needed ten thousand.

What "better" is depends on the test (`--test`) and what you rank by
(`--by`). `dqn` beats `greedy` on score but loses on lines: it builds the
stack up for multi-line clears, which score far more than single lines.

`compare` refuses runs whose games were cut at different lengths, counts
only the games every run played, and notes when env settings differ: fine
for an agent's action space, not for the game itself.

## A way of working

1. **Get a baseline**: `run` the agent as it is, with a `--name` and
   `--notes`.
2. **Change one thing**: a setting, the reward, the network, a new agent.
3. **Commit, or don't.** A run on uncommitted code records it exactly in a
   patch, but a commit gives the change a message.
4. **Run it**, with a name and a note saying what changed.
5. **Compare** it with the baseline, on both tests. Believe `worse`; treat
   `can't tell` as no difference yet. `diff` shows what changed.
6. **For learners, try more than one seed** (`--seed 1`, `2`, `3`) before
   believing a difference: training is noisy.

## How runs are kept

Everything is local, under `omagym/experiments/` (git ignores it; set
`OMAGYM_HOME` to keep it elsewhere):

```
experiments/
  experiments.sqlite         every run, queryable
  runs/<run id>/
    code.patch               uncommitted changes the run ran with, if any
    best.pt, last.pt         what an agent that learns learned
    replays/best.json        the main test's best and worst games,
    replays/worst.json         playable with `omagym watch` or `bin/run <game> --replay`
    replays/challenge/...    the same for each other test
    snapshots/01.json ...    the same game, played at ten points of training
```

For each run the database keeps what ran (game, agent, every setting with
its default, seed, device, the game's rules version, library versions), the
code (the git commit, plus a patch of anything uncommitted: `git checkout
<commit> && git apply code.patch` brings it back), and how it did (every
test game, a summary of each test, and for learners the learning curve).
It's plain SQLite, so anything the commands don't show is a query away.

## Layout

```
pyproject.toml           dependencies and the `omagym` command (uv)
SCIENCE.md               the field, and what to try next with each agent
src/omagym/
  native.py              loads lib<game>_env.so through ctypes, building it first
  env.py                 Env: reset / step / observe / mask, any game
  games/
    __init__.py            per game: its tests, and the settings every agent plays with
    omatris.py             board features of each landing, and the pieces in hand
    omasnake.py            the snake's state, safe moves
  agents/
    base.py              Agent, the interface every strategy implements
    __init__.py          the registry, and the list of agent modules
    random_agent.py      random
    greedy.py            greedy, for Tetris and for Snake: start reading here
    cem.py               cem
    afterstate_value.py  the network that rates boards, and what it sees: shared by dqn and mcts
    dqn.py               dqn
    lookahead.py         lookahead
    mcts.py              mcts
    ppo.py               ppo
  evaluation.py          playing the tests' games, and summarising them
  training.py            everything around an agent's training: budget, curve, checkpoints, snapshots
  store.py               the experiment database
  provenance.py          which code a run ran
  comparison.py          which run played better, game by game, and how sure that is
  report.py              the tables the commands print
  cli.py                 the `omagym` command, and `run`
  records.py             the commands on recorded runs
experiments/             your runs (not in git)
```

## Writing an agent

An agent is one class in one file:

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

2. Add `"my_agent"` to the module list at the bottom of `agents/__init__.py`.
3. `uv run omagym agents` lists it; `uv run omagym run --agent cautious` tests it.

What an agent sees is the game's observation, a dict of NumPy arrays named
in the game's README (`board`, `candidates`, `afterstates` for Omatris;
`grid`, `state` for Omasnake); `self.spec` has their shapes and column
labels. `mask` says which actions are legal; an illegal one is refused.

**To make it learn**, set `trainable = True` and `default_steps`, and write
three more methods. `cem.py` is the simplest example, `dqn.py` the neural
one.

- `train(self, ctx)`: a generator that learns forever. Make games with
  `ctx.make_env()`, deal them with `ctx.next_seed()`, and `yield {"steps":
  n, ...anything to plot}` every so often. The framework stops it when the
  budget is spent and does the evaluations, checkpoints and snapshots
  between yields.
- `save(path)` and `load(path)`: write and read what it learned.
- Its reward is up to you: the env reports the score gained, plus named
  signals each step (lines, holes, height, topped out, dots eaten...).

**To make it plan**, override `decide(env, obs, mask)` instead of `act()`,
and try moves on `env.clone(reseed_hidden=True)`: a copy that deals its own
pieces beyond the ones a player can see, so the search can't cheat.

**To add a test**, add a few lines to the game's entry in
`games/__init__.py`: its env settings, games, cap and measure.

## Changing a game

The engines and their envs are C++ in `../games/<game>/` (`src/` and
`env/`), built by `bin/build-env`. omagym rebuilds them before every
command, so a change there is picked up on the next run. If a change makes
the same moves play out differently, bump the game's `Rules::kVersion`:
runs record it, so results from before and after are never mistaken for
each other.
