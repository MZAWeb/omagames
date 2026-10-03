"""XGBoost learning to rank a teacher's choices: imitation with gradient-boosted trees.

**The idea.** A strong player is slow (`mcts` searches every move) or only
as good as its hand-written rules (`lookahead`). Watch it play, and train a
fast model to make the same choices. That is *imitation learning*, or
*distillation* when the teacher is a search (README, Science 2.6). The
student here is XGBoost, gradient-boosted decision trees, as a *ranker*.

**Learning to rank.** Every position offers a few dozen landings, and the
teacher picks one. That is one *query*: a group of candidates, one of them
labelled 1 (chosen) and the rest 0. XGBoost's `rank:pairwise` objective
learns a score for any landing such that, within each query, the chosen
one scores above the others. To play, score every landing on offer and
take the best, as every Tetris agent here does.

**Why trees.** A landing is described by numbers about the board it leaves
and the pieces in hand (`rich_hand`, the same 31 that `dqn` can see). A
weighted sum of them, like greedy's or cem's, can't say "holes matter more
when the stack is high"; trees split on one feature, then another, so
interactions like that come for free. And they train in seconds on a CPU.

**The catch: drift.** A student trained only on the teacher's games has
never seen the positions its own small mistakes lead to, and there it has
no idea. DAgger (Ross et al., 2011) fixes it: once it has a model, let the
student play some of the moves (`dagger`), while the teacher still says
what it would have done. Then it learns to recover from its own mistakes.

**Read next:** README, To do 2, for what to try with it.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

import numpy as np
import xgboost as xgb

from ..games import omatris
from . import afterstate_value, register, resolve
from .base import Agent


@register
class XGBoostRanker(Agent):
    name = "xgb"
    games = ("omatris",)
    description = "XGBoost trees learning to rank the landings a teacher (any agent or run) chooses: imitation."
    trainable = True
    default_steps = 20_000

    @dataclass
    class Config:
        # Who to learn from: a run (its name or id), or an agent's name for
        # that agent with its default settings ("lookahead", "greedy").
        teacher: str = "lookahead"
        # What a landing is described by (see afterstate_value.py).
        inputs: str = "rich_hand"
        # The share of moves the student plays itself once it has a model,
        # the teacher still labelling them (DAgger). 0 is plain imitation.
        dagger: float = 0.0
        # Training games, cut at this many pieces, and how often (in games)
        # the trees are fitted again on everything seen so far.
        episode_steps: int = 300
        refit_every: int = 5
        # The trees: how many, how deep, how big a step each takes.
        trees: int = 200
        depth: int = 6
        learning_rate: float = 0.1
        objective: str = "rank:pairwise"
        seed: int = 0

    @classmethod
    def env_config(cls, game: str) -> dict:
        return {"actions": "placement"}

    def __init__(self, config: Config, spec, device: str = "cpu"):
        super().__init__(config, spec, device)
        afterstate_value.check(config.inputs)
        if not 0.0 <= config.dagger <= 1.0:
            raise ValueError("dagger is a share of moves, 0 to 1")
        self.rng = np.random.default_rng(config.seed)
        self.booster: xgb.Booster | None = None

    # -- playing --------------------------------------------------------------

    def _inputs(self, obs, mask) -> np.ndarray:
        return afterstate_value.inputs_for(self.config.inputs, obs, self.spec, omatris.landing_count(mask))

    def act(self, obs, mask, explore=False) -> int:
        if self.booster is None:
            return 0   # nothing learned yet: any landing will do
        scores = self.booster.predict(xgb.DMatrix(self._inputs(obs, mask)))
        return int(np.argmax(scores))

    # -- learning -------------------------------------------------------------

    def _teacher(self) -> Agent:
        """The teacher: a recorded run, trained if it learned, or an agent's defaults."""
        from ..store import Store

        try:
            run = Store().run(self.config.teacher)
        except KeyError:
            cls = resolve(self.config.teacher, "omatris")
            return cls(cls.Config(), self.spec, self.device)
        cls = resolve(run["agent"], run["game"])
        if cls.env_config("omatris").get("actions") != "placement":
            raise ValueError(f"{run['id']} plays in another action space; the teacher has to place pieces")
        teacher = cls(cls.Config(**run["agent_config"]), self.spec, self.device)
        if cls.trainable:
            teacher.load(Path(run["dir"]) / "best.pt")
        return teacher

    def train(self, ctx):
        c = self.config
        teacher = self._teacher()
        env = ctx.make_env(max_steps=c.episode_steps)
        # Everything seen so far: one row per landing, its label, and which
        # position (query) it was on offer in.
        rows, labels, queries = [], [], []
        steps = games = 0
        while True:
            obs = env.reset(ctx.next_seed())
            while True:
                mask = env.mask()
                choice = teacher.decide(env, obs, mask)
                features = self._inputs(obs, mask)
                rows.append(features)
                labels.append(np.arange(len(features)) == choice)
                queries.append(np.full(len(features), steps))
                # DAgger: sometimes play the student's move, labelled by the teacher's.
                student = self.booster is not None and self.rng.random() < c.dagger
                step = env.step(self.act(obs, mask) if student else choice)
                steps += 1
                if step.done:
                    break
                obs = step.obs
            games += 1
            progress = {"steps": steps, "games": float(games), "positions": float(steps)}
            if games % c.refit_every == 0:
                progress["agreement"] = self._fit(np.concatenate(rows), np.concatenate(labels),
                                                  np.concatenate(queries))
            yield progress

    def _fit(self, x: np.ndarray, y: np.ndarray, qid: np.ndarray) -> float:
        """Fits the trees on every position so far; how often they pick the teacher's landing."""
        c = self.config
        params = {"objective": c.objective, "max_depth": c.depth, "eta": c.learning_rate, "tree_method": "hist",
                  "device": "cuda" if self.device == "cuda" else "cpu", "seed": c.seed}
        data = xgb.DMatrix(x, label=y.astype(np.float32), qid=qid)
        self.booster = xgb.train(params, data, num_boost_round=c.trees)
        # Agreement: on what share of positions the trees' best is the teacher's.
        scores = self.booster.predict(data)
        starts = np.flatnonzero(np.r_[True, qid[1:] != qid[:-1]])
        picks = [s + int(np.argmax(scores[s:e])) for s, e in zip(starts, np.r_[starts[1:], len(qid)])]
        return float(np.mean(y[picks]))

    # -- keeping what was learned ---------------------------------------------

    def save(self, path: Path) -> None:
        if self.booster is not None:
            path.write_bytes(bytes(self.booster.save_raw("json")))

    def load(self, path: Path) -> None:
        self.booster = xgb.Booster()
        self.booster.load_model(bytearray(path.read_bytes()))
