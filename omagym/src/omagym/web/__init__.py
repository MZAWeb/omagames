"""`omagym web`: a read-only browser view of the experiment store.

A small JSON API over the store (runs, one run in full, comparisons, replays
drawn as frames) and the single page in static/ that shows it. Standard
library only: http.server, one thread per request, each with its own view
of the SQLite store. Nothing here writes to the store.
"""

from __future__ import annotations

import json
import subprocess
import threading
import webbrowser
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse

from .. import comparison, native, records
from ..games import defaults
from ..store import Store
from ..training import snapshot_files

STATIC = Path(__file__).parent / "static"
_TYPES = {".html": "text/html", ".js": "text/javascript", ".css": "text/css", ".svg": "image/svg+xml"}

# Drawing a long replay takes a moment and they never change, so they're kept.
_frames: dict[str, dict] = {}
_frames_lock = threading.Lock()


def serve(port: int, open_browser: bool) -> None:
    server = ThreadingHTTPServer(("127.0.0.1", port), _Handler)
    url = f"http://127.0.0.1:{port}/"
    print(f"omagym web: {url}  (Ctrl+C to stop)")
    if open_browser:
        webbrowser.open(url)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print()
    finally:
        server.server_close()


class _Handler(BaseHTTPRequestHandler):
    def log_message(self, format, *args):  # noqa: A002 - the base class's name
        pass  # quiet: the page asks for a lot, and none of it is news

    def do_GET(self) -> None:
        url = urlparse(self.path)
        query = {k: v[-1] for k, v in parse_qs(url.query).items()}
        try:
            if url.path.startswith("/api/"):
                self._json(_route(url.path.removeprefix("/api/"), query))
            else:
                self._static(url.path)
        except (KeyError, ValueError, native.EnvError) as error:
            self._json({"error": str(error.args[0] if error.args else error)}, HTTPStatus.BAD_REQUEST)

    def _json(self, body, status: HTTPStatus = HTTPStatus.OK) -> None:
        data = json.dumps(body, default=_plain).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def _static(self, path: str) -> None:
        file = (STATIC / (path.lstrip("/") or "index.html")).resolve()
        if STATIC.resolve() not in file.parents or not file.is_file():
            file = STATIC / "index.html"
        data = file.read_bytes()
        self.send_response(HTTPStatus.OK)
        self.send_header("Content-Type", _TYPES.get(file.suffix, "application/octet-stream"))
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)


def _plain(value):
    """What json can't write by itself: numpy numbers, mostly."""
    return value.item() if hasattr(value, "item") else str(value)


def _route(path: str, query: dict):
    store = Store()
    if path == "runs":
        return [_listing(store, run) for run in store.runs(limit=None)]
    if path == "groups":
        return [_group(store, name) for name in store.groups()]
    if path == "games":
        return _games(store)
    if path.startswith("run/"):
        return _details(store, store.run(path.removeprefix("run/")))
    if path == "compare":
        return _compare(store, [r for r in query.get("runs", "").split(",") if r], query)
    if path == "frames":
        return _replay_frames(store.run(query["run"]), query["file"])
    raise KeyError(f"no such API: {path}")


# -- what the pages need -----------------------------------------------------


def _tests(game: str) -> list[dict]:
    """The game's tests, main first: name, what it measures, which way is better."""
    d = defaults(game)
    return [{"name": d.main_test, "prefix": "", "metric": "score", "lower": False, "about": "the main test",
             "episodes": d.eval_episodes, "max_steps": d.eval_max_steps}] + [
        {"name": t.name, "prefix": f"{t.name}/", "metric": t.metric, "lower": t.lower_is_better, "about": t.about,
         "episodes": t.episodes, "max_steps": t.max_steps} for t in d.tests]


