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
    return metric if metric in ("score", "steps", "reward", "won", "steps_to_win") else f"sum_{metric}"


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
