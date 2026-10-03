"""omagym: train agents on omagames, test them, and compare what you tried.

    uv run omagym agents
    uv run omagym eval --game omatris --agent greedy
    uv run omagym train --game omatris --agent dqn --steps 100000
    uv run omagym compare --game omatris      # each agent's best run, ranked
    uv run omagym compare greedy-baseline last
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from . import native, provenance, records, report
from .agents import UNAVAILABLE, all_agents, make_config, resolve
from .env import Env
from .evaluation import evaluate
from .games import defaults
from .store import Store
from .training import Schedule, train


def main(argv: list[str] | None = None) -> int:
    args = _parser().parse_args(argv)
    try:
        return args.command(args) or 0
    except (KeyError, ValueError, native.EnvError) as error:
        print(f"omagym: {error.args[0] if error.args else error}", file=sys.stderr)
        return 2


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="omagym", description=__doc__.split("\n")[0])
    commands = parser.add_subparsers(required=True, metavar="command")

    def command(name, function, help):
        sub = commands.add_parser(name, help=help, description=help)
        sub.set_defaults(command=function)
        return sub

    command("agents", _agents, "list the agents and their settings")

    for name, function, help in (
        ("train", _train, "train an agent, evaluating as it goes; recorded as a run"),
        ("eval", _eval, "test an agent on the fixed evaluation games; recorded as a run"),
    ):
        sub = command(name, function, help)
        sub.add_argument("--game", default="omatris", choices=native.games())
        sub.add_argument("--agent", help="agent name (see `omagym agents`)")
        sub.add_argument("--set", action="append", default=[], metavar="KEY=VALUE", help="an agent setting")
        sub.add_argument("--env", action="append", default=[], metavar="KEY=VALUE", help="an env setting, e.g. mode=sprint")
        sub.add_argument("--episodes", type=int, help="evaluation games (default per game)")
        sub.add_argument("--max-steps", type=int, help="steps an evaluation game is cut at (default per game)")
        sub.add_argument("--name", help="a label for the run, usable wherever a run id is")
        sub.add_argument("--notes", help="what you were trying")
        sub.add_argument("--seed", type=int, default=0)
        sub.add_argument("--device", default="auto", help="auto, cpu or cuda")
    train_parser = commands.choices["train"]
    train_parser.add_argument("--steps", type=int, default=100_000, help="env steps to train for")
    train_parser.add_argument("--eval-every", type=int, help="steps between quick evaluations (default steps/10)")
    train_parser.add_argument("--eval-episodes", type=int, default=5, help="games in each quick evaluation")
    train_parser.add_argument("--snapshots", type=int, default=10,
                              help="games recorded along the way, from step 0 to the end, for `watch --training`")
    commands.choices["eval"].add_argument("--run", help="test the best checkpoint of this training run")

    sub = command("runs", records.runs, "list recorded runs, newest first")
    sub.add_argument("--game")
    sub.add_argument("--agent")
    sub.add_argument("--kind", choices=("train", "eval"))
    sub.add_argument("--limit", type=int, default=30)

    sub = command("models", records.models, "list trained models: training runs with a checkpoint")
    sub.add_argument("--game")
    sub.add_argument("--agent")

    sub = command("show", records.show, "everything about one run, or one trained model")
    sub.add_argument("run", help="run id, a unique prefix of one, its name, or 'last'")

    sub = command("compare", records.compare, "which runs played best, and whether the difference is real")
    sub.add_argument("runs", nargs="*", help="runs to rank (default: each agent's best run of --game)")
    sub.add_argument("--game", default="omatris", choices=native.games(), help="the game, when no runs are given")
    sub.add_argument("--by", default="score", metavar="METRIC", help="what to rank by: score, steps, lines, ate...")
    sub.add_argument("--lower", action="store_true", help="lower is better (holes, max_height...)")

    sub = command("diff", records.diff, "runs side by side, with the settings that differ between them")
    sub.add_argument("runs", nargs="+")

    sub = command("watch", records.watch, "play a run's best (or worst) evaluation game in the real app")
    sub.add_argument("run")
    which = sub.add_mutually_exclusive_group()
    which.add_argument("--worst", action="store_true")
    which.add_argument("--training", action="store_true",
                       help="a training run's snapshots in order, stepped through with N and B")

    sub = command("delete", records.delete, "forget runs: their results, checkpoints and replays")
    sub.add_argument("runs", nargs="+")
    sub.add_argument("--yes", action="store_true", help="don't ask first")
    sub.add_argument("--with-tests", action="store_true",
                     help="a trained model goes with the runs that tested its checkpoint")

    sub = command("note", records.note, "name a run or write down what it was about")
    sub.add_argument("run")
    sub.add_argument("--name")
    sub.add_argument("--notes")
    return parser


def _pairs(items: list[str]) -> dict[str, str]:
    pairs = {}
    for item in items:
        key, sep, value = item.partition("=")
        if not sep:
            raise ValueError(f"expected KEY=VALUE, got {item!r}")
        pairs[key.strip()] = value.strip()
    return pairs


def _env_values(raw: dict[str, str]) -> dict:
    """Env settings typed the way the game's schema wants them."""
    values = {}
    for key, value in raw.items():
        if value.lower() in ("true", "false"):
            values[key] = value.lower() == "true"
        else:
            try:
                values[key] = int(value)
            except ValueError:
                values[key] = value
    return values


