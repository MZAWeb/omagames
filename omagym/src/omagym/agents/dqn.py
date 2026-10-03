"""A first neural agent: deep Q-learning on Tetris afterstates.

Rather than score actions, it scores the board each landing would leave
behind (the afterstate): V(board) is "how much reward is still to come from
here". Picking a move is then just taking the landing whose afterstate scores
best, which is why this works with a placement action space whose size
changes every piece.

Learning is temporal difference, as in DQN: after landing on afterstate s,
getting reward r and then choosing afterstate s', the target for V(s) is
r + gamma * V_target(s'). A replay buffer of past (s, r, s', done) breaks up
correlated steps, and a target network, synced every so often, keeps the
target from chasing itself.

It is deliberately small. Every knob is in Config, and `inputs = "board"`
swaps the five hand-made features for the raw 240 cells, which is slower to
learn but a real test of the network rather than of the features.
"""

from __future__ import annotations

import copy
from dataclasses import dataclass
from pathlib import Path

import numpy as np
import torch
from torch import nn

from ..games import omatris
from . import register
from .base import Agent

# Rough sizes of each feature on a busy board, so the network sees inputs
# around 0..1 whatever their units.
_FEATURE_SCALE = np.array([4.0, 40.0, 60.0, 200.0, 1.0], np.float32)


