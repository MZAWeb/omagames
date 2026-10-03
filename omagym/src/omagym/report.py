"""Turning runs into text: the tables `runs`, `show` and `compare` print."""

from __future__ import annotations

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
    rows = []
    for r in runs:
        rows.append([
            r["id"], r["kind"], r["agent"], r["name"] or "", r["status"], r["code"],
            spread(r["summary"], "score"),
        ])
    return table(["run", "kind", "agent", "name", "status", "code", "score"], rows)


def show(run: dict, curve: list[tuple[int, float]], patch_exists: bool) -> str:
    out = [
        f"run       {run['id']}" + (f"  ({run['name']})" if run["name"] else ""),
        f"kind      {run['kind']} of {run['agent']} on {run['game']}, {run['status']}",
        f"code      {run['code']}" + ("  (uncommitted changes in code.patch)" if patch_exists else ""),
        f"rules     version {run['rules_version']}   versions {run['versions']}",
        f"started   {run['started']}   finished {run['finished'] or '-'}",
        f"dir       {run['dir']}",
    ]
    if run["parent"]:
        out.append(f"of        {run['parent']}")
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
    if curve:
        out += ["", "learning curve (eval score by training steps):", _curve(curve)]
    return "\n".join(out)


def compare(runs: list[dict]) -> str:
    game = runs[0]["game"]
    keys = headline_keys(game)
    rows = [[r["id"], r["kind"], r["agent"], r["name"] or "", r["code"], number(r["summary"].get("episodes")),
             *(spread(r["summary"], k) for k in keys)] for r in runs]
    out = [table(["run", "kind", "agent", "name", "code", "games", *(k.removeprefix("sum_") for k in keys)], rows)]

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
