"""The "dumb" strategies: hand-written rules, one move ahead, no learning.

They matter more than they look. Before any learner is worth believing, it
has to beat these, and they are the quickest check that the whole pipeline
(env, evaluation, experiment store, replays) works: if greedy suddenly
scores differently, something other than an agent changed.

**Read this file first.** Every Tetris agent after it (cem, dqn, lookahead,
mcts) is this same loop, "rate every landing, take the best", with a better
way of rating.
"""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np

from ..games import omasnake, omatris
from . import register
from .base import Agent


@register
class TetrisGreedy(Agent):
    """Rates the board each landing would leave and takes the best.

    **How it rates a board.** Four features, each multiplied by a weight and
    added up:

        rating = 0.76 * lines - 0.36 * holes - 0.18 * bumpiness - 0.51 * height

    - `lines`: lines this landing clears. Good, so its weight is positive.
    - `holes`: empty cells with something above them. Each needs every line
      above it cleared before it can be filled: the worst thing on a board.
    - `bumpiness`: how much neighbouring columns differ in height. A ragged
      surface has fewer places a piece fits flat.
    - `height`: all the columns' heights added up. A tall stack is close to
      topping out.
    - `topped_out`: this landing ends the game. -100 makes sure that's only
      ever chosen when every landing does.

    **Where the weights come from.** Not from us: they are the ones Yiyuan
    Lee's well-known Tetris bot found with a genetic algorithm. So even the
    "no learning" agent stands on some learning; it just happened elsewhere,
    once. `cem` does that search itself, with more features.

    **Why "greedy".** It only ever looks at the move in front of it: the best
    landing now, whatever that does to the next piece. `lookahead` fixes
    exactly that.

    Every weight is a setting, so `--set holes=-0.5` tries another.
    """

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
        # The `placement` action space offers every place the piece can come
        # to rest, with the board each would leave (the "afterstate"). The
        # env did the hard part, finding the landings; choosing is left to us.
        return {"actions": "placement"}

    def act(self, obs, mask, explore=False) -> int:
        # The landings on offer fill the first slots; the rest are masked.
        count = omatris.landing_count(mask)
        # One row per landing, one column per feature (omatris.FEATURES).
        features = omatris.afterstate_features(obs, self.spec, count)
        weights = np.array([getattr(self.config, f) for f in omatris.FEATURES], np.float32)
        # features @ weights is every landing's rating at once: each row's
        # features times the weights, added up. Take the highest.
        return int(np.argmax(features @ weights))


@register
class SnakeGreedy(Agent):
    """Heads for the food, never into a wall or its own body if it can help it.

    **How it decides.** Of the moves that don't kill it on the spot, it takes
    the one that brings its head closest to the food, measured as the
    Manhattan distance (steps across plus steps up or down, since a snake
    can't move diagonally).

    **How it dies.** It only checks one move ahead. Heading for the food, it
    happily coils into a pocket of its own body with no way out, and it
    can't know until every move left is fatal. Avoiding that means thinking
    about the space a move leaves (could I still reach my tail from there?),
    which is where a smarter Snake agent would start.
    """

    name = "greedy"
    games = ("omasnake",)
    description = "Steps toward the food, avoiding walls and its body one move ahead. No learning."

    @dataclass
    class Config:
        pass

    @classmethod
    def env_config(cls, game: str) -> dict:
        # Up, down, left, right, rather than "turn left / right / keep on":
        # absolute directions make "toward the food" a simple comparison.
        return {"actions": "absolute"}

    def act(self, obs, mask, explore=False) -> int:
        s = omasnake.state(obs, self.spec)
        best, best_distance = None, None
        # The mask already rules out reversing into the neck.
        for action in np.flatnonzero(mask):
            if not omasnake.lands_safely(obs, self.spec, int(action)):
                continue
            dx, dy = omasnake.MOVES[action]
            distance = abs(s["head_x"] + dx - s["food_x"]) + abs(s["head_y"] + dy - s["food_y"])
            if best is None or distance < best_distance:
                best, best_distance = int(action), distance
        # Boxed in: every move dies, so any legal one will do.
        return best if best is not None else int(np.flatnonzero(mask)[0])
