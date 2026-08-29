# Black Omack

Blackjack for Omarchy. Sit at a table with up to six AI table mates, each with
a persistent personality, and try to grow your stack of Omabucks (Ø). A fresh
table seats four of them.

## Rules (v1)

- Six-deck shoe, reshuffled when fewer than a quarter of the cards remain.
- Dealer stands on all 17s (including soft 17) and peeks for blackjack when
  showing an ace or a ten-value card.
- Blackjack pays 3:2, wins pay 1:1, pushes return the bet.
- Double down on any first two cards (one card only), including after a split.
- Split once per hand; split aces get one card each and cannot hit.
- No insurance, no surrender.
- Bets from Ø 10 up to your whole bankroll, in steps of 10. Doubling and
  splitting require enough bankroll to cover the extra bet.

You start with Ø 1,000. The bankroll, your table mates and session stats are
saved between launches. Your session net appears with round and shoe context
in the action dock; every seat shows its own cumulative net beside its bankroll.
At the payout every hand shows what it paid. When you're broke you can only
start a new game.

## House Table

The table is arranged around a themed oval: the dealer stays at top-center,
AI table mates fill fixed upper-arc seats symmetrically in deal order, and your
larger accent-framed tray stays bottom-center. Bets sit beside their hands as
chips. The fixed action dock shows only the current phase's commands together
with the round, shoe depth and latest table event.

Heads-up and two-mate tables enlarge the dealer and player cards without
drawing empty seats. A full table keeps six stable arc positions while there
is room; at 160% text scale, or when those seats no longer fit at their minimum
size, the table reflows into a large dealer/player stage and a scrollable
deal-order roster. Identity, personality, bankroll, bet, cards, total, active
state, result and net payout remain visible in both layouts. Split and long
hands grow or wrap instead of covering seat details.

**Six seats.** The full arc uses three fixed 190×132 logical-pixel bot seats on
each side, filled from the dealer outward in deal order; an odd extra mate sits
dealer-left. The existing seat minimums and 430×181 human tray do not shrink.
Six-mate spatial mode needs roughly a 1280×760 window; below that it reflows to
the stage and scrollable roster, where all six mates remain available in deal
order.

## Decision Coach

The optional Decision Coach is off by default and remembers your choice. Press
`C` or use its badged control to toggle it. During your turn it shows one
text-only basic-strategy recommendation for the active hand, such as
`Hit — hard 16 vs 10`; it shows no odds, confidence or prediction and never
changes focus or plays for you. Once the hand is decided it changes to the
neutral resolved state and hides the recommendation. Strategy is calculated by
the C++ bridge with the same tested `BasicStrategy` engine used by table mates,
not by QML.

## Table mates

Each bot is rolled with a `skill` (how well it plays: rookie, regular, pro)
and an `aggression` (bet sizing and appetite for risk: timid, steady, bold).
Under its name it says how it bets and then how it plays — "timid pro",
"bold rookie", "steady regular". A bot that goes broke leaves and a fresh one
takes its seat. Change how many are at the table (0–6) between rounds with the
`−`/`+` buttons in the header; the choice is saved.

## Keyboard

Everything is reachable without a mouse; each button shows its key as a badge.

| Key | Action |
|---|---|
| `Enter` / `Space` | Deal / next round; `Space` also finishes automatic pacing without playing your hand |
| `H` `S` `D` `P` | Hit / Stand / Double / Split |
| `↑` `↓` or `+` `-` | Bet ±10 |
| `M` | Bet max |
| `B` | Type a bet (`Enter` deals, `Escape` reverts) — bet controls only exist while betting |
| `[` `]` | Fewer / more table mates (between rounds) |
| `C` | Toggle the Decision Coach (off by default) |
| `Ctrl+N` | New game (asks for confirmation) |
| `Y` / `Enter`, `N` / `Escape` | Confirm / cancel a dialog |
| `Ctrl+Q` | Quit |

## Build, test, run

```sh
bin/build blackomack
bin/test blackomack
bin/run blackomack
```

Settings live in `~/.config/Omacom/blackomack.conf`.

## Install (Arch)

```sh
curl -fsSL https://raw.githubusercontent.com/MZAWeb/omagames/main/install.sh | bash -s blackomack
```

or from a checkout, `bin/install blackomack`.