def _results(run: dict) -> dict:
    """Each test's headline numbers, from the run's summary."""
    s, out = run["summary"], {}
    for test in _tests(run["game"]):
        p = test["prefix"]
        if f"{p}episodes" not in s:
            continue
        key = comparison.metric_key(test["metric"], (k.removeprefix(p) for k in s if k.startswith(p)))
        out[test["name"]] = {
            "metric": test["metric"], "lower": test["lower"],
            "mean": s.get(f"{p}{key}_mean"), "std": s.get(f"{p}{key}_std"),
            "games": s[f"{p}episodes"], "won": s.get(f"{p}won_mean", 0) * s[f"{p}episodes"],
            "survived": s.get(f"{p}cut_short"), "score": s.get(f"{p}score_mean"),
            # The game's own measures, as the means over the test's games.
            "measures": {k: s.get(f"{p}{k}_mean") for k, _, _ in defaults(run["game"]).style},
        }
    return out


def _games(store: Store) -> list[dict]:
    """The games that have runs, with what the page needs to show them:
    their tests, their measures, and which of those Rankings has a column for."""
    names = sorted({r[0] for r in store.db.execute("SELECT DISTINCT game FROM runs")})
    out = []
    for name in names:
        d = defaults(name)
        out.append({"name": name, "main_test": d.main_test, "tests": _tests(name),
                    "measures": [{"key": k, "label": label, "lower": lower} for k, label, lower in d.style],
                    "ranking": list(d.ranking)})
    return out


def _listing(store: Store, run: dict) -> dict:
    return {
        "id": run["id"], "name": run["name"], "notes": run["notes"], "agent": run["agent"], "game": run["game"],
        "status": run["status"], "started": run["started"], "finished": run["finished"], "code": run["code"],
        "trained_steps": store.trained_steps(run["id"]) if run["kind"] == "train" else None,
        "train_mix": run.get("train_mix") or {}, "results": _results(run),
        "tags": run.get("tags", []), "group": run.get("group_name"),
    }


def _group(store: Store, name: str) -> dict:
    """A group of seeds as one row: their mean results, and who is in it."""
    members = store.group(name)
    for member in members:
        member["trained_steps"] = store.trained_steps(member["id"]) if member["kind"] == "train" else None
    run, _ = comparison.combine(name, members, [[] for _ in members])
    spread = {}
    for test in _tests(run["game"]):
        prefix = test["prefix"]
        key = prefix + comparison.metric_key(test["metric"], (k.removeprefix(prefix) for k in run["summary"]
                                                              if k.startswith(prefix))) + "_mean"
        values = [m["summary"][key] for m in members if key in m["summary"]]
        if values:
            mean = sum(values) / len(values)
            spread[test["name"]] = (sum((v - mean) ** 2 for v in values) / len(values)) ** 0.5
    return {**_listing(store, run), "id": run["id"], "name": name, "seeds": len(members),
            "members": [m["id"] for m in members], "trained_steps": run["trained_steps"], "seed_spread": spread}


def _details(store: Store, run: dict) -> dict:
    folder = Path(run["dir"])
    patch = folder / "code.patch"
    curves = {key: store.series(run["id"], key) for key in store.metric_keys(run["id"])}
    replays = []
    for test in _tests(run["game"]):
        where = folder / "replays" / test["name"] if test["prefix"] else folder / "replays"
        for which in ("best", "worst"):
            if (where / f"{which}.json").exists():
                replays.append({"test": test["name"], "which": which, "file": str((where / f"{which}.json").relative_to(folder))})
    games = {}
    for test in _tests(run["game"]):
        where = (folder / "replays" / test["name"]) if test["prefix"] else (folder / "replays")
        games[test["name"]] = sorted(str(f.relative_to(folder)) for f in (where / "games").glob("*.json"))
    return {
        **_listing(store, run),
        "kind": run["kind"], "agent_config": run["agent_config"], "env_config": run["env_config"],
        "seed": run["seed"], "device": run["device"], "rules_version": run["rules_version"],
        "versions": run["versions"], "commit": run["commit_sha"], "dirty": bool(run["dirty"]),
        "commit_message": _commit_message(run["commit_sha"]),
        "patch": patch.read_text() if patch.exists() else "",
        "train_steps": run["train_steps"], "tests": _tests(run["game"]),
        "summary": run["summary"], "curves": curves,
        "episodes": {t["name"]: store.episodes(run["id"], t["name"] if t["prefix"] else "") for t in _tests(run["game"])},
        "replays": replays, "games": games,
        "snapshots": [str(f.relative_to(folder)) for f in snapshot_files(run)],
        "checkpoints": [n for n in ("best.pt", "last.pt") if (folder / n).exists()],
    }


