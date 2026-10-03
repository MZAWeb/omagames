"""The experiment store: every run, what it ran, and how it did.

One SQLite file holds what you query and compare (runs, their settings, the
code they ran, training curves, every evaluation episode, and a summary of
each). A folder per run holds the files: the patch of uncommitted code,
checkpoints, replays. Both live under OMAGYM_HOME, by default
omagym/experiments/, which git ignores: results are local, the code that
produced them is in git.
"""

from __future__ import annotations

import json
import os
import shutil
import sqlite3
from datetime import datetime, timezone
from pathlib import Path

_SCHEMA = """
CREATE TABLE IF NOT EXISTS runs (
    id TEXT PRIMARY KEY,
    kind TEXT NOT NULL,            -- train | eval
    game TEXT NOT NULL,
    agent TEXT NOT NULL,
    name TEXT,                     -- your label for it
    notes TEXT,
    parent TEXT,                   -- for an eval of a trained agent: the training run
    agent_config TEXT NOT NULL,    -- JSON
    env_config TEXT NOT NULL,      -- JSON, as the env resolved it
    eval_episodes INTEGER,
    eval_max_steps INTEGER,
    train_steps INTEGER,
    seed INTEGER,
    device TEXT,
    code TEXT NOT NULL,            -- commit, plus the patch's hash when dirty
    commit_sha TEXT,
    dirty INTEGER NOT NULL,
    rules_version INTEGER,
    versions TEXT,                 -- JSON: python, numpy, torch
    started TEXT NOT NULL,
    finished TEXT,
    status TEXT NOT NULL,          -- running | done | failed | interrupted
    dir TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS metrics (run TEXT, step INTEGER, key TEXT, value REAL);
CREATE INDEX IF NOT EXISTS metrics_run ON metrics (run, key);
CREATE TABLE IF NOT EXISTS episodes (run TEXT, episode INTEGER, seed INTEGER, data TEXT);
CREATE TABLE IF NOT EXISTS summary (run TEXT, key TEXT, value REAL, PRIMARY KEY (run, key));
"""


def home() -> Path:
    return Path(os.environ.get("OMAGYM_HOME", Path(__file__).resolve().parents[2] / "experiments"))


def now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="seconds")


