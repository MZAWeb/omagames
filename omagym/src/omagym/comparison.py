"""Which run played better, and whether the difference is real.

Every run is tested on the same fixed games (see evaluation.py), so runs are
compared game by game: on game 7, A scored 40 more than B. Those paired
differences are far less noisy than the difference of two averages, because
how kind each game's pieces were cancels out. A bootstrap over them gives a
95% range for the true difference. If the whole range is below zero, the run
is worse than the leader beyond reasonable doubt; if it straddles zero, these
games can't tell the two apart.
"""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np

# Resamples for the bootstrap, and its seed, so a comparison always prints
# the same range for the same runs.
_RESAMPLES = 10_000
_SEED = 0


@dataclass(frozen=True)
class Standing:
    run: dict
    mean: float
    std: float
    # Against the leader, game by game. Zero, and verdict "best", for the leader.
    wins: int = 0
    ties: int = 0
    losses: int = 0
    difference: float = 0.0
    low: float = 0.0
    high: float = 0.0
    verdict: str = "best"


@dataclass(frozen=True)
class Ranking:
    metric: str
    lower_is_better: bool
    seeds: list[int]
    standings: list[Standing]
    # Which test, and where its games were cut.
    test: str = ""
    cap: int = 0


def metric_key(metric: str) -> str:
    """The episode field a metric is read from: "lines" is the summed signal "sum_lines"."""
    direct = ("score", "steps", "reward", "won", "steps_to_win", "difficulty", "pieces_per_difficulty",
              "points_per_line", "avg_holes", "avg_height", "clears_1", "clears_2", "clears_3", "clears_4",
              "tetris_share", "tetris_streak")
    return metric if metric in direct else f"sum_{metric}"


def rank(runs: list[dict], episodes: dict[str, list[dict]], metric: str = "score",
         lower_is_better: bool = False, test: str = "", cap: int | None = None) -> Ranking:
    """Ranks `runs` by `metric` on the games all of them played."""
    if len({r["game"] for r in runs}) > 1:
        raise ValueError("those runs are of different games")
    if len({r["eval_max_steps"] for r in runs}) > 1:
        caps = ", ".join(f"{r['id']} at {r['eval_max_steps']}" for r in runs)
        raise ValueError(f"those runs' games were cut at different lengths ({caps}), so they played different tests")
    key = metric_key(metric)
    results = {}
    for run in runs:
        played = episodes.get(run["id"], [])
        if not played:
            raise ValueError(f"{run['id']} has no evaluation results ({run['status']})")
        if key not in played[0]:
            known = sorted(k.removeprefix("sum_") for k, v in played[0].items()
                           if isinstance(v, (int, float)) and k not in ("episode", "seed"))
            raise ValueError(f"no metric {metric!r}; this game has: {', '.join(known)}")
        results[run["id"]] = {e["seed"]: float(e[key]) for e in played}
    seeds = sorted(set.intersection(*(set(r) for r in results.values())))
    if not seeds:
        raise ValueError("those runs played no game in common")

    values = {rid: np.array([r[s] for s in seeds]) for rid, r in results.items()}
    # Ranking and verdicts work on "goodness", which is the metric or its
    # negative; differences are reported in the metric's own units.
    sign = -1.0 if lower_is_better else 1.0
    # Stable, so runs that score exactly alike keep the order they were given in.
    ordered = sorted(runs, key=lambda r: -sign * values[r["id"]].mean())
    leader = values[ordered[0]["id"]]
    standings = [Standing(ordered[0], leader.mean(), leader.std())]
    for run in ordered[1:]:
        mine = values[run["id"]]
        gaps = mine - leader
        better = sign * gaps
        low, high = _bootstrap(gaps)
        standings.append(Standing(
            run, mine.mean(), mine.std(),
            wins=int((better > 0).sum()), ties=int((better == 0).sum()), losses=int((better < 0).sum()),
            difference=float(gaps.mean()), low=low, high=high,
            verdict=_verdict(gaps, high if sign > 0 else -low),
        ))
    return Ranking(metric, lower_is_better, seeds, standings, test, cap or runs[0]["eval_max_steps"])


def _bootstrap(gaps: np.ndarray) -> tuple[float, float]:
    rng = np.random.default_rng(_SEED)
    means = gaps[rng.integers(len(gaps), size=(_RESAMPLES, len(gaps)))].mean(axis=1)
    low, high = np.percentile(means, [2.5, 97.5])
    return float(low), float(high)


def _verdict(gaps: np.ndarray, best_case: float) -> str:
    """`best_case`: the end of the range most in the run's favour, in goodness."""
    if not gaps.any():
        return "same"
    return "worse" if best_case < 0 else "can't tell"


def style(runs: list[dict], test: str, columns: tuple, main: bool) -> list[dict]:
    """How each run played a test, beyond the measure it is ranked by.

    One row per column the game names (lines, points per line, Tetrises,
    holes...), each run's mean over the test's games, and which run did best
    on it. Plus how many games each kept going (the main test: survived to
    the cap) or won (another test).
    """
    prefix = f"{test}/" if test else ""
    rows = []
    for key, label, lower in columns:
        values = [run["summary"].get(f"{prefix}{key}_mean") for run in runs]
        rows.append({"key": key, "label": label, "lower": lower, "values": values})
    kept = "cut_short" if main else "won_mean"
    games = [run["summary"].get(f"{prefix}episodes") for run in runs]
    values = [run["summary"].get(f"{prefix}{kept}") for run in runs]
    if not main:
        values = [v * g if v is not None and g else None for v, g in zip(values, games)]
    rows.append({"key": kept, "label": "survived to the cap" if main else "won", "lower": False, "values": values})
    for row in rows:
        known = [v for v in row["values"] if v is not None]
        row["best"] = (min(known) if row["lower"] else max(known)) if known else None
    return [row for row in rows if any(v is not None for v in row["values"])]


def combine(name: str, members: list[dict], episodes: list[list[dict]]) -> tuple[dict, list[dict]]:
    """A group of runs (the seeds of one setup) as if it were one run.

    On each game every seed played, the group's result is the seeds' mean,
    so a group is ranked game by game like any run, with the luck of any one
    seed averaged out. Its summary is the mean of theirs, its trained steps
    their mean; settings are the first seed's (the seeds differ only in seed).
    """
    first = members[0]
    by_seed = [{e["seed"]: e for e in played} for played in episodes]
    seeds = sorted(set.intersection(*(set(s) for s in by_seed))) if by_seed else []
    games = []
    for seed in seeds:
        rows = [s[seed] for s in by_seed]
        game = {"seed": seed, "episode": rows[0].get("episode", 0), "ended": rows[0].get("ended")}
        for key, value in rows[0].items():
            if isinstance(value, (int, float)) and key not in ("seed", "episode") and all(key in r for r in rows):
                game[key] = sum(r[key] for r in rows) / len(rows)
        games.append(game)
    summary = {}
    for key in first["summary"]:
        values = [m["summary"][key] for m in members if key in m["summary"]]
        if len(values) == len(members):
            summary[key] = sum(values) / len(values)
    steps = [m.get("trained_steps") for m in members if m.get("trained_steps")]
    run = {**first, "id": f"group:{name}", "name": f"{name} ({len(members)} seeds)", "group_name": name,
           "summary": summary, "trained_steps": sum(steps) / len(steps) if steps else None,
           "notes": first["notes"], "members": [m["id"] for m in members]}
    return run, games
