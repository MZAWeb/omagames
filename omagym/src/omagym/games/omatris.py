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


# -- Richer features -----------------------------------------------------------
#
# The five above are enough to survive. To score well an agent has to tell a
# Tetris from four singles and a well kept open from one filled in, which
# takes more. These are the features the strong hand-made and evolved Tetris
# controllers use (Dellacherie's, and the ones Thiery and Scherrer added),
# split in two groups because a planner treats them differently:
#
# - EARNED: what a placement scored, once made. Along a line of play these
#   add up: two Tetrises are worth twice one.
# - BOARD: what the board is like afterwards. Only the board at the end of a
#   line of play matters; the boards on the way there are gone.
#
# A one-step agent (cem) just uses both groups of its one landing.

EARNED = ("clear1", "clear2", "clear3", "clear4", "spin_mini", "spin_full", "topped_out")
BOARD = ("holes", "bumpiness", "height", "max_height", "wells", "deepest_well", "row_transitions",
         "column_transitions")
RICH = EARNED + BOARD

# Roughly how big each feature gets on a busy board, so that a weight of 1
# means about the same for every feature. Without this, a learner adjusting
# weights would have to discover that "height" (sums to hundreds) needs a
# weight a hundred times smaller than "clear4" (0 or 1): scaling the inputs
# removes that busywork.
RICH_SCALE = np.array([1, 1, 1, 1, 1, 1, 1,                    # earned: 0 or 1 already
                       10, 30, 60, 20, 20, 10, 40, 30], np.float32)


def rich_features(obs: dict[str, np.ndarray], spec: EnvSpec, count: int) -> np.ndarray:
    """[count, len(RICH)] floats, one row per landing on offer, unscaled.

    Every column is computed for all landings at once with numpy, from the
    afterstates the env provides (1 = filled, row 0 at the top).
    """
    after = obs["afterstates"][:count].astype(np.int32)       # [n, rows, cols]
    n, rows, cols = after.shape
    column = spec.tensors["candidates"].column
    landing = obs["candidates"][:count]

    # What the placement earned: which kind of clear (one-hot, so each kind
    # gets its own weight), whether it was a T-spin, whether it topped out.
    lines = landing[:, column("lines")]
    clears = np.stack([lines == k for k in (1, 2, 3, 4)], axis=1)
    spin = landing[:, column("spin")]
    earned = np.concatenate([clears, np.stack([spin == 1, spin == 2, landing[:, column("topped_out")] > 0], axis=1)],
                            axis=1)

    # Column heights: from the floor up to the highest filled cell.
    filled_anywhere = after.any(axis=1)                        # [n, cols]
    heights = np.where(filled_anywhere, rows - after.argmax(axis=1), 0)

    # A hole is an empty cell with something filled above it in its column.
    covered = np.maximum.accumulate(after, axis=1)
    holes = (covered & (after == 0)).sum(axis=(1, 2))

    bumpiness = np.abs(np.diff(heights, axis=1)).sum(axis=1)

    # A well is a column lower than both its neighbours (the walls count as
    # infinitely high). One deep well is where a Tetris goes; several are a
    # mess. So both their total depth and the deepest one are features.
    padded = np.pad(heights, ((0, 0), (1, 1)), constant_values=rows)
    neighbours = np.minimum(padded[:, :-2], padded[:, 2:])
    well_depths = np.maximum(neighbours - heights, 0)

    # Transitions: how often, reading along a row (or down a column), a filled
    # cell meets an empty one. A tidy board has few; a ragged one, many. The
    # walls and the floor count as filled.
    wall = np.ones((n, rows, 1), np.int32)
    by_row = np.concatenate([wall, after, wall], axis=2)
    row_transitions = (np.diff(by_row, axis=2) != 0).sum(axis=(1, 2))
    floor = np.ones((n, 1, cols), np.int32)
    by_column = np.concatenate([after, floor], axis=1)
    column_transitions = (np.diff(by_column, axis=1) != 0).sum(axis=(1, 2))

    board = np.stack([holes, bumpiness, heights.sum(axis=1), heights.max(axis=1), well_depths.sum(axis=1),
                      well_depths.max(axis=1), row_transitions, column_transitions], axis=1)
    return np.concatenate([earned, board], axis=1).astype(np.float32)