class Store:
    def __init__(self, root: Path | None = None):
        self.root = root or home()
        self.root.mkdir(parents=True, exist_ok=True)
        self.db = sqlite3.connect(self.root / "experiments.sqlite")
        self.db.row_factory = sqlite3.Row
        self.db.executescript(_SCHEMA)

    # -- writing --------------------------------------------------------------

    def new_run(self, kind: str, game: str, agent: str, **fields) -> dict:
        stamp = datetime.now().strftime("%Y%m%d-%H%M%S")
        run_id = f"{stamp}-{game}-{agent}"
        suffix = 1
        while self.db.execute("SELECT 1 FROM runs WHERE id = ?", (run_id,)).fetchone():
            suffix += 1
            run_id = f"{stamp}-{game}-{agent}-{suffix}"
        directory = self.root / "runs" / run_id
        directory.mkdir(parents=True)
        row = {
            "id": run_id, "kind": kind, "game": game, "agent": agent, "started": now(),
            "status": "running", "dir": str(directory), **fields,
        }
        for key in ("agent_config", "env_config", "versions"):
            if isinstance(row.get(key), dict):
                row[key] = json.dumps(row[key], sort_keys=True)
        columns = ", ".join(row)
        self.db.execute(f"INSERT INTO runs ({columns}) VALUES ({', '.join('?' * len(row))})", list(row.values()))
        self.db.commit()
        return self.run(run_id)

    def log(self, run_id: str, step: int, metrics: dict[str, float]) -> None:
        self.db.executemany(
            "INSERT INTO metrics VALUES (?, ?, ?, ?)",
            [(run_id, step, k, float(v)) for k, v in metrics.items() if k != "steps"],
        )
        self.db.commit()

    def add_episodes(self, run_id: str, episodes: list[dict]) -> None:
        self.db.executemany(
            "INSERT INTO episodes VALUES (?, ?, ?, ?)",
            [(run_id, e["episode"], e["seed"], json.dumps(e)) for e in episodes],
        )
        self.db.commit()

    def set_summary(self, run_id: str, summary: dict[str, float]) -> None:
        self.db.executemany(
            "INSERT OR REPLACE INTO summary VALUES (?, ?, ?)", [(run_id, k, float(v)) for k, v in summary.items()]
        )
        self.db.commit()

    def finish(self, run_id: str, status: str) -> None:
        self.db.execute("UPDATE runs SET status = ?, finished = ? WHERE id = ?", (status, now(), run_id))
        self.db.commit()

    def annotate(self, run_id: str, name: str | None = None, notes: str | None = None) -> None:
        if name is not None:
            self.db.execute("UPDATE runs SET name = ? WHERE id = ?", (name, run_id))
        if notes is not None:
            self.db.execute("UPDATE runs SET notes = ? WHERE id = ?", (notes, run_id))
        self.db.commit()

    def delete(self, run_id: str) -> None:
        """Forgets a run: its rows, and its folder of checkpoints and replays."""
        with self.db:
            for table in ("metrics", "episodes", "summary"):
                self.db.execute(f"DELETE FROM {table} WHERE run = ?", (run_id,))
            row = self.db.execute("DELETE FROM runs WHERE id = ? RETURNING dir", (run_id,)).fetchone()
        directory = Path(row["dir"]) if row else None
        # Only ever a folder this store made, whatever the database says.
        if directory and directory.parent == self.root / "runs" and directory.is_dir():
            shutil.rmtree(directory)

    # -- reading --------------------------------------------------------------

    def run(self, ref: str) -> dict:
        """A run by id, by a unique prefix of its id, by name, or "last"."""
        if ref == "last":
            row = self.db.execute("SELECT * FROM runs ORDER BY started DESC, id DESC LIMIT 1").fetchone()
        else:
            rows = self.db.execute(
                "SELECT * FROM runs WHERE id = ? OR id LIKE ? OR name = ? ORDER BY started DESC",
                (ref, ref + "%", ref),
            ).fetchall()
            exact = [r for r in rows if r["id"] == ref]
            if len(rows) > 1 and not exact:
                ids = ", ".join(r["id"] for r in rows[:5])
                raise KeyError(f"{ref!r} could be any of: {ids}")
            row = exact[0] if exact else (rows[0] if rows else None)
        if row is None:
            raise KeyError(f"no run {ref!r}; see `omagym runs`")
        run = dict(row)
        for key in ("agent_config", "env_config", "versions"):
            run[key] = json.loads(run[key]) if run.get(key) else {}
        run["summary"] = self.summary(run["id"])
        return run

    def runs(self, game: str | None = None, agent: str | None = None, kind: str | None = None,
             limit: int | None = 30):
        query, params = "SELECT id FROM runs WHERE 1 = 1", []
        for column, value in (("game", game), ("agent", agent), ("kind", kind)):
            if value:
                query += f" AND {column} = ?"
                params.append(value)
        query += " ORDER BY started DESC, id DESC"
        if limit is not None:
            query += " LIMIT ?"
            params.append(limit)
        return [self.run(r["id"]) for r in self.db.execute(query, params)]

    def evaluations_of(self, run_id: str) -> list[str]:
        """Runs that tested this training run's checkpoint."""
        return [r["id"] for r in self.db.execute("SELECT id FROM runs WHERE parent = ? ORDER BY id", (run_id,))]

    def summary(self, run_id: str) -> dict[str, float]:
        rows = self.db.execute("SELECT key, value FROM summary WHERE run = ?", (run_id,))
        return {r["key"]: r["value"] for r in rows}

    def episodes(self, run_id: str) -> list[dict]:
        rows = self.db.execute("SELECT data FROM episodes WHERE run = ? ORDER BY episode", (run_id,))
        return [json.loads(r["data"]) for r in rows]

    def trained_steps(self, run_id: str) -> int | None:
        """Steps a training run actually took: its budget, unless it stopped early."""
        row = self.db.execute("SELECT MAX(step) FROM metrics WHERE run = ? AND key LIKE 'train/%'", (run_id,)).fetchone()
        return row[0]

    def series(self, run_id: str, key: str) -> list[tuple[int, float]]:
        rows = self.db.execute("SELECT step, value FROM metrics WHERE run = ? AND key = ? ORDER BY step", (run_id, key))
        return [(r["step"], r["value"]) for r in rows]

    def metric_keys(self, run_id: str) -> list[str]:
        rows = self.db.execute("SELECT DISTINCT key FROM metrics WHERE run = ? ORDER BY key", (run_id,))
        return [r["key"] for r in rows]
