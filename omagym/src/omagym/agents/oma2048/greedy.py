"""greedy for Oma2048: rate the board each slide leaves, take the best.

The same loop as Tetris's greedy (read omatris/greedy.py first): every
legal slide's afterstate, the board before the new tile spawns, is rated
with a weighted sum of hand-made features (games/oma2048.py), plus the
points the slide scores. The weights encode the strategy most good players
use: keep cells free, keep the biggest tile in a corner with the rows and
columns running down from it, keep neighbours close so they can merge.
"""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np

from ...games import oma2048
from .. import register
from ..base import Agent


@register
class Greedy2048(Agent):
    name = "greedy"
    games = ("oma2048",)
    description = "Weighted features of each slide's afterstate (free cells, corner, order, smoothness); the best. No learning."

    @dataclass
    class Config:
        empty: float = 2.7
        highest: float = 1.0
        corner: float = 3.0
        monotonicity: float = 1.0
        smoothness: float = 0.1
        merges: float = 0.7
        # Points the slide scores, per 100.
        gain: float = 1.0

    def rate(self, boards: np.ndarray) -> np.ndarray:
        """How good each board is, by the weighted features."""
        weights = np.array([getattr(self.config, f) for f in oma2048.FEATURES], np.float32)
        return oma2048.features(boards) @ weights

    def act(self, obs, mask, explore=False) -> int:
        legal, boards = oma2048.afterstates(obs, mask)
        value = self.rate(boards) + self.config.gain * obs["gains"][legal] / 100.0
        return int(legal[np.argmax(value)])