def _device(choice: str) -> str:
    if choice != "auto":
        return choice
    try:
        import torch
    except ImportError:
        return "cpu"
    return "cuda" if torch.cuda.is_available() else "cpu"


def _agents(args) -> None:
    for cls in all_agents():
        games = ", ".join(cls.games) or "any game"
        learns = "  (learns)" if cls.trainable else ""
        print(f"{cls.name} [{games}]{learns}\n    {cls.description}")
        settings = [f"{f}={getattr(cls.Config(), f)}" for f in cls.Config.__dataclass_fields__]
        if settings:
            print("    settings: " + ", ".join(settings))
    for module, reason in UNAVAILABLE.items():
        print(f"{module}: unavailable, {reason}")


def _start(args, kind: str, store: Store, agent_name: str, overrides: dict, parent: str | None = None):
    """Builds the agent, resolves the env config and opens the run."""
    cls = resolve(agent_name, args.game)
    config = make_config(cls, overrides)
    env_config = {**cls.env_config(args.game), **_env_values(_pairs(args.env))}
    with Env(args.game, **env_config) as probe:
        spec = probe.spec
    resolved_env = {k: v for k, v in spec.config.items() if k != "max_steps"}
    game_defaults = defaults(args.game)
    device = _device(args.device)
    run = store.new_run(
        kind, args.game, cls.name, name=args.name, notes=args.notes, parent=parent,
        agent_config=vars(config), env_config=resolved_env, seed=args.seed, device=device,
        eval_episodes=args.episodes or game_defaults.eval_episodes,
        eval_max_steps=args.max_steps or game_defaults.eval_max_steps,
        train_steps=getattr(args, "steps", None), rules_version=spec.rules_version,
        versions=provenance.versions(), code="pending", dirty=0,
    )
    code = provenance.snapshot(Path(run["dir"]))
    store.db.execute("UPDATE runs SET code = ?, commit_sha = ?, dirty = ? WHERE id = ?",
                     (code["code"], code["commit_sha"], code["dirty"], run["id"]))
    store.db.commit()
    if code["dirty"]:
        print(f"note: uncommitted changes recorded in {run['dir']}/code.patch")
    return store.run(run["id"]), cls(config, spec, device)


def _train(args) -> int:
    if not args.agent:
        raise ValueError("--agent is required")
    store = Store()
    run, agent = _start(args, "train", store, args.agent, _pairs(args.set))
    if not agent.trainable:
        store.finish(run["id"], "failed")
        raise ValueError(f"{agent.name} does not learn; use `omagym eval` for it")
    schedule = Schedule(
        steps=args.steps, eval_every=args.eval_every or max(1, args.steps // 10),
        eval_episodes=args.eval_episodes, final_episodes=run["eval_episodes"], max_steps=run["eval_max_steps"],
        snapshots=args.snapshots,
    )
    print(f"run {run['id']}: training {agent.name} on {args.game} for {args.steps:,} steps on {run['device']}")
    try:
        status = train(store, run, agent, schedule)
    except Exception:
        store.finish(run["id"], "failed")
        raise
    print(f"\n{status}.\n")
    trained = store.run(run["id"])
    print(report.show(trained, store.series(run["id"], "eval/score_mean"), False,
                      records.model(store, trained), records.checkpoints(trained)))
    return 0


def _eval(args) -> int:
    store = Store()
    parent = store.run(args.run) if args.run else None
    if parent:
        if parent["game"] != args.game and args.game != "omatris":
            raise ValueError(f"{parent['id']} trained on {parent['game']}, not {args.game}")
        args.game = parent["game"]
        overrides = {k: str(v) for k, v in parent["agent_config"].items()}
        overrides.update(_pairs(args.set))
        args.env = [f"{k}={v}" for k, v in parent["env_config"].items()] + args.env
        run, agent = _start(args, "eval", store, parent["agent"], overrides, parent=parent["id"])
        checkpoint = Path(parent["dir"]) / "best.pt"
        if not checkpoint.exists():
            raise ValueError(f"{parent['id']} has no checkpoint to test")
        agent.load(checkpoint)
    elif args.agent:
        run, agent = _start(args, "eval", store, args.agent, _pairs(args.set))
    else:
        raise ValueError("give --agent, or --run to test a trained agent")

    print(f"run {run['id']}: {run['eval_episodes']} games of {args.game}, cut at {run['eval_max_steps']} steps")
    label = run["name"] or run["id"]

    def progress(i, result):
        print(f"  game {i + 1:>3}: score {report.number(result['score']):>10}  {result['steps']:>6} steps", flush=True)

    try:
        episodes, summary = evaluate(agent, args.game, run["env_config"], run["eval_episodes"],
                                     run["eval_max_steps"], Path(run["dir"]) / "replays", label, progress)
    except BaseException:
        store.finish(run["id"], "failed")
        raise
    store.add_episodes(run["id"], episodes)
    store.set_summary(run["id"], summary)
    store.finish(run["id"], "done")
    print()
    print(report.show(store.run(run["id"]), [], False))
    return 0


if __name__ == "__main__":
    sys.exit(main())