@register
class AfterstateDQN(Agent):
    name = "dqn"
    games = ("omatris",)
    description = "Learns a value network over the board each landing leaves (DQN on afterstates)."
    trainable = True

    @dataclass
    class Config:
        # "features": the five numbers greedy uses; "board": every cell.
        inputs: str = "features"
        hidden: int = 64
        layers: int = 2
        lr: float = 1e-3
        gamma: float = 0.95
        # Exploration: the chance of a random landing, decaying linearly.
        epsilon_start: float = 1.0
        epsilon_end: float = 0.01
        epsilon_steps: int = 50_000
        buffer: int = 50_000
        batch: int = 512
        # Steps of play before the first update, so the buffer is not tiny.
        warmup: int = 2_000
        target_sync: int = 1_000
        # A training game is cut short here; a good agent never tops out.
        episode_steps: int = 2_000
        # The reward it learns from, built from the env's signals: a little
        # for every piece placed, a lot for lines (squared, so a Tetris is
        # worth sixteen singles), and a penalty for topping out.
        reward_piece: float = 1.0
        reward_line: float = 10.0
        reward_top_out: float = -20.0
        seed: int = 0

    @classmethod
    def env_config(cls, game: str) -> dict:
        return {"actions": "placement"}

    def __init__(self, config: Config, spec, device: str = "cpu"):
        super().__init__(config, spec, device)
        if config.inputs not in ("features", "board"):
            raise ValueError(f"inputs must be features or board, not {config.inputs!r}")
        torch.manual_seed(config.seed)
        self.rng = np.random.default_rng(config.seed)
        board = spec.tensors["afterstates"].shape[1:]
        self.input_size = len(omatris.FEATURES) if config.inputs == "features" else int(np.prod(board))
        self.net = self._network().to(device)
        self.target = copy.deepcopy(self.net)
        self.optimizer = torch.optim.Adam(self.net.parameters(), lr=config.lr)
        self.steps = 0

    def _network(self) -> nn.Module:
        layers, width = [], self.input_size
        for _ in range(self.config.layers):
            layers += [nn.Linear(width, self.config.hidden), nn.ReLU()]
            width = self.config.hidden
        layers.append(nn.Linear(width, 1))
        return nn.Sequential(*layers)

    def _inputs(self, obs, count: int) -> np.ndarray:
        if self.config.inputs == "features":
            return omatris.afterstate_features(obs, self.spec, count) / _FEATURE_SCALE
        return obs["afterstates"][:count].reshape(count, -1).astype(np.float32)

    def _values(self, inputs: np.ndarray) -> np.ndarray:
        with torch.no_grad():
            x = torch.as_tensor(inputs, device=self.device)
            return self.net(x).squeeze(1).cpu().numpy()

    @property
    def epsilon(self) -> float:
        c = self.config
        fraction = min(1.0, self.steps / max(1, c.epsilon_steps))
        return c.epsilon_start + fraction * (c.epsilon_end - c.epsilon_start)

    def _choose(self, inputs: np.ndarray, explore: bool) -> int:
        if explore and self.rng.random() < self.epsilon:
            return int(self.rng.integers(len(inputs)))
        return int(np.argmax(self._values(inputs)))

    def act(self, obs, mask, explore=False) -> int:
        return self._choose(self._inputs(obs, omatris.landing_count(mask)), explore)

    def _reward(self, signals: dict[str, float]) -> float:
        c = self.config
        reward = c.reward_piece + c.reward_line * signals["lines"] ** 2
        return reward + (c.reward_top_out if signals["topped_out"] else 0.0)

    def train(self, ctx):
        c = self.config
        memory = _Memory(c.buffer, self.input_size)
        env = ctx.make_env(max_steps=c.episode_steps)
        losses: list[float] = []
        obs, mask = env.reset(ctx.next_seed()), env.mask()
        # The afterstate chosen last step and its reward, waiting for the
        # next choice to complete the transition.
        pending: tuple[np.ndarray, float] | None = None
        episode_reward = 0.0
        while True:
            inputs = self._inputs(obs, omatris.landing_count(mask))
            action = self._choose(inputs, explore=True)
            chosen = inputs[action]
            if pending is not None:
                memory.add(pending[0], pending[1], False, chosen)
            step = env.step(action)
            self.steps += 1
            reward = self._reward(step.signals)
            episode_reward += reward

            if step.terminated:
                memory.add(chosen, reward, True, np.zeros_like(chosen))
            if step.done:
                # A truncated game still had a future; its last transition is
                # dropped rather than taught as an ending.
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
                pending, episode_reward = None, 0.0
                obs = env.reset(ctx.next_seed())
            else:
                pending = (chosen, reward)
                obs = step.obs
            mask = env.mask()

            if len(memory) >= c.warmup:
                losses.append(self._learn(memory))
            if self.steps % c.target_sync == 0:
                self.target.load_state_dict(self.net.state_dict())

    def _learn(self, memory: _Memory) -> float:
        c = self.config
        states, rewards, dones, nexts = memory.sample(self.rng, c.batch, self.device)
        with torch.no_grad():
            target = rewards + c.gamma * (1.0 - dones) * self.target(nexts).squeeze(1)
        loss = nn.functional.mse_loss(self.net(states).squeeze(1), target)
        self.optimizer.zero_grad()
        loss.backward()
        self.optimizer.step()
        return loss.item()

    def save(self, path: Path) -> None:
        torch.save({"net": self.net.state_dict(), "steps": self.steps}, path)

    def load(self, path: Path) -> None:
        state = torch.load(path, map_location=self.device)
        self.net.load_state_dict(state["net"])
        self.target.load_state_dict(state["net"])
        self.steps = state["steps"]


class _Memory:
    """A ring buffer of (afterstate, reward, done, next afterstate)."""

    def __init__(self, capacity: int, width: int):
        self.states = np.zeros((capacity, width), np.float32)
        self.nexts = np.zeros((capacity, width), np.float32)
        self.rewards = np.zeros(capacity, np.float32)
        self.dones = np.zeros(capacity, np.float32)
        self.capacity, self.size, self.cursor = capacity, 0, 0

    def __len__(self) -> int:
        return self.size

    def add(self, state, reward, done, next_state) -> None:
        i = self.cursor
        self.states[i], self.rewards[i], self.dones[i], self.nexts[i] = state, reward, done, next_state
        self.cursor = (i + 1) % self.capacity
        self.size = min(self.size + 1, self.capacity)

    def sample(self, rng, batch: int, device: str):
        rows = rng.integers(self.size, size=min(batch, self.size))
        return tuple(
            torch.as_tensor(a[rows], device=device) for a in (self.states, self.rewards, self.dones, self.nexts)
        )
