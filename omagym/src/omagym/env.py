"""A game as an agent plays it: reset, step, observe, mask.

The shape follows Gymnasium (reset/step, terminated vs truncated) without
depending on it. Observations are a dict of numpy arrays named and shaped by
the game's own spec, so nothing here knows Tetris from Snake.
"""

from __future__ import annotations

from dataclasses import dataclass, field

import numpy as np

from . import native


@dataclass(frozen=True)
class Tensor:
    name: str
    dtype: np.dtype
    shape: tuple[int, ...]
    offset: int
    labels: tuple[str, ...] = ()

    def column(self, label: str) -> int:
        """Index of a named entry along the last axis ("lines" in candidates)."""
        return self.labels.index(label)


@dataclass(frozen=True)
class EnvSpec:
    game: str
    rules_version: int
    config: dict
    tensors: dict[str, Tensor]
    size: int
    action_count: int
    action_labels: tuple[str, ...]
    signals: tuple[str, ...]


@dataclass
class Step:
    obs: dict[str, np.ndarray]
    reward: float
    terminated: bool
    truncated: bool
    ticks: int
    signals: dict[str, float] = field(default_factory=dict)

    @property
    def done(self) -> bool:
        return self.terminated or self.truncated


def _spec(game_spec: dict, env_spec: dict) -> EnvSpec:
    observation = env_spec["observation"]
    tensors = {
        t["name"]: Tensor(t["name"], np.dtype(t["dtype"]), tuple(t["shape"]), t["offset"], tuple(t.get("labels", ())))
        for t in observation["tensors"]
    }
    return EnvSpec(
        game=game_spec["game"],
        rules_version=game_spec["rules_version"],
        config=env_spec["config"],
        tensors=tensors,
        size=observation["size"],
        action_count=env_spec["actions"]["count"],
        action_labels=tuple(env_spec["actions"]["labels"]),
        signals=tuple(env_spec["signals"]),
    )


class Env:
    """One game in play. `config` keys are the game's (see its README); unknown keys fail."""

    def __init__(self, game: str, _handle: int | None = None, **config):
        # Set before anything can fail, so __del__ has something to look at.
        self._handle = 0
        self._lib = native.load(game)
        self._handle = _handle if _handle is not None else self._lib.create(config)
        self.spec = _spec(self._lib.game_spec, self._lib.env_spec(self._handle))
        self._buffer = np.zeros(self.spec.size, np.uint8)
        self._mask = np.zeros(self.spec.action_count, np.uint8)
        self._result = native.StepResult()

    @property
    def game(self) -> str:
        return self.spec.game

    def reset(self, seed: int) -> dict[str, np.ndarray]:
        self._lib.reset(self._handle, seed)
        return self.observe()

    def step(self, action: int) -> Step:
        self._lib.step(self._handle, int(action), self._result)
        r = self._result
        return Step(
            obs=self.observe(),
            reward=r.reward,
            terminated=bool(r.terminated),
            truncated=bool(r.truncated),
            ticks=r.ticks,
            signals={name: r.signal_values[i] for i, name in enumerate(self.spec.signals)},
        )

    def observe(self) -> dict[str, np.ndarray]:
        """A copy, so an agent may keep it: the next step writes a fresh one."""
        self._lib.observe(self._handle, self._buffer.ctypes.data)
        data = self._buffer.copy()
        return {
            t.name: np.frombuffer(data, t.dtype, int(np.prod(t.shape)), t.offset).reshape(t.shape)
            for t in self.spec.tensors.values()
        }

    def mask(self) -> np.ndarray:
        """True for every action step() would accept now."""
        self._lib.action_mask(self._handle, self._mask.ctypes.data)
        return self._mask.astype(bool)

    def replay(self) -> dict:
        """The episode so far as replay/v1, which `bin/run <game> --replay` plays."""
        return self._lib.replay(self._handle)

    def info(self) -> dict:
        return self._lib.info(self._handle)

    def clone(self, reseed_hidden: bool = False, seed: int = 0) -> Env:
        """A copy that plays on independently; reseeded, it cannot see the future."""
        return Env(self.game, _handle=self._lib.clone(self._handle, reseed_hidden, seed))

    def close(self) -> None:
        if self._handle:
            self._lib.destroy(self._handle)
            self._handle = 0

    def __del__(self):
        self.close()

    def __enter__(self) -> Env:
        return self

    def __exit__(self, *exc) -> None:
        self.close()
