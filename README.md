# Omagames

[![CI](https://github.com/MZAWeb/omagames/actions/workflows/ci.yml/badge.svg)](https://github.com/MZAWeb/omagames/actions/workflows/ci.yml)

Small, polished games for [Omarchy](https://omarchy.org), built with Qt Quick
and C++ in the spirit of `omawrite` and `omacalc`: dead simple, keyboard first,
and automatically following the Omarchy theme, system dark/light mode and text
size.

| Game | Directory | What |
|---|---|---|
| **Omadoku** | `games/omadoku` | Sudoku with generated puzzles, notes mode and optional instant checking |
| **Black Omack** | `games/blackomack` | Casino blackjack with a persistent Omabucks bankroll and AI table mates |

Each game has its own README with rules and keyboard shortcuts.

This is a monorepo: games live under `games/`, everything shared (theming,
fonts, app bootstrap, common QML controls) lives under `common/` and is compiled
into each game. Each game ships as its own Arch package.

## Build & run

```sh
bin/build            # build every game -> build/games/<game>/<game>
bin/build omadoku    # build one game
bin/run omadoku      # build + launch
bin/test             # run all QtTest suites headlessly (bin/test omadoku for one)
bin/install omadoku  # build + makepkg -fsi the Arch package
```

Requirements: Qt 6 (`qt6-base`, `qt6-declarative`), `xdg-desktop-portal` and a
portal backend for live dark-mode / text-scale updates.

Pass `--theme-file path/to/colors.toml` to any game to preview it in another
Omarchy theme.

## Layout

```
common/        shared C++ (OmarchyTheme, SystemTheme, OmaGames::setupApplication), QML module `OmaGames`, fonts
games/<name>/  <name>.pro, src/ (engine + QML), tests/, pkgbuild/, README.md
docs/          ARCHITECTURE.md, NEW-GAME.md, PARALLEL-AGENTS.md
bin/           build / run / test / install
```

## Contributing

- `docs/ARCHITECTURE.md` — how a game is structured and why.
- `docs/NEW-GAME.md` — step-by-step guide to add a game.
- `CLAUDE.md` — the rules every change must follow (humans and agents alike).
- `docs/PARALLEL-AGENTS.md` — running several coding agents at once with git worktrees.

The iA Writer Mono font is bundled under the SIL Open Font License 1.1; see
`common/fonts/OFL.txt`. MIT licensed, see `LICENSE`.
