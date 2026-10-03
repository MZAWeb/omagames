"""What every agent is, and what a learning agent is given to learn with."""

from __future__ import annotations

import dataclasses
from collections.abc import Iterator
from dataclasses import dataclass
from pathlib import Path
from typing import TYPE_CHECKING, ClassVar

import numpy as np

from ..env import EnvSpec

if TYPE_CHECKING:
    from ..env import Env
    from ..training import TrainContext


class Agent:
    """Something that plays a game: given what it sees and what it may do, an action.

    A subclass sets `name` (what the command line calls it), `games` (empty
    for any game), a `Config` dataclass of everything tunable, and `act()`.
    One that learns also sets `trainable` and implements `train()`, `save()`
    and `load()`. Decorate it with `@register` and import its module in
    `agents/__init__.py`.
    """

    name: ClassVar[str]
    games: ClassVar[tuple[str, ...]] = ()
    description: ClassVar[str] = ""
    trainable: ClassVar[bool] = False

    @dataclass
    class Config:
        pass

    @classmethod
    def env_config(cls, game: str) -> dict:
        """The env config this agent plays `game` with (its action space, say)."""
        return {}

    def __init__(self, config: Config, spec: EnvSpec, device: str = "cpu"):
        self.config = config
        self.spec = spec
        self.device = device

    def act(self, obs: dict[str, np.ndarray], mask: np.ndarray, explore: bool = False) -> int:
        """The action to take. `explore` is True while training, False when evaluated."""
        raise NotImplementedError

    def decide(self, env: Env, obs: dict[str, np.ndarray], mask: np.ndarray, explore: bool = False) -> int:
        """What the framework calls to get an action: act(), unless the agent plans.

        A planning agent (lookahead, mcts) overrides this to try moves out
        on copies of the game, `env.clone(reseed_hidden=True)`, before
        choosing. The reseed keeps it honest: a copy deals its own pieces
        beyond the ones a player can see, so a search can't peek at the real
        ones. Every other agent decides from what it sees alone.
        """
        return self.act(obs, mask, explore)

    def train(self, ctx: TrainContext) -> Iterator[dict[str, float]]:
        """Learns, forever, yielding progress as {"steps": n, ...} every so often.

        The framework decides when to stop (it stops pulling from the
        generator), evaluates between yields and checkpoints the best. Every
        yield must carry "steps", the env steps taken so far.
        """
        raise TypeError(f"{self.name} does not learn")

    def save(self, path: Path) -> None:
        """Writes what was learned. Agents that learn nothing write nothing."""

    def load(self, path: Path) -> None:
        """Reads back what save() wrote."""


def make_config(cls: type[Agent], overrides: dict[str, str]) -> Agent.Config:
    """The agent's Config with `key=value` overrides from the command line, typed."""
    fields = {f.name: f for f in dataclasses.fields(cls.Config)}
    values = {}
    for key, raw in overrides.items():
        if key not in fields:
            known = ", ".join(fields) or "none"
            raise ValueError(f"{cls.name} has no setting {key!r}; it has: {known}")
        values[key] = _parse(raw, fields[key].type)
    return cls.Config(**values)


def _parse(raw: str, kind) -> object:
    kind = kind if isinstance(kind, str) else getattr(kind, "__name__", str(kind))
    if kind == "bool":
        if raw.lower() not in ("true", "false", "1", "0", "yes", "no"):
            raise ValueError(f"{raw!r} is not true or false")
        return raw.lower() in ("true", "1", "yes")
    if kind == "int":
        return int(raw)
    if kind == "float":
        return float(raw)
    return raw
