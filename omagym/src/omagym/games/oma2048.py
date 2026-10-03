"""Oma2048 helpers shared by its agents: the afterstates on offer, and the
classic hand-made features of a 2048 board.

The env gives, for each of the four slides, the board it leaves before the
new tile spawns (the afterstate) and the points it scores, so an agent can
rate where each move leads without playing it. Tiles are powers of two: 1
is a 2, 11 is a 2048, 0 is empty.
"""

from __future__ import annotations

import numpy as np

from ..env import EnvSpec

# The columns of features(), in order. The heuristics strong 2048 players
# (and bots) use: keep cells free, keep the big tile in a corner with the
# rows and columns running down from it, keep neighbours close in value so
# they can merge.
FEATURES = ("empty", "highest", "corner", "monotonicity", "smoothness", "merges")


def afterstates(obs: dict[str, np.ndarray], mask: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    """The legal slides, and the boards they leave: [n] actions, [n, 4, 4] boards."""
    legal = np.flatnonzero(mask)
    return legal, obs["afterstates"][legal].astype(np.int32)


def features(boards: np.ndarray) -> np.ndarray:
    """[n, len(FEATURES)] floats, one row per board ([n, 4, 4], powers of two)."""
    n = len(boards)
    empty = (boards == 0).sum(axis=(1, 2))
    highest = boards.max(axis=(1, 2))
    corners = np.stack([boards[:, 0, 0], boards[:, 0, -1], boards[:, -1, 0], boards[:, -1, -1]], axis=1)
    corner = (corners.max(axis=1) == highest).astype(np.float32)
    # Monotonicity: how far each row and column is from running steadily one
    # way (the smaller of its total rises and its total falls), negated, so
    # higher is better and 0 is perfectly ordered.
    def unordered(lines: np.ndarray) -> np.ndarray:
        steps = np.diff(lines, axis=2)
        rises = np.clip(steps, 0, None).sum(axis=2)
        falls = np.clip(-steps, 0, None).sum(axis=2)
        return np.minimum(rises, falls).sum(axis=1)
    monotonicity = -(unordered(boards) + unordered(boards.transpose(0, 2, 1)))
    # Smoothness: the differences between filled neighbours, negated.
    def rough(lines: np.ndarray) -> np.ndarray:
        a, b = lines[:, :, :-1], lines[:, :, 1:]
        both = (a > 0) & (b > 0)
        return (np.abs(a - b) * both).sum(axis=(1, 2))
    smoothness = -(rough(boards) + rough(boards.transpose(0, 2, 1)))
    # Merges available: equal filled neighbours, across and down.
    def pairs(lines: np.ndarray) -> np.ndarray:
        a, b = lines[:, :, :-1], lines[:, :, 1:]
        return ((a == b) & (a > 0)).sum(axis=(1, 2))
    merges = pairs(boards) + pairs(boards.transpose(0, 2, 1))
    return np.stack([empty, highest, corner, monotonicity, smoothness, merges], axis=1).astype(np.float32).reshape(n, -1)


def gains(obs: dict[str, np.ndarray], spec: EnvSpec, legal: np.ndarray) -> np.ndarray:
    """The points each legal slide scores."""
    return obs["gains"][legal].astype(np.float32)
