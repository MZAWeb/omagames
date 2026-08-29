#!/usr/bin/env bash
# One-line installer for Omagames on Omarchy / Arch:
#
#   curl -fsSL https://raw.githubusercontent.com/MZAWeb/omagames/main/install.sh | bash -s omadoku
#   curl -fsSL https://raw.githubusercontent.com/MZAWeb/omagames/main/install.sh | bash -s omadoku blackomack
#
# Clones the repo, builds the requested game(s) into proper Arch packages with
# makepkg and installs them with pacman, so `pacman -R <game>` removes them
# cleanly. Set OMAGAMES_REF to build a tag or branch other than main.
#
# The whole script is wrapped in a function so a partially downloaded file
# can never execute half-way.
main() {
  set -euo pipefail

  local repo="https://github.com/MZAWeb/omagames.git"
  local ref="${OMAGAMES_REF:-main}"

  if [[ $# -eq 0 ]]; then
    echo "usage: install.sh <game> [game...]   (games: omadoku, blackomack)" >&2
    exit 1
  fi
  for tool in git makepkg pacman; do
    command -v "$tool" >/dev/null 2>&1 || { echo "$tool is required (this installer targets Omarchy / Arch Linux)." >&2; exit 1; }
  done

  # makepkg and sudo need a terminal, but stdin is the curl pipe. Attach the
  # tty when there is one; otherwise carry on and let makepkg complain itself.
  if [[ ! -t 0 ]] && { exec 3</dev/tty; } 2>/dev/null; then
    exec <&3 3<&-
  fi

  local work
  work="$(mktemp -d "${TMPDIR:-/tmp}/omagames.XXXXXX")"
  trap 'rm -rf "$work"' EXIT

  echo "==> Fetching omagames ($ref)"
  git clone --quiet --depth 1 --branch "$ref" "$repo" "$work/omagames"

  local game
  for game in "$@"; do
    if [[ ! -f "$work/omagames/games/$game/pkgbuild/PKGBUILD" ]]; then
      echo "unknown game '$game'; available: $(ls "$work/omagames/games" | tr '\n' ' ')" >&2
      exit 1
    fi
  done

  for game in "$@"; do
    echo "==> Building and installing $game"
    ( cd "$work/omagames/games/$game/pkgbuild" && makepkg -si --noconfirm --needed )
  done

  echo
  echo "Installed: $*. Find them in the launcher, or run them by name."
  echo "Remove with: sudo pacman -R $*"
}

main "$@"
