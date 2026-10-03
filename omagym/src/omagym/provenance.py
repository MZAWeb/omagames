"""What code a run ran, recorded so its result can always be traced back.

The checkout's commit, and when the tree has changes not yet committed, a
patch of them (new files included) saved beside the run. `git checkout
<commit> && git apply code.patch` gets the exact code back. A run's "code" is
the commit, plus "+" and the patch's hash when there was one, so two runs
on the same code say so at a glance in `omagym compare`.
"""

from __future__ import annotations

import hashlib
import platform
import subprocess
from importlib import metadata
from pathlib import Path

from .native import REPO_ROOT


def _git(*args: str, check: bool = True) -> str:
    result = subprocess.run(["git", "-C", str(REPO_ROOT), *args], capture_output=True, text=True)
    if check and result.returncode != 0:
        raise RuntimeError(f"git {' '.join(args)}: {result.stderr.strip()}")
    return result.stdout


def snapshot(run_dir: Path) -> dict:
    """{code, commit_sha, dirty}, writing code.patch into run_dir when dirty."""
    try:
        commit = _git("rev-parse", "HEAD").strip()
    except (RuntimeError, FileNotFoundError):
        return {"code": "unknown", "commit_sha": None, "dirty": 0}
    patch = _git("diff", "HEAD", "--binary")
    for path in _git("ls-files", "--others", "--exclude-standard").splitlines():
        # `git diff --no-index` exits 1 when the files differ, which they do.
        patch += _git("diff", "--no-index", "--binary", "/dev/null", path, check=False)
    if not patch:
        return {"code": commit[:10], "commit_sha": commit, "dirty": 0}
    (run_dir / "code.patch").write_text(patch)
    digest = hashlib.sha256(patch.encode()).hexdigest()[:8]
    return {"code": f"{commit[:10]}+{digest}", "commit_sha": commit, "dirty": 1}


def versions() -> dict:
    found = {"python": platform.python_version()}
    for package in ("numpy", "torch"):
        try:
            found[package] = metadata.version(package)
        except metadata.PackageNotFoundError:
            pass
    return found
