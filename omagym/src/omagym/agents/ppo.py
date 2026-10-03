"""Proximal policy optimisation (PPO): learning the policy itself.

**The idea.** Every other learner here learns a *value* and acts by "take
the best". PPO learns the *policy* directly: a network that, given what the
agent sees, outputs a probability for every action (README, Science 2.4). It
plays by sampling from those probabilities, and learns by making the
actions that turned out better than expected more likely, and the others
less.

It needs no afterstates, no features, no copy of the game: only the
observation and the action mask. So this is the one agent here that plays
any game. Tetris in the `drop` action space (a column and a rotation),
Tetris pressing keys (`--env actions=raw --env frame_skip=4`), or Snake.

**The loop**, repeated forever:

1. **Collect.** Play `envs` games side by side for `rollout` steps each,
   recording what was seen, done, and earned.
2. **Judge each action** with an *advantage*: how much better it turned out
   than the critic expected. The critic is a second network that learns
   V(observation), the reward to expect from here. An action with a
   positive advantage was a pleasant surprise. GAE (generalised advantage
   estimation) blends one-step and many-step views of "how it turned out"
   with `gae_lambda`.
3. **Update** both networks for a few `epochs` over that batch, then throw
   the batch away (PPO is on-policy: it learns only from its current self).

**The "proximal" part.** A policy-gradient step that is too big can wreck a
policy that took hours to learn. PPO clips each update: once an action's
probability has moved more than `clip` (20%) from what it was when the
batch was collected, that action stops pushing. Simple, and it is why PPO is
the default choice in so much of reinforcement learning.

**What to expect on Tetris:** weaker than the afterstate agents, and that is
the lesson. They are told every landing and its board; PPO has to work out
from the raw well what each of 80 actions does. With raw key presses it is
harder still: a line's reward comes dozens of presses after the ones that
earned it. That is the Trackmania problem in miniature.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

import numpy as np
import torch
from torch import nn

from . import register
from .base import Agent

# uint8 tensors hold categories (a cell's contents, a piece's kind) and are
# one-hot encoded with this many classes; anything higher shares the last.
# 2048's cells are powers of two up to 2^17, so it needs 18.
_CATEGORIES = {"oma2048": 18}
_DEFAULT_CATEGORIES = 8
# Tensors only the placement action space has: a policy over 80 drops has no use for them.
_SKIP = ("candidates", "afterstates")


@register
class PPO(Agent):
    name = "ppo"
    description = "Learns a policy network directly with proximal policy optimisation. Plays any game."
    trainable = True
    default_steps = 1_000_000

    @dataclass
    class Config:
        # Games played side by side, and steps of each per batch: a batch is
        # envs * rollout steps. Many games at once give varied, less
        # correlated experience.
        envs: int = 8
        rollout: int = 128
        # The update: passes over each batch, split into minibatches.
        epochs: int = 4
        minibatches: int = 4
        lr: float = 2.5e-4
        gamma: float = 0.99
        gae_lambda: float = 0.95
        clip: float = 0.2
        # How much the critic's error and the policy's randomness count in
        # the loss. The entropy bonus keeps the policy from committing to
        # one action before it knows enough.
        value_coef: float = 0.5
        entropy_coef: float = 0.01
        max_grad_norm: float = 0.5
        hidden: int = 256
        # How the policy chooses an Omatris drop: "flat", one choice among
        # all 80 (column x rotation x hold); or "factored", hold first, then
        # the rotation, then the column, each its own small choice (see
        # _Factored). Any other game is always flat.
        policy: str = "flat"
        # The reward: the game's score times `reward_scale` (networks learn
        # badly from rewards in the thousands), plus `death_penalty` when the
        # game is lost.
        reward_scale: float = 0.01
        death_penalty: float = -1.0
        # For a game won rather than lost (a Challenge cleared, a Sprint's
        # forty lines): without it, a game that ends by being won only pays
        # less than one played on. And per dealt row of a Challenge cleared.
        reward_win: float = 0.0         # try 1, the size of the death penalty
        reward_dealt_row: float = 0.0   # try 0.1
        # Hunger: a training game that goes this many steps without any reward
        # ends, and counts as lost. Without it, an agent that finds dying
        # costs more than scoring earns can learn to stall forever (a Snake
        # circling, never eating): reward hacking, README, Science section 3.
        patience: int = 300
        episode_steps: int = 2_000
        seed: int = 0

    @classmethod
    def env_config(cls, game: str) -> dict:
        # A fixed set of actions, which is what a policy network outputs.
        return {"omatris": {"actions": "drop"}, "omasnake": {"actions": "absolute"}}.get(game, {})

    def __init__(self, config: Config, spec, device: str = "cpu"):
        super().__init__(config, spec, device)
        torch.manual_seed(config.seed)
        self.rng = np.random.default_rng(config.seed)
        self.tensors = [t for t in spec.tensors.values() if t.name not in _SKIP]
        self.categories = _CATEGORIES.get(spec.game, _DEFAULT_CATEGORIES)
        size = sum(int(np.prod(t.shape)) * (self.categories if t.dtype == np.uint8 else 1) for t in self.tensors)
        self.normalizer = _RunningNorm(size)
        # Two networks: the actor (the policy, one output per action) and the
        # critic (the value, one output). Tanh and orthogonal initialisation
        # are the choices that make PPO behave, per "The 37 implementation
        # details of PPO"; the small gain on the actor's last layer starts it
        # off nearly uniform, trying everything.
        if config.policy not in ("flat", "factored"):
            raise ValueError(f"policy must be flat or factored, not {config.policy!r}")
        if config.policy == "factored" and spec.action_count != _Factored.ACTIONS:
            raise ValueError("a factored policy is for Omatris's drop actions (80 of them)")
        outputs = _Factored.OUTPUTS if config.policy == "factored" else spec.action_count
        self.actor = _mlp(size, config.hidden, outputs, last_gain=0.01).to(device)
        self.critic = _mlp(size, config.hidden, 1, last_gain=1.0).to(device)
        self.optimizer = torch.optim.Adam([*self.actor.parameters(), *self.critic.parameters()],
                                          lr=config.lr, eps=1e-5)
        self.steps = 0

    # -- seeing ---------------------------------------------------------------

    def _encode(self, obs) -> np.ndarray:
        """Every tensor the game shows, flattened into one vector of numbers.

        uint8 tensors are categories, so each value becomes a one-hot block
        (a cell holding food is not "four times" a cell holding body).
        int32 and float tensors are quantities, used as they are; the running
        normalisation in _RunningNorm puts them all on a similar scale.
        """
        parts = []
        for t in self.tensors:
            values = obs[t.name].reshape(-1)
            if t.dtype == np.uint8:
                parts.append(np.eye(self.categories, dtype=np.float32)[np.minimum(values, self.categories - 1)].reshape(-1))
            else:
                parts.append(values.astype(np.float32))
        return np.concatenate(parts)

    def _policy(self, x: torch.Tensor, mask: torch.Tensor):
        """The policy's distribution over legal actions for each observation in x."""
        if self.config.policy == "factored":
            return _Factored(self.actor(x), mask)
        return _Flat(self.actor(x), mask)

    def act(self, obs, mask, explore=False) -> int:
        x = torch.as_tensor(self.normalizer.apply(self._encode(obs))[None], device=self.device)
        with torch.no_grad():
            policy = self._policy(x, torch.as_tensor(mask[None], device=self.device))
        return int(policy.sample() if explore else policy.mode())

    # -- learning -------------------------------------------------------------

    def train(self, ctx):
        c = self.config
        envs = [ctx.make_env(max_steps=c.episode_steps) for _ in range(c.envs)]
        obs = [env.reset(ctx.next_seed()) for env in envs]
        self._hungry = np.zeros(len(envs), np.int64)   # steps since each game's last reward
        finished_scores: list[float] = []
        while True:
            batch = self._collect(envs, obs, ctx, finished_scores)
            stats = self._update(batch)
            yield {"steps": self.steps,
                   "episode_score": float(np.mean(finished_scores)) if finished_scores else float("nan"),
                   "episodes": float(len(finished_scores)), **stats}
            finished_scores.clear()

    def _collect(self, envs, obs, ctx, finished_scores) -> dict:
        """1. Plays every game on for `rollout` steps; what happened, as arrays."""
        c = self.config
        T, N = c.rollout, len(envs)
        seen = np.zeros((T, N, self.normalizer.size), np.float32)
        masks = np.zeros((T, N, self.spec.action_count), bool)
        actions, logprobs = np.zeros((T, N), np.int64), np.zeros((T, N), np.float32)
        values, rewards, dones = np.zeros((T, N), np.float32), np.zeros((T, N), np.float32), np.zeros((T, N), np.float32)

        for t in range(T):
            encoded = np.stack([self._encode(o) for o in obs])
            self.normalizer.update(encoded)
            seen[t] = self.normalizer.apply(encoded)
            masks[t] = np.stack([env.mask() for env in envs])
            x, m = torch.as_tensor(seen[t], device=self.device), torch.as_tensor(masks[t], device=self.device)
            with torch.no_grad():
                dist = self._policy(x, m)
                action = dist.sample()
                logprobs[t] = dist.log_prob(action).cpu().numpy()
                values[t] = self.critic(x).squeeze(1).cpu().numpy()
            actions[t] = action.cpu().numpy()

            for i, env in enumerate(envs):
                before = obs[i]
                step = env.step(int(actions[t, i]))
                self.steps += 1
                self._hungry[i] = 0 if step.reward > 0 else self._hungry[i] + 1
                starved = self._hungry[i] >= c.patience
                won = step.terminated and env.info().get("phase") == "finished"
                lost = (step.terminated and not won) or starved
                rewards[t, i] = c.reward_scale * step.reward + (c.death_penalty if lost else 0.0)
                rewards[t, i] += c.reward_win * won + c.reward_dealt_row * self._dealt_cleared(before, step.obs)
                if step.truncated and not starved:
                    # Cut short, not lost: the game had a future, worth about
                    # what the critic says, so that is added rather than
                    # pretending the reward stops here.
                    last = torch.as_tensor(self.normalizer.apply(self._encode(step.obs))[None], device=self.device)
                    with torch.no_grad():
                        rewards[t, i] += c.gamma * float(self.critic(last))
                dones[t, i] = float(step.done or starved)
                if step.done or starved:
                    finished_scores.append(env.info()["score"])
                    self._hungry[i] = 0
                    obs[i] = env.reset(ctx.next_seed())
                else:
                    obs[i] = step.obs

        # 2. Advantages, by GAE, walking backwards through time. delta is the
        # one-step surprise: reward + discounted value of the next state, minus
        # the value expected here. The advantage adds up the surprises ahead,
        # each discounted by gamma * lambda, stopping at the end of a game.
        with torch.no_grad():
            last = torch.as_tensor(self.normalizer.apply(np.stack([self._encode(o) for o in obs])), device=self.device)
            next_value = self.critic(last).squeeze(1).cpu().numpy()
        advantages = np.zeros((T, N), np.float32)
        running = np.zeros(N, np.float32)
        for t in reversed(range(T)):
            following = next_value if t == T - 1 else values[t + 1]
            alive = 1.0 - dones[t]
            delta = rewards[t] + c.gamma * following * alive - values[t]
            running = delta + c.gamma * c.gae_lambda * alive * running
            advantages[t] = running
        returns = advantages + values   # what the critic should have said

        flat = lambda a: a.reshape(T * N, *a.shape[2:])  # noqa: E731
        return {"seen": flat(seen), "masks": flat(masks), "actions": flat(actions), "logprobs": flat(logprobs),
                "advantages": flat(advantages), "returns": flat(returns)}

    def _dealt_cleared(self, before, after) -> float:
        """A Challenge's dealt rows this step cleared; 0 for a game without them."""
        stats = self.spec.tensors.get("stats")
        if stats is None or "dealt_rows_left" not in stats.labels:
            return 0.0
        column = stats.column("dealt_rows_left")
        return float(before["stats"][column] - after["stats"][column])

    def _update(self, batch: dict) -> dict:
        """3. A few passes over the batch, in shuffled minibatches."""
        c = self.config
        b = {k: torch.as_tensor(v, device=self.device) for k, v in batch.items()}
        size = len(b["actions"])
        stats: dict[str, list[float]] = {"policy_loss": [], "value_loss": [], "entropy": [], "clip_fraction": []}
        for _ in range(c.epochs):
            order = torch.as_tensor(self.rng.permutation(size), device=self.device)
            for rows in order.chunk(c.minibatches):
                dist = self._policy(b["seen"][rows], b["masks"][rows])
                logprob = dist.log_prob(b["actions"][rows])
                # How much likelier the action is now than when it was played.
                ratio = (logprob - b["logprobs"][rows]).exp()
                # Advantages normalised per minibatch: only their sign and
                # relative size matter, and this keeps the step size steady.
                adv = b["advantages"][rows]
                adv = (adv - adv.mean()) / (adv.std() + 1e-8)
                # The clipped objective: the smaller (more pessimistic) of the
                # plain and the clipped improvement, so no update gains by
                # moving an action's probability past the clip.
                clipped = ratio.clamp(1 - c.clip, 1 + c.clip)
                policy_loss = -torch.min(ratio * adv, clipped * adv).mean()
                value_loss = (self.critic(b["seen"][rows]).squeeze(1) - b["returns"][rows]).pow(2).mean()
                entropy = dist.entropy(b["actions"][rows]).mean()
                loss = policy_loss + c.value_coef * value_loss - c.entropy_coef * entropy

                self.optimizer.zero_grad()
                loss.backward()
                # Gradient clipping: no single minibatch gets to shove the
                # networks far, however surprising it was.
                nn.utils.clip_grad_norm_([*self.actor.parameters(), *self.critic.parameters()], c.max_grad_norm)
                self.optimizer.step()
                for key, value in (("policy_loss", policy_loss), ("value_loss", value_loss), ("entropy", entropy),
                                   ("clip_fraction", ((ratio - 1).abs() > c.clip).float().mean())):
                    stats[key].append(float(value))
        return {key: float(np.mean(values)) for key, values in stats.items()}

    def save(self, path: Path) -> None:
        torch.save({"actor": self.actor.state_dict(), "critic": self.critic.state_dict(),
                    "norm": self.normalizer.state(), "steps": self.steps}, path)

    def load(self, path: Path) -> None:
        state = torch.load(path, map_location=self.device, weights_only=False)
        self.actor.load_state_dict(state["actor"])
        self.critic.load_state_dict(state["critic"])
        self.normalizer.load(state["norm"])
        self.steps = state["steps"]


