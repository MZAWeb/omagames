"""Deep Q-learning on Tetris afterstates: a network that learns how good a board is.

**The idea.** The agent keeps a network V that rates boards (see
afterstate_value.py). To move, it rates the board every landing would leave
and takes the best. To learn, it plays, and nudges V toward what each board
turned out to be worth.

"Turned out to be worth" is temporal-difference learning (README, Science 2.3).
After landing on board s, collecting reward r, and reaching board s' next,
a better guess for V(s) than the current one is

    target = r + gamma * V(s')

(what the move earned, plus the discounted value of where it led). The
network is trained to move V(s) toward that target. Repeat for millions of
moves and V comes to predict the reward still to come from any board. gamma,
a little under 1, makes reward soon worth more than reward later.

**What keeps it stable.** Two tricks from DQN (Mnih et al., 2015):

- A **replay memory**: every transition (s, r, s') is stored and the network
  learns from random batches of old ones, not the move just made. Moves in
  a row are alike, and learning from them in order would drag the network
  one way, then another.
- A **target network**: the V(s') in the target comes from a frozen copy of
  the network, refreshed every `target_sync` steps. Otherwise every update
  would also move the target it is chasing.

**Ideas to try** (README, Science 2.3, has more): learning from the next few
rewards before guessing the rest (n-step returns); bootstrapping from the
board the network rates best rather than the one played (Q-learning, and
Double DQN to keep it honest); the game's own points as the reward; and
`inputs=rich` or `board` for what the network sees, and `rich_hand` to
show it the held and next pieces too.

**Read next:** mcts.py, which plans with a network like this one.
"""

from __future__ import annotations

import copy
from dataclasses import dataclass
from pathlib import Path

import numpy as np
import torch
from torch import nn

from ..games import omatris
from . import afterstate_value, register
from .base import Agent


