"""Expectimax: look a few moves ahead, averaging over where tiles might spawn.

**The idea.** In 2048 every move is two halves: your slide, which you
choose, then a new tile, which chance chooses. A search that treats chance
as an opponent (minimax) plays far too scared; one that ignores it plays as
if the spawn were known. *Expectimax* does what the game asks: at your
moves, take the best (max); at the spawns, take the average (the
expectation). Then a move is worth what it earns plus the average of what
the best play can make of where the tile lands. It is the planner behind
the strongest 2048 bots, and the "chance nodes" Tetris's mcts doesn't have.

**Sampled, not enumerated.** A full expectimax would try every empty cell
with a 2 and with a 4. That needs the rules in Python; instead this plays
each slide on `samples` copies of the real game, each dealt its own spawn
(`env.clone(reseed_hidden=True)`), and averages them: *sparse sampling*
(Kearns, Mansour and Ng, 2002). Fewer samples are noisier and faster.

**At the bottom of the search** (`depth` moves down), boards are rated by
greedy's weighted features: the same judgement greedy uses one move ahead,
used here a few moves ahead.
"""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np

from ...env import Env
from .. import register
from ..base import Agent
from .greedy import Greedy2048


@register
class Expectimax(Agent):
    name = "expectimax"
    games = ("oma2048",)
    description = "Expectimax over slides and sampled spawns on copies of the game, greedy's features at the leaves."

    @dataclass
    class Config:
        # Slides looked ahead, this one included.
        depth: int = 2
        # Spawns sampled at each chance node.
        samples: int = 4
        # The points a slide scores count this much, per 100, against the
        # leaf ratings (as greedy's gain).
        gain: float = 1.0
        seed: int = 0

    def __init__(self, config: Config, spec, device: str = "cpu"):
        super().__init__(config, spec, device)
        if config.depth < 1 or config.samples < 1:
            raise ValueError("depth and samples must be at least 1")
        self.rng = np.random.default_rng(config.seed)
        self.judge = Greedy2048(Greedy2048.Config(), spec, device)

    def act(self, obs, mask, explore=False) -> int:
        raise TypeError("expectimax searches with the game itself; the framework calls decide(env, ...)")

    def decide(self, env: Env, obs, mask, explore=False) -> int:
        legal = np.flatnonzero(mask)
        values = [self._chance(env, int(a), self.config.depth) for a in legal]
        return int(legal[int(np.argmax(values))])

    def _chance(self, env: Env, action: int, depth: int) -> float:
        """A slide's worth: the average, over sampled spawns, of what it earns and leads to."""
        total = 0.0
        for _ in range(self.config.samples):
            copy = env.clone(reseed_hidden=True, seed=int(self.rng.integers(2**31)))
            step = copy.step(action)
            total += self.config.gain * step.reward / 100.0 + self._max(copy, step, depth - 1)
            copy.close()
        return total / self.config.samples

    def _max(self, env: Env, step, depth: int) -> float:
        """A position's worth: the best slide from it, or its rating at the bottom."""
        if step.done:
            # Lost: worse than any board's rating, which can be negative.
            return -1000.0 if step.terminated else float(self.judge.rate(step.obs["board"][None].astype(np.int32))[0])
        if depth == 0:
            return float(self.judge.rate(step.obs["board"][None].astype(np.int32))[0])
        mask = env.mask()
        return max(self._chance(env, int(a), depth) for a in np.flatnonzero(mask))