class _Flat:
    """One choice among every action; a masked one gets a hugely negative
    logit, so probability zero."""

    def __init__(self, logits: torch.Tensor, mask: torch.Tensor):
        self.dist = torch.distributions.Categorical(logits=logits.masked_fill(~mask, -1e9))

    def sample(self):
        return self.dist.sample()

    def mode(self):
        return self.dist.logits.argmax(-1)

    def log_prob(self, actions):
        return self.dist.log_prob(actions)

    def entropy(self, actions=None):
        return self.dist.entropy()


class _Factored:
    """A drop chosen in three steps: hold or not, then the rotation, then the column.

    The 80 drop actions are hold x 40 + rotation x 10 + column (the game's
    README). Instead of one softmax over all 80, the actor outputs 2 + 4 + 10
    numbers: scores for hold, for each rotation and for each column. Each
    step is masked to what is still legal given the steps before (a rotation
    with no reachable column can't be picked), and its probability is the
    product of the three. The column scores are shared by every rotation, so
    what is learned about a column ("the well is in column 9") carries over,
    and each choice is among 2, 4 or 10 rather than 80.
    """

    ACTIONS, OUTPUTS = 80, 2 + 4 + 10

    def __init__(self, outputs: torch.Tensor, mask: torch.Tensor):
        self.hold_logits, self.rot_logits, self.col_logits = outputs.split([2, 4, 10], dim=-1)
        self.mask = mask.view(-1, 2, 4, 10)
        self.rows = torch.arange(len(mask), device=mask.device)

    def _hold(self):
        legal = self.mask.any(-1).any(-1)
        return torch.distributions.Categorical(logits=self.hold_logits.masked_fill(~legal, -1e9))

    def _rot(self, hold):
        legal = self.mask[self.rows, hold].any(-1)
        return torch.distributions.Categorical(logits=self.rot_logits.masked_fill(~legal, -1e9))

    def _col(self, hold, rot):
        legal = self.mask[self.rows, hold, rot]
        return torch.distributions.Categorical(logits=self.col_logits.masked_fill(~legal, -1e9))

    def _choose(self, pick):
        hold = pick(self._hold())
        rot = pick(self._rot(hold))
        col = pick(self._col(hold, rot))
        return hold * 40 + rot * 10 + col

    def sample(self):
        return self._choose(lambda d: d.sample())

    def mode(self):
        return self._choose(lambda d: d.logits.argmax(-1))

    def _steps(self, actions):
        hold, rot, col = actions // 40, actions % 40 // 10, actions % 10
        return (self._hold(), hold), (self._rot(hold), rot), (self._col(hold, rot), col)

    def log_prob(self, actions):
        return sum(d.log_prob(a) for d, a in self._steps(actions))

    def entropy(self, actions):
        # The three steps' entropies along the path actually taken: the
        # exact entropy over all 80 would need every branch.
        return sum(d.entropy() for d, _ in self._steps(actions))