@register
class AfterstateDQN(Agent):
    name = "dqn"
    games = ("omatris",)
    description = "Learns a value network over the board each landing leaves (DQN on afterstates)."
    trainable = True

    @dataclass
    class Config:
        # What the network sees, and how big it is (afterstate_value.py).
        inputs: str = "features"
        hidden: int = 64
        layers: int = 2
        # Learning rate: how far each update moves the network. Too high and
        # it never settles; too low and it takes forever.
        lr: float = 1e-3
        # The discount: 0.95 values a reward 20 moves away at about a third.
        gamma: float = 0.95
        # Exploration: the chance of a random landing, from `epsilon_start`
        # down to `epsilon_end` over the first `epsilon_steps` moves. Early on
        # it knows nothing, so trying things is all it can do.
        epsilon_start: float = 1.0
        epsilon_end: float = 0.01
        epsilon_steps: int = 50_000
        # The replay memory's size, and the batch each update learns from.
        buffer: int = 50_000
        batch: int = 512
        # Moves played before the first update, so the first batches aren't
        # the same handful of moves over and over.
        warmup: int = 2_000
        target_sync: int = 1_000
        # A training game is cut short here; a good agent never tops out.
        episode_steps: int = 2_000
        # The reward it learns from, a sum of terms, each a setting: what the
        # agent is paid for is what it learns to do (see _reward()). The
        # defaults are the original three; the others start at 0.
        reward_piece: float = 1.0       # per piece placed: pays for playing long
        reward_line: float = 10.0       # times lines squared: a Tetris is worth sixteen singles
        reward_top_out: float = -20.0   # once, for losing
        reward_win: float = 0.0         # once, for winning: a Challenge cleared (try 100)
        reward_dealt_row: float = 0.0   # per dealt row of a Challenge cleared: digging (try 10)
        reward_tspin: float = 0.0       # per T-spin
        # Shaping, potential-based so it can't change what the best play is
        # (README, Science section 3): paid for the *change* in holes and in the
        # stack's height after each piece, so a hole is felt when it is made.
        shaping_holes: float = 0.0      # try 1: -1 for every hole made, +1 for every one opened
        shaping_height: float = 0.0     # try 0.5, per row the tallest column grows
        seed: int = 0

    @classmethod
    def env_config(cls, game: str) -> dict:
        return {"actions": "placement"}

    def __init__(self, config: Config, spec, device: str = "cpu"):
        super().__init__(config, spec, device)
        afterstate_value.check(config.inputs)
        torch.manual_seed(config.seed)
        self.rng = np.random.default_rng(config.seed)
        # The board potential before the next move, for shaping; none at a game's start.
        self._shape_before: float | None = None
        self.input_size = afterstate_value.input_size(config.inputs, spec)
        self.net = afterstate_value.network(config.inputs, spec, config.hidden, config.layers).to(device)
        self.target = copy.deepcopy(self.net)
        self.optimizer = torch.optim.Adam(self.net.parameters(), lr=config.lr)
        self.steps = 0

    # -- playing --------------------------------------------------------------

    def _inputs(self, obs, mask) -> np.ndarray:
        """Every landing on offer, as the network sees it."""
        return afterstate_value.inputs_for(self.config.inputs, obs, self.spec, omatris.landing_count(mask))

    def values(self, inputs: np.ndarray, network: nn.Module | None = None) -> np.ndarray:
        """The network's rating of each board (no learning happens here)."""
        with torch.no_grad():
            x = torch.as_tensor(inputs, device=self.device)
            return (network or self.net)(x).squeeze(1).cpu().numpy()

    @property
    def epsilon(self) -> float:
        c = self.config
        fraction = min(1.0, self.steps / max(1, c.epsilon_steps))
        return c.epsilon_start + fraction * (c.epsilon_end - c.epsilon_start)

    def act(self, obs, mask, explore=False) -> int:
        inputs = self._inputs(obs, mask)
        if explore and self.rng.random() < self.epsilon:
            return int(self.rng.integers(len(inputs)))
        return int(np.argmax(self.values(inputs)))

    # -- learning -------------------------------------------------------------

    def _reward(self, step, before: dict | None = None) -> float:
        """What the move just made earned, from the env's signals.

        `before` is the observation the move was made from; without it (a
        planner asking) the terms that compare before and after are left out.
        """
        c, s = self.config, step.signals
        topped_out = s["topped_out"] > 0
        # Each move places one piece in the placement space.
        reward = c.reward_piece + c.reward_line * s["lines"] ** 2 + c.reward_tspin * (s["tspin"] > 0)
        if topped_out:
            reward += c.reward_top_out
        elif step.terminated:
            reward += c.reward_win   # it ended without topping out: the goal was reached
        if before is not None:
            stats = self.spec.tensors["stats"].column("dealt_rows_left")
            reward += c.reward_dealt_row * float(before["stats"][stats] - step.obs["stats"][stats])
            # Potential-based shaping: Phi(board) = -(holes * a + height * b);
            # the reward is gamma * Phi(after) - Phi(before).
            if self._shape_before is not None:
                after = -(c.shaping_holes * s["holes"] + c.shaping_height * s["max_height"])
                reward += c.gamma * after - self._shape_before
        self._shape_before = None if step.done else -(c.shaping_holes * s["holes"] + c.shaping_height * s["max_height"])
        return reward

    def train(self, ctx):
        c = self.config
        memory = _Memory(c.buffer, self.input_size)
        env = ctx.make_env(max_steps=c.episode_steps)
        # The board chosen last move and the reward it earned, waiting for the
        # next choice to complete its transition (s, r, s').
        pending: tuple[np.ndarray, float] | None = None
        losses: list[float] = []
        episode_reward = 0.0
        obs = env.reset(ctx.next_seed())
        while True:
            mask = env.mask()
            inputs = self._inputs(obs, mask)
            action = self.act(obs, mask, explore=True)
            chosen = inputs[action]
            if pending is not None:
                memory.add(pending[0], pending[1], c.gamma, chosen)

            step = env.step(action)
            self.steps += 1
            reward = self._reward(step, obs)
            episode_reward += reward

            if step.terminated:
                # The game is over: nothing comes after, so the target is just
                # the reward (discount 0).
                memory.add(chosen, reward, 0.0, np.zeros_like(chosen))
            if step.done:
                # A game cut short (truncated) still had a future we never saw;
                # its last transition is dropped rather than taught as an ending.
                pending = None
                info = env.info()
                yield {
                    "steps": self.steps,
                    "episode_score": info["score"],
                    "episode_lines": info["lines"],
                    "episode_reward": episode_reward,
                    "epsilon": self.epsilon,
                    "loss": float(np.mean(losses)) if losses else float("nan"),
                }
                losses.clear()
                episode_reward = 0.0
                obs = env.reset(ctx.next_seed())
            else:
                pending = (chosen, reward)
                obs = step.obs

            if len(memory) >= c.warmup:
                losses.append(self._learn(memory))
            if self.steps % c.target_sync == 0:
                self.target.load_state_dict(self.net.state_dict())

    def _learn(self, memory: _Memory) -> float:
        """One update: a random batch of old transitions, a step toward their targets."""
        c = self.config
        states, rewards, discounts, follows = memory.sample(self.rng, c.batch, self.device)
        with torch.no_grad():
            target = rewards + discounts * self.target(follows).squeeze(1)
        # Mean squared error between the ratings now and the targets.
        loss = nn.functional.mse_loss(self.net(states).squeeze(1), target)
        self.optimizer.zero_grad()
        loss.backward()
        self.optimizer.step()
        return loss.item()

    # -- keeping what was learned ---------------------------------------------

    def save(self, path: Path) -> None:
        torch.save({"net": self.net.state_dict(), "steps": self.steps}, path)

    def load(self, path: Path) -> None:
        state = torch.load(path, map_location=self.device)
        self.net.load_state_dict(state["net"])
        self.target.load_state_dict(state["net"])
        self.steps = state["steps"]


class _Memory:
    """The replay memory: a ring of (board, reward, discount, next board).

    `discount` is gamma, or 0 when the game ended, so the target is always
    reward + discount * V(next board).
    """

    def __init__(self, capacity: int, width: int):
        self.states = np.zeros((capacity, width), np.float32)
        self.follows = np.zeros((capacity, width), np.float32)
        self.rewards = np.zeros(capacity, np.float32)
        self.discounts = np.zeros(capacity, np.float32)
        self.capacity, self.size, self.cursor = capacity, 0, 0

    def __len__(self) -> int:
        return self.size

    def add(self, state, reward, discount, follow) -> None:
        # Once full, the oldest transition is overwritten: the memory holds the
        # most recent `capacity`, from an agent much like the current one.
        i = self.cursor
        self.states[i], self.rewards[i], self.discounts[i], self.follows[i] = state, reward, discount, follow
        self.cursor = (i + 1) % self.capacity
        self.size = min(self.size + 1, self.capacity)

    def sample(self, rng, batch: int, device: str):
        rows = rng.integers(self.size, size=min(batch, self.size))
        return tuple(torch.as_tensor(a[rows], device=device)
                     for a in (self.states, self.rewards, self.discounts, self.follows))
