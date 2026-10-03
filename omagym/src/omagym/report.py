"""Turning runs into text: what `runs`, `show`, `diff` and `compare` print."""

from __future__ import annotations

from .comparison import Ranking
from .games import defaults


def table(headers: list[str], rows: list[list[str]]) -> str:
    widths = [max(len(str(x)) for x in column) for column in zip(headers, *rows)]
    line = lambda cells: "  ".join(str(c).ljust(w) for c, w in zip(cells, widths)).rstrip()  # noqa: E731
    return "\n".join([line(headers), line(["-" * w for w in widths]), *(line(r) for r in rows)])


def number(value: float | None, digits: int = 1) -> str:
    if value is None:
        return "-"
    return f"{value:,.0f}" if abs(value) >= 1000 or float(value).is_integer() else f"{value:,.{digits}f}"


def spread(summary: dict, key: str) -> str:
    """"mean ± std" of a summarised metric, or "-" when the run has none."""
    if f"{key}_mean" not in summary:
        return "-"
    return f"{number(summary[f'{key}_mean'])} ± {number(summary[f'{key}_std'])}"


def headline_keys(game: str) -> list[str]:
    return ["score", *(f"sum_{s}" for s in defaults(game).headline), "steps"]


def runs_table(runs: list[dict]) -> str:
    rows = [[r["id"], r["agent"], r["name"] or "", r["status"], _trained(r), r["code"], spread(r["summary"], "score")]
            for r in runs]
    return table(["run", "agent", "name", "status", "trained", "code", "score"], rows)


def show(run: dict, curve: list[tuple[int, float]], patch_exists: bool, trained: dict | None = None,
         tests: tuple = ()) -> str:
    out = [
        f"run       {run['id']}" + (f"  ({run['name']})" if run["name"] else ""),
        f"agent     {run['agent']} on {run['game']}, {run['status']}",
        f"code      {run['code']}" + ("  (uncommitted changes in code.patch)" if patch_exists else ""),
        f"rules     version {run['rules_version']}   versions {run['versions']}",
        f"started   {run['started']}   finished {run['finished'] or '-'}",
        f"dir       {run['dir']}",
    ]
    if run["notes"]:
        out.append(f"notes     {run['notes']}")
    out += ["", "agent     " + _settings(run["agent_config"]), "env       " + _settings(run["env_config"])]
    if run["summary"]:
        s = run["summary"]
        out += ["", f"evaluated on {number(s['episodes'])} fixed games, cut at {run['eval_max_steps']} steps "
                f"({number(s.get('cut_short', 0))} reached the cap):"]
        rows = [[k.removeprefix("sum_"), number(s.get(f"{k}_mean")), number(s.get(f"{k}_std")),
                 number(s.get(f"{k}_min")), number(s.get(f"{k}_median")), number(s.get(f"{k}_max"))]
                for k in headline_keys(run["game"]) if f"{k}_mean" in s]
        out.append(table(["", "mean", "std", "min", "median", "max"], rows))
    for test in tests:
        prefix = f"{test.name}/"
        if f"{prefix}episodes" not in run["summary"]:
            continue
        s = run["summary"]
        keys = [test.metric, "score", *test.headline]
        field = lambda k: k if k in ("score", "steps", "won", "steps_to_win") else f"sum_{k}"  # noqa: E731
        out += ["", f"{test.name} test, {test.about}: {number(s[prefix + 'episodes'])} games, "
                f"{number(s.get(prefix + 'won_mean', 0) * 100)}% won"]
        out.append(table(["", "mean", "std", "min", "median", "max"],
                         [[k, *(number(s.get(f"{prefix}{field(k)}_{x}")) for x in ("mean", "std", "min", "median", "max"))]
                          for k in keys if f"{prefix}{field(k)}_mean" in s]))
    if curve:
        out += ["", "learning curve (eval score by training steps):", _curve(curve)]
    if trained:
        # Agents report progress between games, so a run ends a little past
        # its budget; only one stopped short of it is worth saying so.
        short = (trained["steps"] or 0) < (run["train_steps"] or 0)
        out += ["", f"trained   {_trained({'trained_steps': trained['steps']})}"
                + (f" of {number(run['train_steps'])} asked for" if short else ""),
                f"model     {', '.join(trained['checkpoints']) or 'no checkpoint'} in {run['dir']}"]
        if trained["snapshots"]:
            out.append(f"snapshots {trained['snapshots']}, of it playing as it learned: "
                       f"omagym watch {run['id']} --training")
    return "\n".join(out)