def _commit_message(sha: str | None) -> str:
    if not sha:
        return ""
    result = subprocess.run(["git", "-C", str(native.REPO_ROOT), "log", "-1", "--format=%s%n%n%b", sha],
                            capture_output=True, text=True)
    return result.stdout.strip() if result.returncode == 0 else ""


def _compare(store: Store, refs: list[str], query: dict) -> dict:
    if not refs:
        raise ValueError("pick some runs to compare")
    game = records.resolve(store, refs[0], "")[0]["game"]
    test = defaults(game).test(query.get("test"))
    metric = query.get("by") or (test.metric if test else "score")
    lower = query.get("lower") == "1" if "lower" in query else (test.lower_is_better if test and "by" not in query else False)
    name = test.name if test else ""
    # Runs, or groups of seeds (`group:<name>`), each with its games of the test.
    resolved = [records.resolve(store, ref, name) for ref in refs]
    runs, episodes = [r for r, _ in resolved], {r["id"]: e for r, e in resolved}
    ranking = comparison.rank(runs, episodes, metric, lower, name, test.max_steps if test else None)
    key = comparison.metric_key(metric, next((e for played in episodes.values() for e in played), None))
    per_game = {run["id"]: {e["seed"]: e.get(key) for e in episodes[run["id"]]} for run in runs}
    differing = {}
    for field in ("agent_config", "env_config", "train_mix"):
        keys = sorted({k for r in runs for k in (r.get(field) or {})})
        for k in keys:
            values = [(r.get(field) or {}).get(k) for r in runs]
            if len({json.dumps(v) for v in values}) > 1:
                differing[f"{field.split('_')[0]}.{k}" if field != "train_mix" else f"mix.{k}"] = values
    return {
        "test": name or defaults(game).main_test, "metric": metric, "lower": lower,
        "seeds": ranking.seeds, "cap": ranking.cap,
        "standings": [{
            "id": s.run["id"], "name": s.run["name"], "agent": s.run["agent"], "notes": s.run["notes"],
            "trained_steps": s.run["trained_steps"], "mean": s.mean, "std": s.std, "wins": s.wins, "ties": s.ties,
            "losses": s.losses, "difference": s.difference, "low": s.low, "high": s.high, "verdict": s.verdict,
        } for s in ranking.standings],
        "per_game": {rid: [values.get(seed) for seed in ranking.seeds] for rid, values in per_game.items()},
        "differing": differing,
        "runs": [r["id"] for r in runs],
        # How they played, beyond the ranked measure, in the ranking's order.
        "style": comparison.style([s.run for s in ranking.standings], name, defaults(game).style, main=test is None),
    }


def _replay_frames(run: dict, file: str) -> dict:
    folder = Path(run["dir"]).resolve()
    path = (folder / file).resolve()
    if folder not in path.parents or not path.is_file():
        raise KeyError(f"{run['id']} has no replay {file}")
    cache_key = str(path)
    with _frames_lock:
        if cache_key in _frames:
            return _frames[cache_key]
    frames = native.load(run["game"]).frames(json.loads(path.read_text()))
    with _frames_lock:
        _frames[cache_key] = frames
    return frames
