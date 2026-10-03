"""Omasnake helpers shared by its agents."""

from __future__ import annotations

import numpy as np

from ..env import EnvSpec

# Grid codes, as the env numbers them.
EMPTY, BODY, HEAD, TAIL, FOOD, BONUS = range(6)
# (dx, dy) per absolute action: up, down, left, right.
MOVES = ((0, -1), (0, 1), (-1, 0), (1, 0))


def state(obs: dict[str, np.ndarray], spec: EnvSpec) -> dict[str, int]:
    """The labelled state row as a dict: head_x, food_y, heading, ..."""
    labels = spec.tensors["state"].labels
    return {label: int(v) for label, v in zip(labels, obs["state"])}


def lands_safely(obs: dict[str, np.ndarray], spec: EnvSpec, action: int) -> bool:
    """Whether an absolute move keeps the head off walls and body.

    The tail cell counts as free: it moves away on the same step. Wrap mode
    crosses the edge instead of hitting it.
    """
    s = state(obs, spec)
    grid = obs["grid"]
    height, width = grid.shape
    x, y = s["head_x"] + MOVES[action][0], s["head_y"] + MOVES[action][1]
    if spec.config.get("mode") == "wrap":
        x, y = x % width, y % height
    elif not (0 <= x < width and 0 <= y < height):
        return False
    return grid[y, x] not in (BODY, HEAD)
