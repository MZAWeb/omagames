"""The "dumb" strategies: one look ahead, no learning, hand-picked rules.

They are the bar a learner has to clear, and the quickest way to check the
whole pipeline (env, evaluation, experiment store, replays) works.
"""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np

from ..games import omasnake, omatris
from . import register
from .base import Agent


@register
class TetrisGreedy(Agent):
    """Scores the board each landing would leave with four weighted features
    and takes the best. The weights are the ones Yiyuan Lee's well-known
    Tetris bot evolved with a genetic algorithm; every one is a setting, so
    `--set holes=-0.5` tries another."""

    name = "greedy"
    games = ("omatris",)
    description = "Weighted board features of each landing's afterstate; takes the best. No learning."

    @dataclass
    class Config:
        lines: float = 0.760666
        holes: float = -0.35663
        bumpiness: float = -0.184483
        height: float = -0.510066
        topped_out: float = -100.0

    @classmethod
    def env_config(cls, game: str) -> dict:
        return {"actions": "placement"}

    def act(self, obs, mask, explore=False) -> int:
        count = omatris.landing_count(mask)
        features = omatris.afterstate_features(obs, self.spec, count)
        weights = np.array([getattr(self.config, f) for f in omatris.FEATURES], np.float32)
        return int(np.argmax(features @ weights))


@register
class SnakeGreedy(Agent):
    """Heads for the food by the shortest straight-line distance, never onto
    a wall or its own body if it can help it. It has no idea it can box
    itself in, which is how it dies."""

    name = "greedy"
    games = ("omasnake",)
    description = "Steps toward the food, avoiding walls and its body one move ahead. No learning."

    @dataclass
    class Config:
        pass

    @classmethod
    def env_config(cls, game: str) -> dict:
        return {"actions": "absolute"}

    def act(self, obs, mask, explore=False) -> int:
        s = omasnake.state(obs, self.spec)
        best, best_distance = None, None
        for action in np.flatnonzero(mask):
            if not omasnake.lands_safely(obs, self.spec, int(action)):
                continue
            dx, dy = omasnake.MOVES[action]
            distance = abs(s["head_x"] + dx - s["food_x"]) + abs(s["head_y"] + dy - s["food_y"])
            if best is None or distance < best_distance:
                best, best_distance = int(action), distance
        # Boxed in: every move dies, so any legal one will do.
        return best if best is not None else int(np.flatnonzero(mask)[0])
