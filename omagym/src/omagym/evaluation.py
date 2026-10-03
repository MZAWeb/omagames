"""Testing an agent fairly: the same games for every agent, every time.

Evaluation episode i is dealt from seed EVAL_SEED_BASE + i, and training
never draws a seed that high, so an agent is never tested on a game it
trained on, and two runs with the same episode count played exactly the same
games. That is what makes their numbers comparable.
"""

from __future__ import annotations

import json
from collections.abc import Callable
from pathlib import Path

import numpy as np

from .agents import Agent
from .env import Env

EVAL_SEED_BASE = 1_000_000_000


def play(agent: Agent, env: Env, seed: int, explore: bool = False) -> dict:
    """One episode; what it scored, how long it lasted, and its signals summed."""
    obs = env.reset(seed)
    totals = {name: 0.0 for name in env.spec.signals}
    reward, steps = 0.0, 0
    while True:
        step = env.step(agent.act(obs, env.mask(), explore=explore))
        steps += 1
        reward += step.reward
        for name, value in step.signals.items():
            totals[name] += value
        obs = step.obs
        if step.done:
            break
    info = env.info()
    return {
        "score": info.get("score", reward),
        "steps": steps,
        "reward": reward,
        "ended": "terminated" if step.terminated else "cut short",
        **{f"sum_{k}": v for k, v in totals.items()},
    }


def summarize(episodes: list[dict]) -> dict[str, float]:
    """mean, std, min, median and max of every number the episodes report."""
    summary: dict[str, float] = {"episodes": len(episodes)}
    keys = [k for k, v in episodes[0].items() if isinstance(v, (int, float)) and k not in ("episode", "seed")]
    for key in keys:
        values = np.array([e[key] for e in episodes], float)
        summary.update({
            f"{key}_mean": values.mean(), f"{key}_std": values.std(), f"{key}_min": values.min(),
            f"{key}_median": float(np.median(values)), f"{key}_max": values.max(),
        })
    summary["cut_short"] = sum(e["ended"] == "cut short" for e in episodes)
    return summary


def evaluate(
    agent: Agent,
    game: str,
    env_config: dict,
    episodes: int,
    max_steps: int,
    replays: Path | None = None,
    label: str = "",
    progress: Callable[[int, dict], None] | None = None,
) -> tuple[list[dict], dict[str, float]]:
    """Plays `episodes` fixed games; their results and a summary.

    With `replays`, the best and worst games by score are written there as
    best.json and worst.json, for `omagym watch`.
    """
    env = Env(game, **{**env_config, "max_steps": max_steps})
    results, best, worst = [], None, None
    for i in range(episodes):
        seed = EVAL_SEED_BASE + i
        result = {"episode": i, "seed": seed, **play(agent, env, seed)}
        results.append(result)
        if replays is not None:
            if best is None or result["score"] > best[0]:
                best = (result["score"], env.replay())
            if worst is None or result["score"] < worst[0]:
                worst = (result["score"], env.replay())
        if progress:
            progress(i, result)
    env.close()
    if replays is not None:
        replays.mkdir(parents=True, exist_ok=True)
        for which, (_, replay) in (("best", best), ("worst", worst)):
            replay["agent"] = label
            (replays / f"{which}.json").write_text(json.dumps(replay, indent=2) + "\n")
    return results, summarize(results)
