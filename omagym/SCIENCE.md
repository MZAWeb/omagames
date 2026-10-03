# The science: how programs learn to play games

A map of the field before diving into any one algorithm. It covers the
problem every method is solving, the families of methods, the techniques
they share, how to tell whether one is better than another, and which of
them suit which omagames game. `README.md` says how to *use* omagym; this
file says what to try with it, and why.

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

## 8. Things to try, agent by agent

Concrete next experiments for each agent here, most promising first. Each
is a `--set` or `--env` away unless it says "code". Run it with a `--name`,
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

### `ppo`

1. **Far more steps:** `--steps 10000000 --set envs=32`. A million steps is
   the very start for a policy learning Tetris from the raw well.
2. **Shape the reward** (code, potential-based so it can't change what is
   best, SCIENCE.md section 3): a little for each line, and a penalty that
   grows with the holes and height.
3. **See the board as a picture** (code): a CNN over the well instead of
   one-hot cells into a plain network, as `dqn`'s `cnn` does.
4. **The Trackmania rehearsal:** `--env actions=raw --env frame_skip=4`, a
   key press every few frames, and watch how much harder credit assignment
   gets.
5. **Snake:** `--game omasnake` with a reward for getting closer to the food
   (code). Without it, a random snake almost never finds a dot to learn
   from.
