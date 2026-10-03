"""Any legal action, uniformly: the floor every other agent has to clear."""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np

from . import register
from .base import Agent


@register
class RandomAgent(Agent):
    name = "random"
    description = "Any legal action, uniformly at random. The floor every agent should clear."

    @dataclass
    class Config:
        seed: int = 0

    @classmethod
    def env_config(cls, game: str) -> dict:
        # Random landings rather than random keys, so the floor is measured
        # in the same action space the other Tetris agents use.
        return {"actions": "placement"} if game == "omatris" else {}

    def __init__(self, config: Config, spec, device: str = "cpu"):
        super().__init__(config, spec, device)
        self.rng = np.random.default_rng(config.seed)

    def act(self, obs, mask, explore=False) -> int:
        return int(self.rng.choice(np.flatnonzero(mask)))
