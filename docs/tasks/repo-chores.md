# Task C (optional) — Repo chores: common tests, packaging, CI

Not UI polish — the visual design pass on the games happens later, after A and B are merged.

Independent of A and B; can run alongside them. Do **not** edit files under
`games/*/src` or `games/*/tests` — those belong to agents A and B.

1. **common/ tests**: add `common/tests/` (`tests.pro` + `tst_common.cpp`)
   covering `OmarchyTheme`: parses a temp `colors.toml` (quoted values, comments),
   falls back to defaults, detects light/dark from `mode` and from background
   luminance, `mix`/`alpha` math. Wire it into `bin/test` (treat `common` like a game).
2. **Packaging check**: verify each `games/<game>/pkgbuild/PKGBUILD` builds with
   `makepkg -f` (no install) once A/B have added their icons; fix paths if needed.
   Consider a `bin/package [game]` that produces `*.pkg.tar.zst` into `dist/`.
3. **CI**: `.github/workflows/ci.yml` on an Arch container: install
   `qt6-base qt6-declarative`, run `bin/build` and `bin/test`.
4. **README**: screenshots section placeholders (`screenshots/<game>.png`),
   per-game install lines, contributing notes pointing at `CLAUDE.md`.
5. Report what you changed and how you verified it.
