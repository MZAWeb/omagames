"""Omatris helpers shared by its agents: the landings on offer and the
classic hand-made features of the board each would leave behind."""

from __future__ import annotations

import numpy as np

from ..env import EnvSpec

# The columns of afterstate_features(), in order.
FEATURES = ("lines", "holes", "bumpiness", "height", "topped_out")


def landing_count(mask: np.ndarray) -> int:
    """The placement space offers its landings in its first slots."""
    return int(mask.sum())


def afterstate_features(obs: dict[str, np.ndarray], spec: EnvSpec, count: int) -> np.ndarray:
    """[count, len(FEATURES)] floats, one row per landing on offer.

    Holes are empty cells with a filled cell above them, bumpiness the height
    differences between neighbouring columns, height the columns' heights
    summed. Computed for all landings at once, from the afterstates the env
    already provides.
    """
    after = obs["afterstates"][:count].astype(np.int32)  # [n, rows, cols], 1 = filled
    rows = after.shape[1]
    filled = after.any(axis=1)
    heights = np.where(filled, rows - after.argmax(axis=1), 0)
    covered = np.maximum.accumulate(after, axis=1)
    holes = (covered & (after == 0)).sum(axis=(1, 2))
    bumpiness = np.abs(np.diff(heights, axis=1)).sum(axis=1)
    candidates = spec.tensors["candidates"]
    rows_on_offer = obs["candidates"][:count]
    lines = rows_on_offer[:, candidates.column("lines")]
    topped_out = rows_on_offer[:, candidates.column("topped_out")]
    return np.stack([lines, holes, bumpiness, heights.sum(axis=1), topped_out], axis=1).astype(np.float32)
