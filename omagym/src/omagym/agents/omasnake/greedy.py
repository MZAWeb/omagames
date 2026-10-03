"""greedy for Omasnake: head for the food, one move ahead, no learning."""


from __future__ import annotations

from dataclasses import dataclass

import numpy as np

from ...games import omasnake
from .. import register
from ..base import Agent


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
