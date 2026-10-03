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
from .evaluation import EVAL_SEED_BASE, evaluate, play
from .store import Store


@dataclass
class TrainContext:
    """What an agent's train() is given."""

    game: str
    env_config: dict
    seed: int
    device: str
    rng: np.random.Generator = field(init=False)

    def __post_init__(self):
        self.rng = np.random.default_rng(self.seed)

    def make_env(self, **overrides) -> Env:
        return Env(self.game, **{**self.env_config, **overrides})

    def next_seed(self) -> int:
        """A fresh training game, never one of the evaluation games."""
        return int(self.rng.integers(EVAL_SEED_BASE))


@dataclass
class Schedule:
    steps: int
    eval_every: int
    eval_episodes: int
    final_episodes: int
    max_steps: int
    # Games recorded along the way, evenly spaced from step 0 to the end.
    snapshots: int = 10

    def snapshot_steps(self) -> list[int]:
        if self.snapshots <= 0:
            return []
        if self.snapshots == 1:
            return [self.steps]
        return [round(i * self.steps / (self.snapshots - 1)) for i in range(self.snapshots)]


def train(store: Store, run: dict, agent: Agent, schedule: Schedule, echo=print) -> str:
    """Trains `agent` within `run`; returns the run's final status."""
    run_dir = Path(run["dir"])
    ctx = TrainContext(run["game"], run["env_config"], run["seed"], run["device"])
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
                _, summary = evaluate(agent, run["game"], run["env_config"], schedule.eval_episodes, schedule.max_steps)
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
    store.finish(run["id"], status)
    return status


def _snapshot(store: Store, run: dict, agent: Agent, schedule: Schedule, index: int, steps: int,
              label: str) -> None:
    """Records the agent as it plays now, on the first evaluation game."""
    with Env(run["game"], **{**run["env_config"], "max_steps": schedule.max_steps}) as env:
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
