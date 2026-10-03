"""Lookahead: plan the next few pieces with a beam search on copies of the game.

**The idea.** Every agent so far rates each landing of the *current* piece
and takes the best. But a landing that looks fine now can leave no good
place for the next piece, and a landing that looks a little worse can set
up a Tetris two pieces later. The preview shows the next pieces, so we can
simply try: play a landing on a copy of the game, see what the next piece
could do from there, and only then decide.

Trying every combination is too many: about 40 landings per piece makes
1,600 lines of play two pieces deep and 64,000 three deep. A **beam search**
keeps it small: at each depth, keep only the `beam` most promising lines
and look one piece further down those alone.

    depth 1   rate every landing of this piece        → keep the best 8
    depth 2   from each of those 8 boards, rate every
              landing of the next piece               → keep the best 8 of all of them
    depth 3   ...and so on, to `depth`

At the end, play the *first* move of the best line. Next piece, search
again from scratch: the search is a way of choosing one move, not a plan
to stick to.

**What makes a line good.** What it earned on the way (the clears and
spins it made) plus how good the board it ends on is. Only the final board
counts as a board, because the ones on the way are gone; but every clear on
the way counts, because those points are banked. Something has to rate
boards and clears; this agent doesn't learn, it borrows the judgement of
another (`model`):

- by default, greedy's hand-tuned weights, so `depth=1` plays exactly like
  greedy, and anything deeper is greedy plus foresight;
- a `cem` training run: the weights it evolved;
- a `dqn` training run: its network's rating of each board.

**Staying honest.** Copies come from `env.clone(reseed_hidden=True)`. A
copy keeps the pieces a player can see (the preview) and deals its own
beyond them, so a search deeper than the preview is guessing, as a player
would, rather than reading the real future.

**Read next:** mcts.py, which replaces the fixed beam with a search that
spends its time where it matters.
"""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np

from ..env import Env
from ..games import omatris
from . import register
from .base import Agent

# Greedy's four weights, written in the rich features' scaled units, so that
# rating a landing with these is rating it as greedy does.
_GREEDY = {"clear1": 0.760666, "clear2": 2 * 0.760666, "clear3": 3 * 0.760666, "clear4": 4 * 0.760666,
           "holes": -0.35663 * 10, "bumpiness": -0.184483 * 30, "height": -0.510066 * 60, "topped_out": -100.0}


@register
class Lookahead(Agent):
    name = "lookahead"
    games = ("omatris",)
    description = "Beam search over the next pieces on copies of the game, rating boards with greedy, cem or dqn."

    @dataclass
    class Config:
        # Pieces looked at: 1 is this piece only (no foresight), 2 adds the
        # next one, and so on. The preview shows three, plus the hold.
        depth: int = 2
        # Lines of play kept at each depth.
        beam: int = 8
        # Whose judgement rates the boards: empty for greedy's weights, or a
        # cem or dqn training run (its id or name).
        model: str = ""
        # For the copies' own pieces beyond the preview.
        seed: int = 0

    @classmethod
    def env_config(cls, game: str) -> dict:
        return {"actions": "placement"}

    def __init__(self, config: Config, spec, device: str = "cpu"):
        super().__init__(config, spec, device)
        if config.depth < 1 or config.beam < 1:
            raise ValueError("depth and beam must be at least 1")
        self.rng = np.random.default_rng(config.seed)
        self.judge = _judge(config.model, spec, device)

    def act(self, obs, mask, explore=False) -> int:
        raise TypeError("lookahead plans with the game itself; the framework calls decide(env, ...)")

    def decide(self, env: Env, obs, mask, explore=False) -> int:
        c = self.config
        # The lines of play being considered. The search starts with one, the
        # empty line: no move made yet, on the real game.
        lines = [_Line(env=None, obs=obs, mask=mask, first=None, earned=0.0)]
        for depth in range(1, c.depth + 1):
            # Every way to extend every line by one landing, with its value.
            options = []
            for line in lines:
                if line.ended:
                    options.append((line.earned, line, None))   # nothing more to place
                    continue
                count = omatris.landing_count(line.mask)
                for landing, rating in enumerate(self.judge.rate(line.obs, count)):
                    options.append((line.earned + rating, line, landing))
            options.sort(key=lambda option: -option[0])
            if depth == c.depth:
                break
            lines = self._extend(env, options[: c.beam], lines)

        _, line, landing = options[0]
        for kept in lines:
            kept.close()
        # Play the first move of the best line: its own landing if it is
        # still one move long, otherwise the move it started with.
        return landing if line.first is None else line.first

    def _extend(self, env: Env, best: list, lines: list[_Line]) -> list[_Line]:
        """Plays each of the best options out on its own copy of the game."""
        extended = []
        for _, line, landing in best:
            if landing is None:
                extended.append(line)
                continue
            source = line.env or env
            copy = source.clone(reseed_hidden=True, seed=int(self.rng.integers(2**31)))
            step = copy.step(landing)
            extended.append(_Line(
                env=copy, obs=step.obs, mask=copy.mask(),
                first=landing if line.first is None else line.first,
                earned=line.earned + self.judge.earned(step), ended=step.done,
            ))
        for line in lines:
            if line not in extended:
                line.close()
        return extended


