"""An n-tuple network learning 2048 by temporal difference on afterstates.

**The idea** (Szubert and Jaśkowski, 2014, the classic 2048 learner). Rate a
board by looking at a few small groups of cells, *tuples*: a row of four, a
2 x 2 square. For each tuple, the tiles in its cells form a pattern (say
"8, 4, 2, empty"), and a lookup table gives that pattern a value. The board's
value is the sum over its tuples. Each tuple is also read in all 8 ways the
board can be turned and mirrored, with the same table, since a pattern in
one corner is worth the same in another. It is a huge, sparse linear model:
millions of weights, a handful of them used for any one board, so it learns
fast and plays in microseconds.

**What it learns:** the value of an *afterstate*, the board a slide leaves
before the tile spawns (README, Science 2.3), as `dqn` does for Tetris. To
move, take the slide with the most points plus value. To learn, after the
next move, nudge the last afterstate's value toward what followed:

    V(after) += alpha * (points next + V(next after) - V(after))

and toward 0 when the game ends. The randomness of the spawns is all the
exploration it needs.

**Tuples:** "4" (the default) is rows of four and 2 x 2 squares, 65,536
patterns each; "6" adds 2 x 3 rectangles, 16 million patterns each, as in
the paper: much stronger, much bigger (a few hundred MB).
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

import numpy as np

from .. import register
from ..base import Agent

# The cells of each tuple shape, as (row, column), on the unturned board.
_SHAPES = {
    "4": [[(0, 0), (0, 1), (0, 2), (0, 3)], [(1, 0), (1, 1), (1, 2), (1, 3)],
          [(0, 0), (0, 1), (1, 0), (1, 1)], [(1, 1), (1, 2), (2, 1), (2, 2)]],
    "6": [[(0, 0), (0, 1), (0, 2), (0, 3)], [(1, 0), (1, 1), (1, 2), (1, 3)],
          [(0, 0), (0, 1), (0, 2), (1, 0), (1, 1), (1, 2)], [(1, 0), (1, 1), (1, 2), (2, 0), (2, 1), (2, 2)]],
}
# Tiles above 2^15 (32768) share the last value: rare enough not to matter.
_LEVELS = 16


def _symmetries(cells: list[tuple[int, int]]) -> list[np.ndarray]:
    """The tuple's cells, as flat indices, under the board's 8 turns and mirrors."""
    out = []
    for flip in (False, True):
        for turns in range(4):
            mapped = []
            for r, c in cells:
                if flip:
                    c = 3 - c
                for _ in range(turns):
                    r, c = c, 3 - r
                mapped.append(r * 4 + c)
            out.append(np.array(mapped))
    return out


@register
class NTuple(Agent):
    name = "ntuple"
    games = ("oma2048",)
    description = "An n-tuple network learning afterstate values by TD (Szubert and Jaśkowski's 2048 learner)."
    trainable = True
    default_steps = 1_000_000

    @dataclass
    class Config:
        tuples: str = "4"
        # How far each update moves the weights it touches.
        alpha: float = 0.0025
        seed: int = 0

    def __init__(self, config: Config, spec, device: str = "cpu"):
        super().__init__(config, spec, device)
        if config.tuples not in _SHAPES:
            raise ValueError(f"tuples must be one of {', '.join(_SHAPES)}, not {config.tuples!r}")
        shapes = _SHAPES[config.tuples]
        # Per shape: the 8 symmetric placements, and powers of 16 to read a
        # pattern as one table index.
        self.placements = [np.stack(_symmetries(cells)) for cells in shapes]      # [8, k] each
        self.radix = [_LEVELS ** np.arange(len(cells)) for cells in shapes]
        self.tables = [np.zeros(_LEVELS ** len(cells), np.float32) for cells in shapes]

    def _indices(self, boards: np.ndarray) -> list[np.ndarray]:
        """Per shape, [n, 8] table indices for n boards (powers of two, flattened)."""
        flat = np.minimum(boards.reshape(len(boards), 16), _LEVELS - 1)
        return [(flat[:, placement] * radix).sum(axis=2) for placement, radix in zip(self.placements, self.radix)]

    def values(self, boards: np.ndarray) -> np.ndarray:
        return sum(table[index].sum(axis=1) for table, index in zip(self.tables, self._indices(boards)))

    def _choose(self, obs, mask) -> tuple[int, np.ndarray, float]:
        """The best slide, its afterstate, and its points plus value."""
        legal = np.flatnonzero(mask)
        boards = obs["afterstates"][legal]
        score = obs["gains"][legal] + self.values(boards)
        best = int(np.argmax(score))
        return int(legal[best]), boards[best], float(score[best])

    def act(self, obs, mask, explore=False) -> int:
        return self._choose(obs, mask)[0]

    def _update(self, board: np.ndarray, target: float) -> None:
        """Moves the board's value toward `target`, spread over its tuples."""
        error = target - float(self.values(board[None])[0])
        for table, index in zip(self.tables, self._indices(board[None])):
            np.add.at(table, index[0], self.config.alpha * error)

    def train(self, ctx):
        env = ctx.make_env()
        steps = 0
        while True:
            obs = env.reset(ctx.next_seed())
            action, after, _ = self._choose(obs, env.mask())
            while True:
                step = env.step(action)
                steps += 1
                if step.done:
                    # Nothing follows the last afterstate: its value is 0.
                    self._update(after, 0.0)
                    break
                # The TD target: what the next slide earns, plus where it leads.
                action, next_after, best = self._choose(step.obs, env.mask())
                self._update(after, best)
                after = next_after
            info = env.info()
            yield {"steps": steps, "episode_score": info["score"], "highest": info["highest"]}

    def save(self, path: Path) -> None:
        with path.open("wb") as f:
            np.savez_compressed(f, *self.tables)

    def load(self, path: Path) -> None:
        with np.load(path) as saved:
            self.tables = [saved[f"arr_{i}"] for i in range(len(self.tables))]
