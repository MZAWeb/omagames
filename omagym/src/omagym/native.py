"""A game's agent environment library, loaded through ctypes.

Each game builds `build-env/<game>/lib<game>_env.so` with `bin/build-env`, and
every library exports the same C functions (common/env/omagames_env.h in the
omagames repo). This module is the only place that knows about that ABI;
everything above it talks to `omagym.env.Env`.
"""

from __future__ import annotations

import ctypes
import functools
import json
import os
import subprocess
from pathlib import Path

ABI_VERSION = 1
MAX_SIGNALS = 16

# omagym/src/omagym/native.py -> the omagames checkout this lab lives in.
REPO_ROOT = Path(os.environ.get("OMAGAMES_ROOT", Path(__file__).resolve().parents[3]))

_P = ctypes.c_void_p


class StepResult(ctypes.Structure):
    _fields_ = [
        ("reward", ctypes.c_double),
        ("terminated", ctypes.c_int32),
        ("truncated", ctypes.c_int32),
        ("ticks", ctypes.c_int64),
        ("signal_values", ctypes.c_double * MAX_SIGNALS),
    ]


class EnvError(RuntimeError):
    """A call the library refused, with its reason."""


def library_path(game: str) -> Path:
    return REPO_ROOT / "build-env" / game / f"lib{game}_env.so"


def build(game: str) -> None:
    """Brings the library up to date with the C++ it is built from.

    make does nothing when nothing changed, so this is cheap, and it means a
    run always plays the engine as the checkout has it: the code recorded with
    a run is the code that ran.
    """
    result = subprocess.run(
        [str(REPO_ROOT / "bin" / "build-env"), game], capture_output=True, text=True
    )
    if result.returncode != 0:
        tail = "\n".join((result.stdout + result.stderr).splitlines()[-30:])
        raise EnvError(f"bin/build-env {game} failed:\n{tail}")


class Library:
    """One game's library, its functions declared."""

    def __init__(self, game: str, path: Path):
        self.game = game
        self.path = path
        lib = ctypes.CDLL(str(path))
        self._lib = lib

        lib.og_abi_version.restype = ctypes.c_int
        for name in ("og_game_spec", "og_last_error"):
            getattr(lib, name).restype = ctypes.c_char_p
        for name in ("og_env_spec", "og_replay_json", "og_info_json"):
            getattr(lib, name).restype = ctypes.c_char_p
            getattr(lib, name).argtypes = [_P]
        lib.og_create.restype = _P
        lib.og_create.argtypes = [ctypes.c_char_p]
        lib.og_destroy.argtypes = [_P]
        lib.og_reset.argtypes = [_P, ctypes.c_uint32]
        lib.og_step.restype = ctypes.c_int
        lib.og_step.argtypes = [_P, ctypes.c_int32, ctypes.POINTER(StepResult)]
        lib.og_observe.argtypes = [_P, _P]
        lib.og_action_mask.argtypes = [_P, _P]
        lib.og_clone.restype = _P
        lib.og_clone.argtypes = [_P, ctypes.c_int, ctypes.c_uint32]

        abi = lib.og_abi_version()
        if abi != ABI_VERSION:
            raise EnvError(f"{path} speaks ABI {abi}; omagym speaks {ABI_VERSION}. Rebuild one of them.")
        self.game_spec: dict = json.loads(lib.og_game_spec())

    def _error(self) -> str:
        return self._lib.og_last_error().decode()

    def create(self, config: dict) -> int:
        handle = self._lib.og_create(json.dumps(config).encode())
        if not handle:
            raise EnvError(f"{self.game}: {self._error()}")
        return handle

    def destroy(self, handle: int) -> None:
        self._lib.og_destroy(handle)

    def env_spec(self, handle: int) -> dict:
        return json.loads(self._lib.og_env_spec(handle))

    def reset(self, handle: int, seed: int) -> None:
        self._lib.og_reset(handle, seed)

    def step(self, handle: int, action: int, out: StepResult) -> None:
        if self._lib.og_step(handle, action, ctypes.byref(out)) != 0:
            raise EnvError(f"{self.game}: {self._error()}")

    def observe(self, handle: int, address: int) -> None:
        self._lib.og_observe(handle, address)

    def action_mask(self, handle: int, address: int) -> None:
        self._lib.og_action_mask(handle, address)

    def clone(self, handle: int, reseed_hidden: bool, seed: int) -> int:
        return self._lib.og_clone(handle, int(reseed_hidden), seed)

    def replay(self, handle: int) -> dict:
        return json.loads(self._lib.og_replay_json(handle))

    def info(self, handle: int) -> dict:
        return json.loads(self._lib.og_info_json(handle))


@functools.cache
def load(game: str) -> Library:
    """The game's library, built first unless OMAGYM_NO_BUILD is set; once a process."""
    if not os.environ.get("OMAGYM_NO_BUILD"):
        build(game)
    path = library_path(game)
    if not path.exists():
        raise EnvError(f"{path} does not exist; run bin/build-env {game}")
    return Library(game, path)


def games() -> list[str]:
    """Every game in the checkout that has an agent environment."""
    return sorted(p.parent.parent.name for p in (REPO_ROOT / "games").glob("*/env/env.pro"))