@dataclass(eq=False)
class _Line:
    """A line of play: the game after its moves, its first move, what it earned."""

    env: Env | None        # a copy of the game after this line's moves; None for the real game
    obs: dict
    mask: np.ndarray
    first: int | None      # the first landing of the line: what would actually be played
    earned: float
    ended: bool = False    # topped out (or cut short): nothing more to place

    def close(self) -> None:
        if self.env is not None:
            self.env.close()


# -- judges: what rates a landing, and what a landing earned -----------------
#
# A judge answers two questions, in the same units:
# - rate(obs, count): for each landing on offer, what it earns plus how good
#   the board it leaves is;
# - earned(step): what a landing that was just made earned.


class _Weights:
    """Weighted rich features: greedy's, or the ones cem evolved."""

    def __init__(self, spec, weights: dict[str, float]):
        self.spec = spec
        self.w = np.array([weights.get(name, 0.0) for name in omatris.RICH], np.float32)

    def rate(self, obs, count: int) -> np.ndarray:
        return (omatris.rich_features(obs, self.spec, count) / omatris.RICH_SCALE) @ self.w

    def earned(self, step) -> float:
        # The earned features, rebuilt from what the step reports.
        made = dict.fromkeys(omatris.EARNED, 0.0)
        lines = int(step.signals["lines"])
        if lines:
            made[f"clear{lines}"] = 1.0
        if step.signals["tspin"]:
            made["spin_full" if step.signals["tspin"] == 2 else "spin_mini"] = 1.0
        made["topped_out"] = float(step.signals["topped_out"] > 0)
        return float(sum(self.w[omatris.RICH.index(name)] * value for name, value in made.items()))


class _Network:
    """A trained dqn's network, which already rates a board as reward still to come."""

    def __init__(self, agent):
        self.agent = agent

    def rate(self, obs, count: int) -> np.ndarray:
        mask = np.zeros(self.agent.spec.action_count, bool)
        mask[:count] = True
        return self.agent.values(self.agent._inputs(obs, mask))

    def earned(self, step) -> float:
        # The reward the network was trained on, so both answers are in its units.
        return self.agent._reward(step)


def _judge(model: str, spec, device: str):
    if not model:
        return _Weights(spec, _GREEDY)
    # Imported here so that an agent with no model needs neither the
    # experiment store nor PyTorch.
    from pathlib import Path

    from ..store import Store
    from . import resolve

    run = Store().run(model)
    if run["kind"] != "train" or run["agent"] not in ("cem", "dqn"):
        raise ValueError(f"model must be a cem or dqn training run; {run['id']} is a {run['kind']} of {run['agent']}")
    cls = resolve(run["agent"], run["game"])
    trained = cls(cls.Config(**run["agent_config"]), spec, device)
    trained.load(Path(run["dir"]) / "best.pt")
    return _Weights(spec, trained.weights()) if run["agent"] == "cem" else _Network(trained)
