"""The commands on runs already recorded: list, show, compare, watch, name, delete.

A run of an agent that learns trained first, and keeps what it learned (its
checkpoints and snapshots) in its folder; `runs --trained` lists those.
"""

from __future__ import annotations

import subprocess
from pathlib import Path

from . import comparison, native, report
from .games import defaults
from .store import Store
from .training import snapshot_files


def runs(args) -> None:
    store = Store()
    found = store.runs(args.game, args.agent, "train" if args.trained else None, args.limit, args.tag)
    for run in found:
        run["trained_steps"] = store.trained_steps(run["id"])
    print(report.runs_table(found) if found else "no runs yet: try `omagym run --agent greedy`")


def details(store: Store, run: dict) -> str:
    """Everything `show` prints about a run."""
    trained = None
    if run["kind"] == "train":
        trained = {"steps": store.trained_steps(run["id"]), "snapshots": len(snapshot_files(run)),
                   "checkpoints": [n for n in ("best.pt", "last.pt") if (Path(run["dir"]) / n).exists()]}
    return report.show(run, store.series(run["id"], "eval/score_mean"),
                       (Path(run["dir"]) / "code.patch").exists(), trained, defaults(run["game"]).tests)


def show(args) -> None:
    store = Store()
    print(details(store, store.run(args.run)))


def resolve(store: Store, ref: str, test: str) -> tuple[dict, list[dict]]:
    """A run, or a group of seeds (by its group name, or `group:<name>`),
    with its games of `test` ("" for the main one)."""
    name = ref.removeprefix("group:")
    if not ref.startswith("group:"):
        try:
            run = store.run(ref)
            run["trained_steps"] = _trained_steps(store, run)
            return run, store.episodes(run["id"], test)
        except KeyError:
            if name not in store.groups():
                raise
    members = store.group(name)
    if not members:
        raise KeyError(f"no run or group {ref!r}; see `omagym runs`")
    for member in members:
        member["trained_steps"] = _trained_steps(store, member)
    return comparison.combine(name, members, [store.episodes(m["id"], test) for m in members])


def compare(args) -> None:
    store = Store()
    game = resolve(store, args.runs[0], "")[0]["game"] if args.runs else args.game
    test = defaults(game).test(args.test)
    # The test says what better means there, unless --by says otherwise.
    metric = args.by or (test.metric if test else "score")
    lower = args.lower or (test is not None and args.by is None and test.lower_is_better)
    name = test.name if test else ""
    if args.runs:
        resolved = [resolve(store, ref, name) for ref in args.runs]
        runs, episodes = [r for r, _ in resolved], {r["id"]: e for r, e in resolved}
    else:
        runs = _best_per_agent(store, game, metric, lower, name)
        for run in runs:
            run["trained_steps"] = _trained_steps(store, run)
        episodes = {run["id"]: store.episodes(run["id"], name) for run in runs}
    ranking = comparison.rank(runs, episodes, metric, lower, name, test.max_steps if test else None)
    print(report.ranking(ranking))
    ordered = [s.run for s in ranking.standings]
    rows = comparison.style(ordered, name, defaults(game).style, main=test is None)
    if rows:
        print()
        print(report.style_table(ordered, rows))


def _best_per_agent(store: Store, game: str, metric: str, lower_is_better: bool, test: str = "") -> list[dict]:
    """Each agent's best finished run on the game's default evaluation games."""
    prefix = f"{test}/" if test else ""
    recorded = {k.removeprefix(prefix) for r in store.runs(game=game, limit=None) for k in r["summary"] if k.startswith(prefix)}
    key = prefix + f"{comparison.metric_key(metric, recorded)}_mean"
    sign = -1.0 if lower_is_better else 1.0
    standard = defaults(game)
    best: dict[str, dict] = {}
    for run in store.runs(game=game, limit=None):
        if (run["status"] in ("done", "interrupted") and key in run["summary"]
                and run["eval_max_steps"] == standard.eval_max_steps
                and run["summary"]["episodes"] >= standard.eval_episodes):
            held = best.get(run["agent"])
            if held is None or sign * run["summary"][key] > sign * held["summary"][key]:
                best[run["agent"]] = run
    if not best:
        raise ValueError(f"no finished {game} runs on the default evaluation games yet; "
                         "try `omagym run --agent greedy`")
    return list(best.values())


def _trained_steps(store: Store, run: dict) -> int | None:
    return store.trained_steps(run["id"]) if run["kind"] == "train" else None


def diff(args) -> None:
    store = Store()
    runs = [store.run(ref) for ref in args.runs]
    if len({r["game"] for r in runs}) > 1:
        raise ValueError("those runs are of different games")
    for run in runs:
        run["trained_steps"] = _trained_steps(store, run)
    print(report.diff(runs))


def watch(args) -> int:
    run = Store().run(args.run)
    if args.training:
        replays = snapshot_files(run)
        if not replays:
            raise ValueError(f"{run['id']} recorded no snapshots (only training runs do)")
        print(f"watching {len(replays)} snapshots of {run['id']}: N and B step between them")
    else:
        test = defaults(run["game"]).test(args.test)
        folder = Path(run["dir"]) / "replays" / (test.name if test else "")
        replays = [folder / ("worst.json" if args.worst else "best.json")]
        if not replays[0].exists():
            raise ValueError(f"{run['id']} has no replays yet")
        print(f"watching {replays[0]}")
    command = [str(native.REPO_ROOT / "bin" / "run"), run["game"]]
    for replay in replays:
        command += ["--replay", str(replay)]
    return subprocess.call(command)


def delete(args) -> int:
    store = Store()
    runs = {run["id"]: run for run in (store.run(ref) for ref in args.runs)}
    print("\n".join(f"  {run_id}" + (f"  ({run['name']})" if run["name"] else "") for run_id, run in runs.items()))
    if not args.yes and not _confirm(f"delete {len(runs)} run(s) and their files? [y/N] "):
        print("nothing deleted")
        return 1
    for run_id in runs:
        store.delete(run_id)
    print(f"deleted {len(runs)} run(s)")
    return 0


def _confirm(question: str) -> bool:
    try:
        return input(question).strip().lower() in ("y", "yes")
    except EOFError:
        # No one to ask (a script, a pipe): the safe answer.
        return False


def tag(args) -> None:
    store = Store()
    for ref in args.runs:
        run = store.run(ref)
        tags = store.tag(run["id"], args.add, args.remove)
        print(f"{run['name'] or run['id']}: {', '.join(tags) or 'no tags'}")


def note(args) -> None:
    store = Store()
    run = store.run(args.run)
    store.annotate(run["id"], args.name, args.notes)
    print(f"{run['id']} noted")
