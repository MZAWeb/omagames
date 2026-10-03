"""The commands on runs already recorded: list, show, compare, watch, name, delete.

A trained model is a training run with a checkpoint: `models` lists those,
and every other command here takes one by its run id or name.
"""

from __future__ import annotations

import subprocess
from pathlib import Path

from . import comparison, native, report
from .games import defaults
from .store import Store
from .training import snapshot_files


def runs(args) -> None:
    runs = Store().runs(args.game, args.agent, args.kind, args.limit)
    print(report.runs_table(runs) if runs else "no runs yet: try `omagym eval --agent greedy`")


def models(args) -> None:
    store = Store()
    found = [model(store, run) for run in store.runs(args.game, args.agent, "train", limit=None)
             if checkpoints(run)]
    print(report.models_table(found) if found else
          "no trained models yet: try `omagym train --agent dqn`")


def model(store: Store, run: dict) -> dict:
    """A training run with what a model listing shows beside it."""
    return {**run, "trained_steps": store.trained_steps(run["id"]),
            "tests": [store.run(t) for t in store.evaluations_of(run["id"])],
            "snapshots": len(snapshot_files(run))}


def checkpoints(run: dict) -> list[str]:
    return [name for name in ("best.pt", "last.pt") if (Path(run["dir"]) / name).exists()]


def show(args) -> None:
    store = Store()
    run = store.run(args.run)
    curve = store.series(run["id"], "eval/score_mean")
    model_info = model(store, run) if run["kind"] == "train" else None
    print(report.show(run, curve, (Path(run["dir"]) / "code.patch").exists(), model_info, checkpoints(run)))


def compare(args) -> None:
    store = Store()
    runs = [store.run(ref) for ref in args.runs] if args.runs else _best_per_agent(store, args.game, args.by, args.lower)
    for run in runs:
        run["trained_steps"] = _trained_steps(store, run)
    episodes = {run["id"]: store.episodes(run["id"]) for run in runs}
    print(report.ranking(comparison.rank(runs, episodes, args.by, args.lower)))


def _best_per_agent(store: Store, game: str, metric: str, lower_is_better: bool) -> list[dict]:
    """Each agent's best finished run on the game's default evaluation games."""
    key = f"{comparison.metric_key(metric)}_mean"
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
                         "try `omagym eval --agent greedy`")
    return list(best.values())


def _trained_steps(store: Store, run: dict) -> int | None:
    """Steps of training behind a run: its own, or for a test of a checkpoint, the training run's."""
    if run["kind"] == "train":
        return store.trained_steps(run["id"])
    return store.trained_steps(run["parent"]) if run["parent"] else None


def diff(args) -> None:
    store = Store()
    runs = [store.run(ref) for ref in args.runs]
    if len({r["game"] for r in runs}) > 1:
        raise ValueError("those runs are of different games")
    print(report.diff(runs))


def watch(args) -> int:
    run = Store().run(args.run)
    if args.training:
        replays = snapshot_files(run)
        if not replays:
            raise ValueError(f"{run['id']} recorded no snapshots (only training runs do)")
        print(f"watching {len(replays)} snapshots of {run['id']}: N and B step between them")
    else:
        replays = [Path(run["dir"]) / "replays" / ("worst.json" if args.worst else "best.json")]
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
    for run_id in list(runs):
        orphans = [e for e in store.evaluations_of(run_id) if e not in runs]
        if orphans and args.with_tests:
            runs.update({e: store.run(e) for e in orphans})
        elif orphans:
            raise ValueError(f"{run_id}'s checkpoint was tested by {', '.join(orphans)}: "
                             "delete those too, or pass --with-tests")
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


def note(args) -> None:
    store = Store()
    run = store.run(args.run)
    store.annotate(run["id"], args.name, args.notes)
    print(f"{run['id']} noted")
