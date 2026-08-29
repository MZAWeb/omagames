# Working in parallel with git worktrees

One branch + one worktree per agent. Worktrees share the same `.git`, so
branches, commits and history are all visible from the main checkout, but each
worktree has its own files and its own untracked `build/` (gitignored), so an
agent's broken build never affects anyone else.

## One-time repo setup (you, in `~/code/omagames`)

```sh
cd ~/code/omagames
git add -A && git commit -m "Repo skeleton: shared theme library, game placeholders, docs"
# Optional but recommended: rename the default branch to match GitHub's default
git branch -m master main

# Create one worktree + branch per agent, as siblings of the main checkout
git worktree add -b omadoku    ../omagames-omadoku    main
git worktree add -b blackomack ../omagames-blackomack main
git worktree add -b chores     ../omagames-chores     main   # only if running agent C
git worktree list
```

Rules of thumb:
- Worktrees live **outside** the repo directory (`../omagames-<name>`), never inside it.
- `main` stays untouched while agents work; you only merge into it.
- Never open the same branch in two worktrees (git refuses anyway).

## Spawning an agent

Start each agent **inside its worktree** so relative paths, `bin/*` and the
build directory are its own. Prompt template:

> You are working in the git worktree `~/code/omagames-omadoku` on branch
> `omadoku`. Read `CLAUDE.md`, `docs/PLAN.md`, then implement
> `docs/tasks/omadoku.md`. Follow `docs/WORKTREES.md` for git usage.

## What each agent must do

1. Confirm you are in the right place: `git rev-parse --abbrev-ref HEAD` must
   print your branch name and `git worktree list` should show your directory.
2. Work only in the paths your brief allows (`games/<yours>/**`, plus
   append-only additions to `common/qml/OmaGames/qmldir` and `common/common.qrc`
   when you add a generic control). Do not touch other games, `docs/PLAN.md`,
   `bin/`, or existing `common/` APIs — if you need a change there, add it as a
   note in your final report instead.
3. Commit early and often on your branch with clear messages
   (`omadoku: solver and generator with tests`). Never commit `build/`,
   `build-tests/` or generated files (already gitignored).
4. Before finishing: `bin/build <game>` and `bin/test <game>` must pass from a
   clean state (`rm -rf build build-tests` first), then `git status` must be clean.
5. Do not merge, rebase onto, or push `main`. Do not run `git worktree`
   commands. Do not force-push.
6. Final report: what's done, what's missing, test totals, and any request for
   changes outside your area.

## Reviewing (you)

```sh
cd ~/code/omagames                    # main checkout, on main
git log --oneline main..omadoku       # commits to review
git diff main...omadoku --stat        # files touched — check nothing outside games/omadoku + common qml
git diff main...omadoku -- common/    # eyeball shared changes specifically

# Try it without leaving main: the worktree already has the code built
../omagames-omadoku/bin/run omadoku
../omagames-omadoku/bin/test omadoku
```

Ask for fixes by re-prompting the same agent in its worktree; it keeps
committing on its branch.

Or push the branches and review as PRs: `git push -u origin omadoku`, then
`gh pr create --base main --head omadoku` — `/code-review` works on either.

## Merging back

Merge one branch at a time, re-verify, then the next:

```sh
cd ~/code/omagames
git merge --no-ff omadoku -m "Merge omadoku"
bin/build && bin/test

git merge --no-ff blackomack -m "Merge blackomack"
# If both added QML controls you may get a conflict in common/qml/OmaGames/qmldir
# and common/common.qrc: keep BOTH sides' lines, then `git add` and `git commit`.
bin/build && bin/test
```

If an agent's branch has drifted far behind `main` (e.g. you merged A and now
want B to pick up a common/ change), update it from inside its worktree:
`cd ../omagames-blackomack && git merge main`.

## Cleanup

```sh
git worktree remove ../omagames-omadoku      # deletes the directory
git branch -d omadoku                        # after it is merged
git worktree prune
```

Keep a worktree around if you expect more rounds with that agent.

## Gotchas

- `build/` and `build-tests/` are per-worktree and gitignored; a fresh worktree
  starts with no build, so the first `bin/build` takes a bit longer.
- Both games write settings to `~/.config/Omacom/<game>.conf` — that is shared
  across worktrees (same user), which is fine but means a test game state from
  one branch can show up when running another build of the same game.
- `git stash` is per-repository, not per-worktree: don't rely on it across agents.