def _mlp(inputs: int, hidden: int, outputs: int, last_gain: float) -> nn.Sequential:
    layers = [nn.Linear(inputs, hidden), nn.Tanh(), nn.Linear(hidden, hidden), nn.Tanh(), nn.Linear(hidden, outputs)]
    for i, layer in enumerate(l for l in layers if isinstance(l, nn.Linear)):
        nn.init.orthogonal_(layer.weight, last_gain if i == 2 else np.sqrt(2))
        nn.init.zeros_(layer.bias)
    return nn.Sequential(*layers)


class _RunningNorm:
    """Keeps a running mean and variance of every input, and rescales inputs
    by them: a score in the thousands and a one-hot cell end up on the same
    scale, without anyone saying how big a score gets."""

    def __init__(self, size: int):
        self.size = size
        self.mean, self.var, self.count = np.zeros(size), np.ones(size), 1e-4

    def update(self, batch: np.ndarray) -> None:
        # Chan et al.'s parallel update: merge the batch's mean and variance in.
        mean, var, n = batch.mean(axis=0), batch.var(axis=0), len(batch)
        delta, total = mean - self.mean, self.count + n
        self.mean = self.mean + delta * n / total
        self.var = (self.var * self.count + var * n + delta**2 * self.count * n / total) / total
        self.count = total

    def apply(self, x: np.ndarray) -> np.ndarray:
        return np.clip((x - self.mean) / np.sqrt(self.var + 1e-8), -10, 10).astype(np.float32)

    def state(self) -> dict:
        return {"mean": self.mean, "var": self.var, "count": self.count}

    def load(self, state: dict) -> None:
        self.mean, self.var, self.count = state["mean"], state["var"], state["count"]
