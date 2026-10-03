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
from .comparison import metric_key
from .env import Env
from .games import defaults

EVAL_SEED_BASE = 1_000_000_000


def play(agent: Agent, env: Env, seed: int, explore: bool = False) -> dict:
    """One episode; what it scored, how long it lasted, and its signals summed."""
    obs = env.reset(seed)
    # Every signal summed over the game, and the highest it reached (a 2048
    # run's best tile is max_highest); plus whatever the game's own tracker
    # counts (Tetris: clears by size, streaks).
    totals = {name: 0.0 for name in env.spec.signals}
    peaks = {name: float("-inf") for name in env.spec.signals}
    make_tracker = defaults(env.game).tracker
    tracker = make_tracker() if make_tracker else None
    reward, steps = 0.0, 0
    while True:
        step = env.step(agent.decide(env, obs, env.mask(), explore=explore))
        steps += 1
        reward += step.reward
        for name, value in step.signals.items():
            totals[name] += value
            peaks[name] = max(peaks[name], value)
        if tracker:
            tracker.step(step.signals)
        obs = step.obs
        if step.done:
            break
    info = env.info()
    # Won: the game reached its goal (a Challenge cleared, a Sprint's forty
    # lines), rather than being lost or cut short.
    won = step.terminated and info.get("phase") == "finished"
    episode = {
        "score": info.get("score", reward),
        "steps": steps,
        "reward": reward,
        "won": float(won),
        # How hard the game was dealt, where the game says (a Challenge's
        # Difficulty, 1 to 100); 0 where it doesn't.
        "difficulty": float(info.get("dealt_difficulty", 0)),
        "ended": "terminated" if step.terminated else "cut short",
        **{f"sum_{k}": v for k, v in totals.items()},
        **{f"max_{k}": v for k, v in peaks.items()},
    }
    return episode | (tracker.result(episode) if tracker else {})


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
    best_by: str = "score",
    lower_is_better: bool = False,
) -> tuple[list[dict], dict[str, float]]:
    """Plays `episodes` fixed games; their results and a summary.

    With `replays`, the best and worst games by `best_by` are written there
    as best.json and worst.json, for `omagym watch`, and every game as
    games/NN.json, so two runs can be watched playing the same deal.
    """
    sign = -1.0 if lower_is_better else 1.0
    env = Env(game, **{**env_config, "max_steps": max_steps})
    results, best, worst = [], None, None
    for i in range(episodes):
        seed = EVAL_SEED_BASE + i
        result = {"episode": i, "seed": seed, **play(agent, env, seed)}
        # How long it took to win, a loss counting as the whole cap: what
        # "fewer pieces is better" ranks by, without rewarding a quick loss.
        result["steps_to_win"] = result["steps"] if result["won"] else max_steps
        # The same, per point of difficulty: a hard deal is allowed more pieces,
        # so games of different difficulty can be held to one standard.
        if result["difficulty"]:
            result["pieces_per_difficulty"] = result["steps_to_win"] / result["difficulty"]
        results.append(result)
        key = metric_key(best_by, result)
        if replays is not None:
            replay = {**env.replay(), "agent": label, "episode": i}
            games = replays / "games"
            games.mkdir(parents=True, exist_ok=True)
            (games / f"{i:02d}.json").write_text(json.dumps(replay) + "\n")
            goodness = sign * result[key]
            if best is None or goodness > best[0]:
                best = (goodness, env.replay())
            if worst is None or goodness < worst[0]:
                worst = (goodness, env.replay())
        if progress:
            progress(i, result)
    env.close()
    if replays is not None:
        replays.mkdir(parents=True, exist_ok=True)
        for which, (_, replay) in (("best", best), ("worst", worst)):
            replay["agent"] = label
            (replays / f"{which}.json").write_text(json.dumps(replay, indent=2) + "\n")
    return results, summarize(results)


def other_tests(agent: Agent, run: dict, store, label: str, progress=None) -> None:
    """Plays the game's other tests (games.defaults(game).tests) and records them.

    Their games are stored under the test's name, their summary with its name
    in front ("challenge/steps_to_win_mean"), their replays in replays/<name>/.
    """
    for test in defaults(run["game"]).tests:
        env_config = {**run["env_config"], **test.env}
        episodes, summary = evaluate(agent, run["game"], env_config, test.episodes, test.max_steps,
                                     Path(run["dir"]) / "replays" / test.name, label, progress,
                                     test.metric, test.lower_is_better)
        store.add_episodes(run["id"], episodes, test.name)
        store.set_summary(run["id"], {f"{test.name}/{k}": v for k, v in summary.items()})
