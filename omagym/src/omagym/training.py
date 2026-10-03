"""Running an agent's training and recording all of it.

The agent owns its learning loop (`Agent.train()` yields progress); this
owns everything around it, the same for every agent: the step budget, a
quick evaluation every so often (the learning curve), a checkpoint of the
best and of the last, and a full evaluation at the end, so a training run
ends with a summary comparable to any eval run's.

It also records snapshots: the agent playing one game at evenly spaced
points of its training, from before it learned anything to the end. Every
snapshot plays the same game (the first evaluation game), so watching them
in order shows the same deal handled better and better.
"""

from __future__ import annotations

import json
from dataclasses import dataclass, field
from pathlib import Path

import numpy as np

from .agents import Agent
from .env import Env
from .evaluation import EVAL_SEED_BASE, evaluate, other_tests, play
from .games import defaults
from .store import Store


@dataclass
class TrainContext:
    """What an agent's train() is given."""

    game: str
    env_config: dict
    seed: int
    device: str
    # The training mix: for each of the game's other tests by name, the share
    # of training games played its way (`--train-mix challenge=0.3`).
    mix: dict[str, float] = field(default_factory=dict)
    rng: np.random.Generator = field(init=False)

    def __post_init__(self):
        self.rng = np.random.default_rng(self.seed)

    def make_env(self, **overrides) -> Env:
        """The game to train on: with a mix, one that picks its setup each game."""
        base = Env(self.game, **{**self.env_config, **overrides})
        if not self.mix:
            return base
        tests = {t.name: t for t in defaults(self.game).tests}
        envs = [base] + [Env(self.game, **{**self.env_config, **tests[n].env, **overrides}) for n in self.mix]
        return _MixedEnv(envs, [1.0 - sum(self.mix.values()), *self.mix.values()])

    def next_seed(self) -> int:
        """A fresh training game, never one of the evaluation games."""
        return int(self.rng.integers(EVAL_SEED_BASE))


class _MixedEnv:
    """Training games drawn from several setups (Marathon, Challenge...).

    Every reset picks one setup at random, in the mix's proportions, and the
    game is then played in it; everything else is passed to that game. The
    pick comes from the game's seed, so the same seeds make the same mix.
    All setups share one spec (they differ in mode, not in what is seen).
    """

    def __init__(self, envs: list[Env], shares: list[float]):
        self._envs, self._shares = envs, np.array(shares) / sum(shares)
        self._current = envs[0]

    def reset(self, seed: int):
        pick = np.random.default_rng(seed).choice(len(self._envs), p=self._shares)
        self._current = self._envs[int(pick)]
        return self._current.reset(seed)

    def close(self) -> None:
        for env in self._envs:
            env.close()

    def __getattr__(self, name):
        return getattr(self._current, name)


@dataclass
class Schedule:
    steps: int
    eval_every: int
    eval_episodes: int
    final_episodes: int
    max_steps: int
    # Where the quick evaluations and the snapshots are cut: shorter than the
    # final evaluation's games, since a learning curve only needs the trend
    # and nobody watches a 2,500-piece snapshot. 0 means max_steps.
    quick_max_steps: int = 500
    # Games recorded along the way, evenly spaced from step 0 to the end.
    snapshots: int = 10

    @property
    def quick_cap(self) -> int:
        return min(self.quick_max_steps or self.max_steps, self.max_steps)

    def snapshot_steps(self) -> list[int]:
        if self.snapshots <= 0:
            return []
        if self.snapshots == 1:
            return [self.steps]
        return [round(i * self.steps / (self.snapshots - 1)) for i in range(self.snapshots)]


def train(store: Store, run: dict, agent: Agent, schedule: Schedule, echo=print) -> str:
    """Trains `agent` within `run`; returns the run's final status."""
    run_dir = Path(run["dir"])
    ctx = TrainContext(run["game"], run["env_config"], run["seed"], run["device"], run.get("train_mix") or {})
    label = run["name"] or run["id"]
    best_score, next_eval = float("-inf"), schedule.eval_every
    upcoming, taken = schedule.snapshot_steps(), 0

    def snapshot_due(steps: int) -> None:
        nonlocal upcoming, taken
        if upcoming and steps >= upcoming[0]:
            taken += 1
            _snapshot(store, run, agent, schedule, taken, steps, label)
            # One game per yield: marks a long gap between yields jumped past
            # are not made up with copies of the same moment.
            upcoming = [s for s in upcoming if s > steps]

    status = "done"
    snapshot_due(0)
    learning = agent.train(ctx)
    try:
        for progress in learning:
            steps = int(progress["steps"])
            store.log(run["id"], steps, {f"train/{k}": v for k, v in progress.items()})
            snapshot_due(steps)
            if steps >= next_eval or steps >= schedule.steps:
                next_eval += schedule.eval_every
                _, summary = evaluate(agent, run["game"], run["env_config"], schedule.eval_episodes, schedule.quick_cap)
                store.log(run["id"], steps, {f"eval/{k}": v for k, v in summary.items()})
                score = summary["score_mean"]
                echo(f"{steps:>9} steps  eval score {score:>12,.1f}  {_highlights(progress)}")
                agent.save(run_dir / "last.pt")
                if score > best_score:
                    best_score = score
                    agent.save(run_dir / "best.pt")
            if steps >= schedule.steps:
                break
    except KeyboardInterrupt:
        status = "interrupted"
        echo("interrupted: evaluating what was learned so far")
    finally:
        learning.close()

    # The final word is the best checkpoint on the full evaluation set.
    if (run_dir / "best.pt").exists():
        agent.load(run_dir / "best.pt")
    episodes, summary = evaluate(
        agent, run["game"], run["env_config"], schedule.final_episodes, schedule.max_steps,
        replays=run_dir / "replays", label=label,
    )
    store.add_episodes(run["id"], episodes)
    store.set_summary(run["id"], summary)
    other_tests(agent, run, store, label)
    store.finish(run["id"], status)
    return status


def _snapshot(store: Store, run: dict, agent: Agent, schedule: Schedule, index: int, steps: int,
              label: str) -> None:
    """Records the agent as it plays now, on the first evaluation game."""
    with Env(run["game"], **{**run["env_config"], "max_steps": schedule.quick_cap}) as env:
        result = play(agent, env, EVAL_SEED_BASE)
        replay = env.replay()
    replay["agent"] = f"{label} after {steps:,} steps"
    replay["training_steps"] = steps
    folder = Path(run["dir"]) / "snapshots"
    folder.mkdir(exist_ok=True)
    (folder / f"{index:02d}.json").write_text(json.dumps(replay, indent=2) + "\n")
    store.log(run["id"], steps, {"snapshot/score": result["score"]})


def snapshot_files(run: dict) -> list[Path]:
    """A training run's snapshots, earliest first."""
    return sorted((Path(run["dir"]) / "snapshots").glob("*.json"))


def _highlights(progress: dict) -> str:
    shown = []
    for key, value in progress.items():
        if key != "steps" and isinstance(value, float) and np.isfinite(value):
            shown.append(f"{key} {value:,.3g}")
    return "  ".join(shown)
