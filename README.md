# Omagames

A collection of small, polished games for [Omarchy](https://omarchy.org), built with
Qt Quick and C++ in the same spirit as `omawrite` and `omacalc`: dead simple,
keyboard friendly, and automatically following the Omarchy theme and system
dark/light mode.

| Game | Directory | What |
|---|---|---|
| **Omadoku** | `games/omadoku` | Sudoku with easy/medium/hard puzzles generated on the fly, notes mode, optional instant validation |
| **Black Omack** | `games/blackomack` | Casino blackjack with a persistent Omabucks bankroll and up to five AI-driven table mates |

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
games/<name>/  <name>.pro, src/ (engine + QML), tests/, pkgbuild/
docs/          PLAN.md (architecture & decisions), tasks/ (agent briefs)
bin/           build / run / test / install
```

See `docs/PLAN.md` for the architecture and `CLAUDE.md` for the coding rules.

The iA Writer Mono font is bundled under the SIL Open Font License 1.1; see
`common/fonts/OFL.txt`.