def ranking(result: Ranking) -> str:
    """The `compare` table: best first, each run against the best, and what that means."""
    standings = result.standings
    first = standings[0].run
    label = _labels([s.run for s in standings])
    metric = result.metric
    better = "lower" if result.lower_is_better else "higher"
    test = f" in the {result.test} test" if result.test else ""
    out = [f"ranked by {metric}, {better} is better: the {len(result.seeds)} games of {first['game']}{test} every "
           f"run played, each cut at {result.cap} steps", ""]
    rows = []
    for place, s in enumerate(standings, 1):
        versus = ["", "", ""] if s.verdict == "best" else [
            f"{s.wins}-{s.ties}-{s.losses}",
            f"{_signed(s.difference)} ({_signed(s.low)} to {_signed(s.high)})",
            s.verdict,
        ]
        rows.append([f"{place}.", label[s.run["id"]], s.run["agent"], _trained(s.run),
                     f"{number(s.mean)} ± {number(s.std)}", *versus])
    out.append(table(["", "run", "agent", "trained", metric, "vs best: won-tied-lost",
                      "difference (95% range)", "verdict"], rows))
    out += ["", *_conclusion(standings, label)]
    differing = _differences([s.run["env_config"] for s in standings])
    if differing:
        out += ["", f"note: env settings differ ({', '.join(differing)}). Fine when that is part of an agent's",
                "design (its action space), not when it changes the game itself (mode)."]
    return "\n".join(out)


def _conclusion(standings, label: dict[str, str]) -> list[str]:
    best = label[standings[0].run["id"]]
    lines = [f"{best} played best."]
    named = lambda verdict: [label[s.run["id"]] for s in standings if s.verdict == verdict]  # noqa: E731
    worse, unclear, same = named("worse"), named("can't tell"), named("same")
    if worse:
        lines.append(f"Worse than {best}, beyond doubt on these games: {', '.join(worse)}.")
    if unclear:
        lines.append(f"Can't be told apart from {best} on these games: {', '.join(unclear)}. "
                     "Testing on more games (--episodes) would settle it.")
    if same:
        lines.append(f"Scored exactly what {best} did on every game: {', '.join(same)}.")
    return lines


def _labels(runs: list[dict]) -> dict[str, str]:
    """What to call each run: its name, unless another run shown has the same one."""
    names = [r["name"] for r in runs]
    return {r["id"]: r["name"] if r["name"] and names.count(r["name"]) == 1 else r["id"] for r in runs}


def _trained(run: dict) -> str:
    steps = run.get("trained_steps")
    return f"{number(steps)} steps" if steps else "-"


def _signed(value: float) -> str:
    return ("+" if value > 0 else "") + number(value)


def diff(runs: list[dict]) -> str:
    game = runs[0]["game"]
    keys = headline_keys(game)
    rows = [[r["id"], r["agent"], _trained(r), r["name"] or "", r["code"], number(r["summary"].get("episodes")),
             *(spread(r["summary"], k) for k in keys)] for r in runs]
    out = [table(["run", "agent", "trained", "name", "code", "games", *(k.removeprefix("sum_") for k in keys)], rows)]

    tests = {(r["game"], r["summary"].get("episodes"), r["eval_max_steps"]) for r in runs}
    if len(tests) > 1:
        out += ["", "warning: these runs were not tested on the same games (game, episodes or step cap differ),",
                "so their numbers are not directly comparable."]
    for title, field in (("agent settings", "agent_config"), ("env settings", "env_config")):
        differing = _differences([r[field] for r in runs])
        if differing:
            out += ["", f"{title} that differ:"]
            out.append(table(["setting", *(r["id"] for r in runs)],
                             [[k, *(str(r[field].get(k, "-")) for r in runs)] for k in differing]))
    return "\n".join(out)


def _settings(values: dict) -> str:
    return ", ".join(f"{k}={v}" for k, v in sorted(values.items())) or "(defaults)"


def _differences(configs: list[dict]) -> list[str]:
    keys = sorted({k for c in configs for k in c})
    return [k for k in keys if len({repr(c.get(k)) for c in configs}) > 1]


def _curve(points: list[tuple[int, float]]) -> str:
    bars = "▁▂▃▄▅▆▇█"
    values = [v for _, v in points]
    low, high = min(values), max(values)
    span = (high - low) or 1.0
    spark = "".join(bars[int((v - low) / span * (len(bars) - 1))] for v in values)
    rows = [[f"{s:,}", number(v)] for s, v in points]
    return f"{spark}  ({number(low)} → {number(high)})\n" + table(["steps", "score"], rows)
