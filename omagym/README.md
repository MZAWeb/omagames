# omagym

A lab for teaching programs to play omagames, and for finding out which way
of teaching works best. **It is not a game.** It is a Python project that
plays the games' real engines, headless and far faster than real time,
through the agent environment libraries described in `../docs/AGENT-ENV.md`.

This file has three parts: **[Intro](#intro)** (what it is and how to use
it), **[Science](#science)** (the field: the families of algorithms, the
techniques they share, how to compare them fairly, which suit which game)
and **[To do](#to-do)** (the experiments worth running next).

# Intro

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

## Browsing it all: `omagym web`

```sh
uv run omagym web          # opens http://127.0.0.1:8765/ in your browser
```

A read-only page over everything recorded, nicer to explore than the
terminal:

- **Rankings**: every run on every test, best first, and a map of the
  Marathon score against Challenge, one dot per run.
- **Runs**: the whole list, sortable, filtered by agent, by whether it
  trained on a mix, or by text in its name and notes.
- **A run**: its results, what it was paid for (the reward settings, first),
  every other setting, the commit and any uncommitted changes it ran with,
  its learning curve, every test game, and a player for its replays and
  snapshots.
- **Compare**: tick any number of runs. They are ranked game by game with
  the same verdicts as `omagym compare`, a heatmap shows who did best on
  each game, the settings that differ are listed, their learning curves are
  overlaid, and clicking a game plays it for up to four runs side by side,
  piece by piece. (Every test game's replay is kept from now on; older runs
  only kept their best and worst.)

The player draws frames the game's own engine computes (`og_replay_frames`),
so what it shows is exactly what the app would. Nothing on the page writes:
naming, noting and deleting stay on the command line.

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

`dqn`'s reward is a sum of terms, each a setting to tweak, and what it is
paid for is what it learns to do: `reward_piece` (per piece, so playing
long pays), `reward_line` (times lines squared), `reward_top_out`,
`reward_win` (clearing a Challenge), `reward_dealt_row` (per dealt row
cleared: digging), `reward_tspin`, and two potential-based shapings that
can't change the best play, `shaping_holes` and `shaping_height`. Mixing in
Challenge games without `reward_win` or `reward_dealt_row` teaches it to
avoid finishing them, since every extra piece pays: try both.

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
| `omagym tag R1 R2 ... --add T [--remove T]` | Labels runs ("great", "baseline"); `runs --tag T` and the web lists filter by them |
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
- `--seeds N`: the same run N times, seeds 0 to N-1, in parallel, named
  `<name>-s0`, `<name>-s1`... and kept as one **group**, `<name>`. `compare
  <name>` ranks a group by its seeds' mean on every game, and the web
  Rankings can combine each group into one row. **Use this for anything that
  learns**: one seed can settle into a different style of play from the
  next (one `dqn-rich-hand` seed builds Tetrises, another plays clean
  doubles), so a single run says little about a setup.

For an agent that learns:

- `--steps N`: how long it trains (`omagym agents` shows each default).
- `--eval-every N`, `--eval-episodes N`, `--quick-max-steps N`: the quick
  evaluations during training that draw its learning curve (every tenth of
  the run, 5 games, cut at 500 pieces; they only need the trend).
- `--train-mix TEST=SHARE`: train on some games of another test too, e.g.
  `--train-mix challenge=0.3` for three games in ten on a dealt mess. Each
  training game picks its setup at random; the tests stay the same fixed
  games. A run records its mix, and `show` and `diff` say it.
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
6. **For learners, always several seeds**: `run --seeds 3` (5 when a
   difference matters), and compare the groups, not single runs. Training
   is noisy enough that one seed per setup has already misled us: what
   looked like "training on Challenge kills Tetrises" was the seed.

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
README.md                this file: intro, science, to do
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
  web/                   `omagym web`: the JSON API, and the page in static/
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

# Science

A map of the field before diving into any one algorithm. It covers the
problem every method is solving, the families of methods, the techniques
they share, how to tell whether one is better than another, and which of
them suit which omagames game. The Intro says how to *use* omagym; this
part says what to try with it, and why.

Names in `code font` are omagym's where they exist already.

## 1. The problem, stated once

Almost every method below solves the same problem, the **Markov decision
process** (MDP):

- At each step the game is in a **state** *s* (the board and the current
  piece, the snake and the food).
- The agent picks an **action** *a* from those allowed (the env's `mask`).
- The game moves to a new state *s'*, partly at random (the next piece, where
  the food appears), and hands back a **reward** *r*.
- The game may **end** (topped out, ran into itself) or be **cut** (the step
  cap). In the env these are `terminated` and `truncated`, and the
  difference matters: an ending has no future, a cut does.

The agent's behaviour is a **policy** π(a | s): what it does in each state,
possibly at random. The goal is the policy that collects the most
**return**, the sum of rewards from now on, usually **discounted**:
r₀ + γr₁ + γ²r₂ + …, with γ (gamma) a little under 1. Discounting makes a
reward now worth more than the same reward later, and keeps infinite games'
sums finite. γ = 0.95 means a reward 20 steps ahead counts about a third as
much; γ = 0.99, about four fifths.

Two quantities appear everywhere:

- The **value** V(s): the return to expect from state *s*, playing on with
  the policy.
- The **action value** Q(s, a): the same, having taken action *a* first.

If you knew Q exactly, playing well would be trivial: take the action with
the biggest Q. Much of the field is ways of estimating V or Q.

Four distinctions sort the methods:

| Distinction | One side | Other side |
|---|---|---|
| What is learned | a **value** (V or Q), and the policy is "pick the best" | the **policy** itself, directly |
| Use of a model | **model-free**: learn only from playing | **model-based**: use, or learn, a simulator of the game to plan ahead |
| Whose experience | **on-policy**: learn only from the current policy's play, then throw it away | **off-policy**: learn from any play, including old play kept in a buffer |
| What it optimises | **reinforcement learning**: improve from rewards, during play | **black-box search**: treat the agent as a box of numbers, score whole games, keep the better numbers |

And one tension runs through all of them: **exploration versus
exploitation**. An agent that always does what it currently thinks is best
never finds out it was wrong. One that keeps trying things never cashes in.

### Our games in these terms

| | Omatris | Omasnake | Trackmania (for later) |
|---|---|---|---|
| State | board, piece, next pieces, hold: fully visible | grid, snake, food: fully visible | car physics and the track: visible only through sensors or pixels |
| Randomness | the next pieces | where food appears | none: the physics is deterministic |
| Actions | depends on the action space: a landing (`placement`), a column and rotation (`drop`), or key presses (`raw`) | 3 or 4 directions | steering, throttle, brake: continuous or discretised |
| Steps per game | hundreds to thousands of pieces | thousands of moves | thousands of frames per minute of driving |
| Reward | arrives at once with each line, if the action space is `placement` | sparse: a dot now and then, a crash at the end | whatever you design, usually progress along the track |
| Can we simulate ahead? | yes, exactly: `clone()` | yes: `clone()` | not really (save states, at best) |

The last row matters most. Because omagames *are* simulators, every
model-based method is open to us for free, and that's rare.

## 2. The families

From least to most machinery. Each lists the classic methods, what it is
good and bad at, and the result worth knowing.

### 2.1 No learning: heuristics and search

A person writes the strategy, or a search tries futures with the real game.

- **Hand-written evaluation.** Score each option with features a person
  chose, and weights a person tuned. `greedy` is this: four features
  (height, holes, bumpiness, lines) and a weighted sum. The famous Tetris
  one is Pierre Dellacherie's six-feature controller, which clears hundreds
  of thousands of lines with one piece of lookahead.
- **Lookahead search.** Try each move in a copy of the game and look deeper:
  depth-limited search, **beam search** (keep only the best few lines of
  play at each depth), and for games with chance, **expectimax** (average
  over what the dice might do rather than assume the worst). Omatris's
  preview pieces make two-piece lookahead exact.

Good: no training, fast, easy to understand, often very strong. This is the
**baseline** every learner has to beat, and in Tetris it's a high bar.
Bad: you have to know the game well enough to write it, and it never
surprises you.

### 2.2 Black-box optimisation: evolutionary methods

Treat the agent as a vector of parameters θ (greedy's weights, or a whole
network's). Play some games with θ, score it, change θ, keep what scores
better. No gradients, no notion of states or steps: just "these numbers
scored that much".

- **Random search** and **hill climbing**: perturb θ, keep the change if it
  helps. Simple and surprisingly hard to beat on small problems.
- **Cross-entropy method (CEM)**: keep a Gaussian over θ. Sample a
  population (say 100), play each, keep the best 10%, refit the Gaussian to
  them, repeat. Add noise to the spread so it doesn't collapse too soon.
- **CMA-ES**: CEM's grown-up sibling. It also learns how the parameters vary
  *together*, so it handles correlated, badly scaled parameters well. Often
  the strongest choice for under a few hundred parameters.
- **Genetic algorithms**: a population with mutation and crossover
  (children mixing two parents' θ).
- **NEAT**: evolves the network's *shape* as well as its weights. SethBling's
  MarI/O, which learned a Super Mario level, used it.
- **Evolution strategies (ES)**, as OpenAI scaled them: estimate the
  gradient of the score from many random perturbations, and step along it.
  It parallelises perfectly, so it competes with RL on big networks given
  enough CPUs.

Good: dead simple, robust to sparse and delayed rewards (only the final
score matters), and embarrassingly parallel. Bad: wasteful of games, since
a whole game yields one number, and it scales badly beyond a few thousand
parameters (ES excepted).

The result to know: **Szita and Lőrincz (2006) used noisy CEM on
Dellacherie-style features and got hundreds of thousands of Tetris lines**,
far more than the RL methods of the time. For years the "dumb" method beat
the clever ones at Tetris, and why that was is a good lesson in itself.

### 2.3 Value-based reinforcement learning

Learn V or Q from experience, and act by picking the best.

The core idea is **temporal-difference (TD) learning**. You don't wait for
the game to end to learn what a state was worth. After one step you already
have a better guess: the reward you just got plus the (discounted) value of
where you landed. Move your estimate toward that:

    Q(s, a) ← Q(s, a) + α · [ r + γ · maxₐ' Q(s', a') − Q(s, a) ]

The bracket is the **TD error**: how surprised you were. α is the learning
rate. This one line is **Q-learning**. Learning from your own guesses like
this is called **bootstrapping**.

The classic methods:

- **Tabular Q-learning** and **SARSA** keep Q in a table, one entry per
  state and action. They are exact and provably converge, but only for games
  small enough to tabulate. Not Tetris: a 10×20 board alone can be filled
  2²⁰⁰ ways. SARSA learns the value of what you actually do next;
  Q-learning, the value of the best next move.
- **Monte Carlo** methods wait for the end of the game and use the real
  return: no bootstrapping, and no bias, but noisy. **n-step returns** and
  **TD(λ)** sit in between: use n real rewards, then bootstrap.
- **Function approximation**: replace the table by a function of the
  state's features, a linear model or a neural network. Now similar states
  share what they learned, and huge games become possible. It also breaks
  the convergence guarantees, which is where the tricks below come from.
- **Afterstates.** When the random part of a step comes *after* your choice
  takes effect (the piece lands, *then* the next one is drawn), learn the
  value of the state right after your move instead of Q(s, a). In Tetris
  that's the board your piece leaves. One V over boards replaces a Q per
  action, and choosing becomes "which landing leaves the best board". This is
  what `dqn` does, and why it learns Tetris in minutes. TD-Gammon (1992),
  which learned backgammon at master level, worked the same way.

**DQN** (Mnih et al., 2015, learned many Atari games from pixels) is
Q-learning with a neural network, made stable by two tricks:

- an **experience replay buffer**: store every step and learn from random
  samples of it, so consecutive, correlated steps don't all pull the same way,
  and each step is reused many times;
- a **target network**: a frozen copy of the network computes the
  r + γ·max Q′ target, and is only refreshed every few thousand steps. That
  stops the target from moving every time the estimate does.

DQN's improvements, each a small change worth trying on its own:

| Improvement | Fixes | Idea |
|---|---|---|
| **Double DQN** | Q-values creep too high, because max over noisy estimates picks the lucky ones | one network picks the best next action, the other scores it |
| **Dueling** network | learning "this state is bad" separately for every action | split Q into a state value plus per-action advantages |
| **Prioritised replay** | most samples teach nothing | replay surprising steps (big TD error) more often |
| **n-step** returns | rewards trickle back one step per update | bootstrap after n real rewards |
| **Distributional** (C51, QR-DQN, **IQN**) | one average hides risk | learn the whole distribution of returns, not just its mean |
| **Noisy nets** | ε-greedy explores blindly | learnable noise in the weights decides how much to explore, state by state |
| **Rainbow** (2018) | | all of the above together; a standard strong baseline |

Good: **sample-efficient**, because off-policy learning from a buffer reuses
every step many times. A natural fit for discrete actions. Bad: unstable
and sensitive to settings; needs discrete actions (or a trick); can be
fooled by its own over-optimistic estimates.

The result to know: the Trackmania AI **Linesight**, which beats human
records, is built on IQN, a distributional DQN.

### 2.4 Policy gradients and actor-critic

Learn the policy π_θ(a | s) directly, as a network that outputs action
probabilities. Play, then make the actions that led to good returns more
likely and the others less.

- **REINFORCE**: the plain version. After each game, push up the log
  probability of each action taken, weighted by the return that followed.
  Unbiased but very noisy.
- **Baselines** and the **advantage**: subtract what you expected to get.
  An action is reinforced by how much *better than usual* it was, its
  advantage A(s, a) = return − V(s), not by the raw return. Same
  expectation, much less noise.
- **Actor-critic**: a second head, the **critic**, learns V(s) (by TD, as in
  2.3) to supply the baseline, while the **actor** is the policy. Most
  modern methods are actor-critics.
- **A2C / A3C**: actor-critic over many envs in parallel, so each update
  sees varied, less correlated experience.
- **GAE** (generalised advantage estimation): a dial (λ) between noisy
  Monte Carlo advantages and biased TD ones; the usual default.
- **TRPO**, then **PPO** (Schulman et al., 2017): a policy-gradient step that
  is too big can wreck the policy in one update. PPO **clips** each update
  so the policy can't move far from the one that collected the data. It is
  simple, robust and the default first choice in much of RL today (it
  trained OpenAI Five for Dota 2, and RLHF for language models).

For **continuous** actions (steering angles), the off-policy actor-critics:

- **DDPG** and its fix **TD3**: a deterministic actor trained to maximise a
  learned Q.
- **SAC** (soft actor-critic): also rewards the policy for staying random
  (maximum entropy), which makes it explore well and train stably. The usual
  first pick for continuous control, and common in robotics.

Good: handles any action space, including continuous ones and huge discrete
ones; learns stochastic policies; PPO in particular is forgiving. Bad:
on-policy methods (REINFORCE, A2C, PPO) throw their data away after each
update, so they need many more steps than DQN. That's affordable only when
the simulator is fast, which ours is.

### 2.5 Model-based: planning with a simulator

Use a model of the game to look ahead before acting, either the real game
(when you have it) or one the agent learns.

- **Monte Carlo tree search (MCTS)**: grow a tree of possible futures from the
  current state. Repeatedly walk down it, choosing moves that are either
  promising or under-explored (the **UCB** rule), play out or evaluate the
  leaf, and back the result up the tree. Spend more time searching and get a
  better move. In a game with chance, the env's `clone(reseed_hidden=True)`
  matters: it deals each simulated future its own random pieces, so the
  search can't cheat by peeking at the real ones.
- **AlphaZero** (2018): MCTS guided by a network that proposes moves and
  values positions; the network is trained to predict what the search
  found. Search makes the network better, and the network makes search
  better. Superhuman at Go, chess and shogi from the rules alone.
- **MuZero** (2020): AlphaZero without being given the rules. It learns its
  own model of what happens next, only as far as it matters for value and
  reward, and searches in that.
- **Learned world models** (World Models, **Dreamer**): learn a compact
  simulator of the game from play, then train the policy largely inside the
  model's "dream", which needs far fewer real steps.

Good: the strongest results in board games; the agent gets stronger just by
thinking longer. Bad: slow per move, and complex. A learned model's errors
compound the further ahead it looks.

### 2.6 Learning from demonstrations: imitation

Learn from examples of good play rather than from rewards.

- **Behavioural cloning**: plain supervised learning. Given recorded games,
  train a network to predict the action the expert took in each state. No
  RL at all.
- **DAgger**: cloning drifts. One small mistake reaches states the expert
  never visited, where the clone has no idea what to do. DAgger lets the
  learner play, asks the expert what it *should* have done in the states it
  reached, adds those, and retrains.
- **Pre-training then RL**: clone first to get a competent start, then
  improve with RL. AlphaGo started from human games.

We have experts and recordings for free: `greedy` can label any state, and
every evaluation saves replays. Cloning greedy is a gentle first neural
network: it's supervised learning, with no RL instability to debug at the
same time.

### 2.7 Further out

Worth knowing exist; none is a first step.

- **Curriculum learning**: start on easy versions (slow gravity, a short
  snake, a short track) and make them harder as the agent improves.
- **Intrinsic motivation** (curiosity, ICM, RND): reward the agent for
  reaching states it can't yet predict, for games where real rewards are too
  rare to stumble upon.
- **Hierarchical RL**: one policy picks goals ("set up a Tetris on the
  left"), another reaches them.
- **Self-play and multi-agent RL**: the opponent is another copy of the
  learner. Not relevant to our single-player games.

## 3. Techniques every family uses

Choosing an algorithm is often the smaller decision. These choices matter as
much, and they apply to most families above.

### What the agent sees: representation

- **Hand-made features** (heights, holes, distance to food) make learning
  easy and put a ceiling on what can be learned: the agent can't care about
  something no feature measures. `dqn --set inputs=features`.
- **Raw state** (every cell, every pixel) makes the network find its own
  features. It's slower, needs more data, and has no human ceiling.
  `--set inputs=board`.
- **Network shape** should fit the input: an **MLP** for a list of
  numbers, a **CNN** (convolutions) for grids and images, since a pattern
  means the same wherever it is on the board.
- **Frame stacking**: give the last few frames together, so a still image
  shows motion (which way the car is sliding).
- **Normalise inputs** to roughly 0..1 or mean 0, variance 1. Networks learn
  badly when one input is in the hundreds and another in fractions.

### What it can do: action space design

Often the biggest lever of all. The same game with a different action space
is a different problem. Omatris has three on purpose:

- `placement`: "put the piece *here*", from a list of reachable landings.
  One decision per piece, reward right away. Easy.
- `drop`: column and rotation, then a hard drop. Fixed-size, so any policy
  network can output it, but it can't express slides or spins.
- `raw`: key presses, one decision every few frames. Credit for a line
  arrives dozens of decisions after the presses that earned it. This is the
  Trackmania problem.

Related techniques:

- **Action masking**: make illegal actions impossible (`mask`) rather than
  punishing them, so the agent never wastes time learning what's not allowed.
- **Frame skip** (action repeat): hold each action for k frames
  (`frame_skip`). Fewer, more meaningful decisions, and k times faster.
- **Discretising** continuous actions (steer left, centre, right) to use
  DQN, as Linesight does.

### What it is rewarded for: reward design

The agent optimises exactly what you reward, often in ways you didn't
intend.

- **Sparse** rewards (only at the end, or only for lines) are honest but
  hard to learn from. **Dense** rewards (a little every step) are easier and
  easier to get wrong.
- **Reward shaping** adds hints, like a penalty for holes or a reward for
  getting closer to the food. **Potential-based shaping** (Ng et al., 1999)
  is the safe form: reward the *change* in some measure of how good the state
  is, Φ(s') − Φ(s). That provably leaves the best policy unchanged.
- **Reward hacking**: reward survival and a Tetris agent may learn to stall;
  reward speed and a racer may learn to drive in circles through a
  checkpoint. Watch replays (`omagym watch`) to catch it.
- `dqn`'s reward is built from the env's named signals (`_reward()`), so
  trying another is a settings change, not a code change.

### Exploration

- **ε-greedy**: act randomly with probability ε, decaying over training.
  `dqn`'s `epsilon_*` settings.
- **Entropy bonus** (policy gradients): reward the policy for staying
  uncertain, so it doesn't commit too early.
- **Noise** on continuous actions, or in the weights (noisy nets).
- **Optimism**: start value estimates high, so untried actions look worth
  trying.

### Making training stable

- **Replay buffers** and **target networks** (2.3).
- **Vectorised envs**: step many games at once, for throughput and for less
  correlated batches (the ABI's `og_step_batch`).
- **Gradient clipping**, **reward scaling** and **advantage normalisation**:
  keep any one update from being huge.
- The **learning rate** is the first setting to blame when training
  diverges; **γ** is the second (higher means a longer horizon, and harder
  learning).
- **Hyperparameters matter enormously** in RL, more than in most ML. The
  same algorithm with different settings can look like a different
  algorithm. Change one at a time.

## 4. Telling whether something is better

Deep RL is notoriously noisy. A paper titled *Deep Reinforcement Learning
that Matters* (Henderson et al., 2018) showed that the same algorithm with
different random seeds could look as different as two algorithms. So:

- **Fixed evaluation games** that training never sees. omagym does this
  (`1_000_000_000 + i`), so every agent is tested on the same games.
- **Several training seeds** per configuration: at least 3, ideally 5 or
  more. Report the spread, not just the best seed.
- **Learning curves, not just final scores.** Compare against steps (sample
  efficiency) and against wall time. CEM may lose on steps and win on
  wall time; both are true.
- **Ablations**: when a change of three things helps, find out which one
  did, by removing them one at a time.
- **Robust statistics**: the interquartile mean and confidence intervals,
  rather than the mean of a few runs (*Deep RL at the Edge of the
  Statistical Precipice*, Agarwal et al., 2021, and its `rliable` library).
- **Watch it play.** A number says how well; a replay says how. Many bugs and
  every reward hack are obvious on screen and invisible in a table.

## 5. What to try on which game

| Game, action space | Good first choices | Why |
|---|---|---|
| Omatris, `placement` | CEM or CMA-ES on features; DQN on afterstates | the classic Tetris setting; a small number of parameters, and afterstates make value learning easy |
| Omatris, `drop` | PPO, DQN | fixed actions, so standard networks fit; harder, because it can't slide or spin a piece into place |
| Omatris, `raw` | PPO with frame skip; DQN with n-step returns | long horizons and delayed credit; rehearsal for Trackmania |
| Omatris, any | behavioural cloning of `greedy`, then RL | supervised first, RL second |
| Omatris, planning | MCTS or expectimax with `clone(reseed_hidden)`, using a learned value | stronger per move than any of the above, slower |
| Omasnake | DQN or PPO on the grid with a small CNN; greedy + lookahead search | sparse rewards and a long game; the snake boxing itself in is a planning problem |
| Trackmania, later | DQN family on discretised controls (Linesight's IQN); PPO or SAC on continuous ones | a slow, real-time simulator rewards sample efficiency |

## 6. A path through it

Each step introduces one new idea on something you already have working.

1. **Black-box search**: CEM on greedy's weights. The idea: optimising a
   score without gradients. Expect it to beat the hand-tuned weights.
2. **Supervised learning**: clone `greedy` with a small network. The idea:
   networks, losses, overfitting, with no RL in the way.
3. **Value learning**: `dqn` as it is, then its ablations (`inputs=board`,
   γ, no target network) and one or two Rainbow improvements. The idea: TD
   learning and what keeps it stable.
4. **Policy gradients**: PPO on `drop`, then `raw`. The idea: learning a
   policy directly, advantages, and long horizons.
5. **Planning**: MCTS with a learned value. The idea: search and learning
   working together.

Then a game nobody wrote features for, Snake from raw pixels say, to check
the method rather than your feature engineering is doing the work.

## 7. Where to read more

Start with the first two; the rest are for when a method becomes the next
thing you build.

- **Sutton & Barto, *Reinforcement Learning: An Introduction*** (2nd ed.,
  free online). The textbook. Chapters 1–6 and 13 cover most of section 2.
- **OpenAI *Spinning Up in Deep RL*** (free online). A short, practical
  introduction to the deep methods, with clear derivations of policy
  gradients.
- **CleanRL**: single-file implementations of DQN, PPO, SAC and others,
  written to be read. The best place to see one algorithm whole.
- **Hugging Face Deep RL course** (free): hands-on, from Q-learning to PPO.
- David Silver's **UCL RL lectures** (video): the theory, clearly.
- Papers, by family:
  - Black-box: Szita & Lőrincz, *Learning Tetris using the noisy
    cross-entropy method* (2006); Hansen, *The CMA evolution strategy: a
    tutorial*; Salimans et al., *Evolution strategies as a scalable
    alternative to RL* (2017).
  - Value: Mnih et al., *Human-level control through deep RL* (DQN, 2015);
    Hessel et al., *Rainbow* (2018); Dabney et al., *Implicit quantile
    networks* (IQN, 2018).
  - Policy: Schulman et al., *Proximal policy optimization algorithms*
    (2017); Haarnoja et al., *Soft actor-critic* (2018).
  - Planning: Silver et al., *AlphaZero* (2018); Schrittwieser et al.,
    *MuZero* (2020); Hafner et al., *DreamerV3* (2023).
  - Tetris specifically: Gabillon, Ghavamzadeh & Scherrer, *Approximate
    dynamic programming finally performs well in the game of Tetris*
    (2013), a good read on why it took RL so long to catch up with CEM.
- For Trackmania: Yosh's videos, and the **Linesight** project's code and
  write-ups.

## 8. Techniques worth trying with any learner

Not tied to one agent; most apply to `cem`, `dqn`, `mcts` and `ppo` alike.

**Getting more from the same experience**
- **Symmetry**: mirror boards left to right (L swaps with J, S with Z) to
  double the data; any game with a symmetry has this for free.
- **Reuse**: prioritised replay, and self-imitation (replaying the agent's
  own best games, the ones that went better than it expected).
- **Demonstrations**: start from recordings of a stronger player (greedy,
  mcts, or you, through the app) rather than from nothing.

**Shaping what it learns from**
- **Reward shaping**, potential-based so it can't change the best play.
- **Curricula**: easy versions first (low gravity, short games, a clean
  board), harder as it improves; or the reverse, start from the hard
  positions it keeps losing.
- **Auxiliary tasks**: have the network also predict things it can check,
  like the next piece's best landing, the holes after a move or the lines
  cleared. Learning those shapes features the value can use.

**Choosing settings**
- **Random search** over settings, with fixed seeds and `compare`: usually
  beats tuning by hand, and grid search, for the same number of runs.
- **Bayesian optimisation** (Optuna, for instance) once a run is expensive.
- **Population-based training**: settings that change during the run.
- **Schedules**: a learning rate, exploration or entropy bonus that decays
  over training, rather than a constant.

**Bigger and better networks**
- Residual CNNs, LayerNorm, attention across the board's columns; scale up
  until the GPU is busy, since the game side is rarely the limit for a big
  network.
- **Distillation**: train a small, fast network to copy a big or a searching
  one, then deploy the small one.
- **Ensembles**: several networks, averaged; their disagreement is a measure
  of uncertainty, which is useful for exploring.

**Exploration**
- Noisy networks, Boltzmann (softmax) exploration instead of epsilon-greedy,
  count-based bonuses, curiosity (RND) for sparse rewards.

**Speed, which buys everything else**
- Vectorised envs (`og_step_batch`), parallel actors (Ape-X, IMPALA): many
  games playing while one learner trains, on all your cores.
- Batched network calls on the GPU, in training and in search.

**Measuring honestly** (section 4)
- Several seeds before believing anything, the same evaluation games for
  everyone, and ablations: when three changes help, find out which did.

# To do

The experiments worth running next, roughly in order. Each is a run (or a
few) to `compare` with what is there now.

## 1. Thompson sampling as mcts's selection rule (smallest, do first)

A plain bandit over landings is just greedy: it assumes a choice pays off
now and changes nothing after, while in Tetris a landing's cost shows up
pieces later. *Hierarchical* bandits, one per decision with payoffs backed
up from the ones below, are tree search: UCT is "UCB applied to trees".
Thompson sampling instead of UCB is a published variant (Bai et al., 2013,
Thompson-sampling Monte Carlo planning).

- In `mcts.py`, a `--set selection=thompson` beside PUCT, in `_select()`.
- A posterior per landing: a Normal whose mean is the average value found
  and whose spread shrinks with visits (the network's rating as the prior
  for unvisited ones). Sample one value per landing, descend into the
  highest. About 40 lines.
- Experiment: PUCT against Thompson at the same simulations (32, then 8 and
  128); TS tends to explore better on small budgets, which is ours.

## 2. XGBoost, learning to rank from a stronger player

XGBoost is supervised: it fits a function from labelled examples, it
doesn't play or explore. Two places it fits:

- **Learning to rank (preferred).** Record mcts (or lookahead) playing; for
  each position, the landings with `rich_hand` features, labelled by which
  one the planner chose. Train with `rank:pairwise` (or `rank:ndcg`), one
  query group per position. Trees capture interactions a weighted sum can't
  ("holes matter more when the stack is high"). Expect much of mcts's play
  at greedy's speed: distillation with a tree ensemble as the student.
- **Fitted value iteration** (Ernst et al., 2005, Fitted Q Iteration):
  play games, targets r + gamma * V(next board) from the current model,
  refit on all of them, repeat. The RL route to trees; less stable, and it
  refits in batches.

Pieces needed: `xgboost` in the `learn` group of pyproject.toml; a new agent
`agents/xgb.py` (trainable: train() collects demonstrations by playing the
teacher, then fits; save/load the booster); the teacher as a setting
(`--set teacher=<mcts run>`). Inference over ~40 landings a move is fast.

## 3. A hierarchical action space for ppo

Choose hold or not, then the rotation, then the column, each a small choice
of its own (a factored or autoregressive policy), instead of one softmax
over 80 drops. Fewer options at each step, and what's learned about a column
is shared across rotations.

## 4. Get dqn-rich-hand-mix's score back without losing its digging

What happened (one seed each): training 50/50 on Challenge made
`dqn-rich-hand` tidy and safe (stack 7.0 -> 5.0 rows, holes per piece 1.4 ->
0.3, 20/20 survived, spread +-360k -> +-46k, Challenge 3.7 -> 1.6) but it
stopped building for Tetrises: points per line 10,595 -> 8,032, Marathon
10.5M -> 8.0M. Digging rewards clean low boards; Marathon's best play is a
tall stack with an open well. It also got half the Marathon practice.

Experiments, each against `dqn-rich-hand` and `dqn-rich-hand-mix` (tick all
three in `omagym web` -> Compare, on both tests):

- A smaller share: `--train-mix challenge=0.2`.
- Longer training so Marathon practice isn't halved: `--steps 300000`.
- A cheaper digging reward: `--set reward_dealt_row=3` (+10 equals a line,
  and may outweigh building for Tetrises).
- Pay Tetrises outright: a bigger `--set reward_line=...` so building for one
  beats digging when there's nothing left to dig.
- More than one seed (`--seed 1/2/3`) before believing any of it.

## 5. Things to try, agent by agent

Concrete next experiments for each agent here: first the ones a `--set` or
`--env` away, most promising first, then the algorithmic ones that change
the code (marked "code"). Run it with a `--name`,
`compare` it with the agent's current run, and change one thing at a time.

### `greedy`

1. **Tune one weight at a time** (`--set holes=-0.5`, `--set height=-0.3`)
   and `compare` each with the default. It is the cheapest way to get a feel
   for how much each feature matters.
2. **Value a Tetris above four singles** (code: score `clear4` on its own,
   as `cem`'s rich features do). Today its `lines` weight is linear, so it
   never builds for one, which is most of the score it leaves behind.
3. **Add a well feature** (code: `deepest_well` from `omatris.rich_features`)
   with a positive weight for one deep well beside a flat stack.
4. **Dellacherie's features** (code): landing height and *eroded cells*
   (lines cleared times the piece's own cells in them), the two that let
   his hand-tuned controller clear hundreds of thousands of lines.
5. **A T-slot feature** (code): reward leaving a slot a T can spin into, so
   it sets up T-spins, which score more per line than anything but a Tetris.
6. **Learn its weights from a stronger player** (code): record `mcts`'s
   choices and fit greedy's weights to them (a linear ranking model). It's
   imitation, and it shows how much of mcts's play a linear rule can capture.

### `cem`

1. **Train on the games it is tested on:** `--set episode_steps=2500`. It
   trains on 300-piece games that never get past level 12, so it never
   meets 20G, learns to stack high, and tops out at the high levels.
2. **Fewer noisy scores:** `--set games=4` (every candidate plays four
   games, not two) and `--set population=100`. Slower per generation, but
   the elite are chosen for being good, not for being lucky.
3. **An objective that counts survival:** `--set objective=lines` against
   `score`, or (code) score with a large penalty for topping out.
4. **Start from greedy** (code: initialise `mean` from greedy's weights in
   rich units, as `lookahead._GREEDY` does) instead of from zero, and see
   how many generations that saves.
5. **Use your 20 cores** (code: score the population in a process pool). The
   candidates are independent, so this is the biggest speed-up available.
6. **CMA-ES instead of CEM** (code): it also learns how the weights vary
   *together* (a full covariance, not one spread per weight), so it handles
   features that trade off against each other, like height and holes.
7. **Weight the elite by rank** (code): the best candidate counts more than
   the tenth best in the refit, rather than all elite counting the same.
8. **Mirrored sampling** (code): for every candidate mean + noise, also try
   mean - noise on the same games. It halves the noise in comparing them.
9. **Evolve a small network, not a weighted sum** (code): the same method
   over the weights of a tiny MLP (neuroevolution), so the evaluation can
   say "holes matter more when the stack is high", which a sum can't.
10. **Restarts** (code): when the spread collapses, restart around the best
    mean with a wider spread (IPOP-style), so one bad generation can't end
    the search in a local optimum.
11. **A cheaper first round** (code): score every candidate on one short
    game, and only the promising ones on full games (successive halving).

### `dqn`

1. **What it sees:** `--set inputs=rich_hand` or `cnn_hand`, so it can
   value the held and next pieces. The runs `dqn-rich`, `dqn-rich-hand`,
   `dqn-cnn` and `dqn-cnn-hand` are this experiment.
2. **Train longer, on more seeds:** `--steps 500000 --set
   epsilon_steps=200000`, with `--seed 1`, `2` and `3`. 100,000 steps is
   short, and one seed can't tell you much.
3. **A longer horizon:** `--set gamma=0.99`. At 0.95 a reward 20 pieces away
   is worth a third; a Tetris is set up over more pieces than that.
4. **The improvements in its docstring, one at a time** (code): n-step
   returns, then Double DQN targets, then the score as the reward.
5. **Prioritised replay** (code): learn more often from the transitions it
   predicted worst.
6. **Shape the reward** (code), potential-based so it can't change what the
   best play is (Science, section 3): reward the change in a board potential,
   r + gamma * Phi(s') - Phi(s), with Phi = -(holes * a + height * b). The
   agent hears about a hole the moment it makes one, not twenty pieces later
   when it tops out.
7. **A true max target** (code): store every landing on offer next, not
   just the one played, and bootstrap from the best of them. That's real
   Q-learning on afterstates, rather than the one-sample version it does now.
8. **Learn the spread, not just the mean** (code): a distributional value
   (quantile regression, as in QR-DQN or IQN, which Linesight uses). Topping
   out is rare but ruinous, and a mean hides how often it happens.
9. **A curriculum** (code): start some training games from messy mid-game
   boards (Challenge mode's dealt rows) or at high gravity, so it learns to
   recover from trouble instead of only avoiding it.
10. **Mirror the board** (code): Tetris is symmetric left to right if L
    swaps with J and S with Z. Store every transition mirrored too and it
    learns from twice the experience for free (data augmentation).
11. **A softer target network** (code): Polyak averaging, the target moving
    a little toward the network every step (tau = 0.005) rather than jumping
    every `target_sync` steps.
12. **An ensemble** (code): a few value networks trained side by side; act
    on their average, and explore where they disagree (bootstrapped DQN).
13. **Normalisation in the network** (code): LayerNorm between the layers,
    which lets a bigger network and a higher learning rate train stably.

### `lookahead`

1. **Judge with a network:** `--set model=dqn` (or your best `dqn` run). It
   has only been tried with greedy's and cem's weights, and `mcts` shows how
   much a learned value adds to a search.
2. **Deeper and wider:** `--set depth=3`, then `--set beam=16`. Note what it
   costs in time as well as what it gains in score.
3. **Average the unseen pieces** (code): beyond the preview, play each line
   on a few reseeded copies and average them (expectimax), rather than
   trusting one guess at the pieces to come.
4. **A judge trained for long games:** a `cem` run with `episode_steps=2500`
   as its `model`.
5. **Merge lines that reach the same board** (code): two orders of the same
   two pieces often end on the same board; keep one, and spend the beam on
   genuinely different futures (a transposition table).
6. **An adaptive beam** (code): wide when the stack is high or the board is
   messy, narrow when it is calm, so the time goes where mistakes cost.
7. **Count the hold in the leaf** (code): rate a line's last board together
   with what is held then, for instance with a `dqn` judge trained on
   `rich_hand`, so lines that keep an I for later stop looking the same as
   lines that waste it.
8. **A time budget instead of a fixed depth** (code): search deeper while
   time is left (iterative deepening), so it thinks hard on difficult
   pieces and quickly on easy ones.

### `mcts`

1. **More thinking:** `--set simulations=64`, then `128`. It is the one agent
   that gets stronger simply by searching more; see how far that goes.
2. **A better starting network:** `--set model=<your best dqn>`, for
   instance one trained with `inputs=rich_hand`.
3. **Learn for longer:** `--steps 100000`. Its own training (the network
   learning from the search) has only had 20,000 moves.
4. **Tune the search:** `c_puct` (1.0, 2.5) and `prior_temperature` (0.25,
   1.0) trade trusting the network against exploring.
5. **Faster search** (code): rate the leaves of several simulations in one
   network call on the GPU. Speed buys simulations, and simulations buy
   score.
6. **A policy head** (code): train a second output to predict the search's
   visit counts and use it as the prior, as AlphaZero does, instead of
   priors made from the value ratings. The search then gets better at
   knowing where to look, not just at judging what it finds.
7. **Better value targets** (code): mix the searched value with the
   rewards that actually followed in the game (n-step returns or TD(lambda)),
   rather than the search's estimate alone.
8. **Chance nodes beyond the preview** (code): there, expand each landing on
   several reseeded copies and average them, so the tree stops treating one
   guess at the hidden pieces as the future.
9. **Gumbel root selection** (code, Danihelka et al., 2022): sequential
   halving among the root's best few landings. It is designed for small
   simulation budgets like this one, where plain PUCT wastes visits.
10. **Reuse the tree within the preview** (code): the part of the tree
    whose pieces were all visible is still right after the move; keep it
    instead of starting from nothing.
11. **Distil the search** (code): train a fast network to copy mcts's
    moves, then play with the network alone: most of the strength at a
    fraction of the time per move.

### `ppo`

1. **Far more steps:** `--steps 10000000 --set envs=32`. A million steps is
   the very start for a policy learning Tetris from the raw well.
2. **Shape the reward** (code, potential-based so it can't change what is
   best, Science, section 3): a little for each line, and a penalty that
   grows with the holes and height.
3. **See the board as a picture** (code): a CNN over the well instead of
   one-hot cells into a plain network, as `dqn`'s `cnn` does.
4. **The Trackmania rehearsal:** `--env actions=raw --env frame_skip=4`, a
   key press every few frames, and watch how much harder credit assignment
   gets.
5. **Snake:** `--game omasnake` with a reward for getting closer to the food
   (code). Without it, a random snake almost never finds a dot to learn
   from.
6. **Score the landings, not 80 fixed drops** (code): in the placement
   action space, let the policy be a softmax over a network's rating of each
   landing's board (an afterstate policy). It inherits the trick that makes
   dqn work, and is the biggest lever here.
7. **Start from a demonstration** (code): behavioural cloning on `greedy`'s
   or `mcts`'s replays first, then PPO from there. It starts competent and
   only has to improve, rather than discover Tetris from scratch.
8. **Curiosity for sparse rewards** (code): an intrinsic reward for reaching
   states it can't yet predict (RND, Burda et al., 2018). The textbook fix
   for Snake, where real rewards are too rare to stumble on.
9. **Memory for raw keys** (code): stack the last few frames, or give the
   policy a recurrent layer, so it knows which way it was moving the piece.
10. **Population-based training** (code): several PPO runs at once; every
    so often the worst copy the weights and settings of the best, with
    small mutations. The settings tune themselves during training.
11. **A residual CNN encoder** (code) for the board, shared by actor and
    critic, rather than one-hot cells into two separate plain networks.
