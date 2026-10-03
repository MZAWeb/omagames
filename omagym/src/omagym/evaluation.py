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
    # Clears by size (a Tetris is a clear of four), where the game clears lines.
    clears = [0, 0, 0, 0]
    # Tetrises in a row (other clears end a streak), and the longest streak.
    streak = longest = 0
    reward, steps = 0.0, 0
    while True:
        step = env.step(agent.decide(env, obs, env.mask(), explore=explore))
        steps += 1
        reward += step.reward
        for name, value in step.signals.items():
            totals[name] += value
        if 1 <= step.signals.get("lines", 0) <= 4:
            clears[int(step.signals["lines"]) - 1] += 1
            streak = streak + 1 if step.signals["lines"] == 4 else 0
            longest = max(longest, streak)
        obs = step.obs
        if step.done:
            break
    info = env.info()
    # Won: the game reached its goal (a Challenge cleared, a Sprint's forty
    # lines), rather than being lost or cut short.
    won = step.terminated and info.get("phase") == "finished"
    return {
        "score": info.get("score", reward),
        "steps": steps,
        "reward": reward,
        "won": float(won),
        # How hard the game was dealt, where the game says (a Challenge's
        # Difficulty, 1 to 100); 0 where it doesn't.
        "difficulty": float(info.get("dealt_difficulty", 0)),
        "ended": "terminated" if step.terminated else "cut short",
        **{f"sum_{k}": v for k, v in totals.items()},
        **({f"clears_{n}": float(c) for n, c in enumerate(clears, 1)} if "lines" in totals else {}),
        **({"tetris_streak": float(longest),
            "tetris_share": 4 * clears[3] / totals["lines"] if totals["lines"] else 0.0} if "lines" in totals else {}),
    } | style({"steps": steps, "score": info.get("score", reward), **{f"sum_{k}": v for k, v in totals.items()}})


def style(episode: dict) -> dict:
    """How a game was played, beyond its score: points per line, and the
    holes and stack height after an average move. From an episode's sums, so
    it can be worked out again for games already recorded."""
    out = {}
    steps = max(1, episode["steps"])
    if "sum_lines" in episode:
        # 0 for a game with no lines, so every game has the field.
        out["points_per_line"] = episode["score"] / episode["sum_lines"] if episode["sum_lines"] else 0.0
    for signal, name in (("sum_holes", "avg_holes"), ("sum_max_height", "avg_height")):
        if signal in episode:
            out[name] = episode[signal] / steps
    return out


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
        if replays is not None:
            replay = {**env.replay(), "agent": label, "episode": i}
            games = replays / "games"
            games.mkdir(parents=True, exist_ok=True)
            (games / f"{i:02d}.json").write_text(json.dumps(replay) + "\n")
            goodness = sign * result[best_by]
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
    from .comparison import metric_key as comparison_key
    from .games import defaults

    for test in defaults(run["game"]).tests:
        env_config = {**run["env_config"], **test.env}
        episodes, summary = evaluate(agent, run["game"], env_config, test.episodes, test.max_steps,
                                     Path(run["dir"]) / "replays" / test.name, label, progress,
                                     comparison_key(test.metric), test.lower_is_better)
        store.add_episodes(run["id"], episodes, test.name)
        store.set_summary(run["id"], {f"{test.name}/{k}": v for k, v in summary.items()})
