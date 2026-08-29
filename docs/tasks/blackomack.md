# Task B — Implement Black Omack (Blackjack)

You are implementing the game in `games/blackomack/`. Read `CLAUDE.md` and
`docs/PLAN.md` first. The skeleton (`blackomack.pro`, placeholder `main.cpp`,
`Main.qml`, `tests/`) already builds; replace the placeholders. Do not modify
`games/omadoku/`. You may add *generic* QML controls to `common/qml/OmaGames/`
(append to `qmldir` and `common.qrc`) — a generic `PlayingCard.qml` is a good
candidate since future card games will reuse it.

Done means: `bin/build blackomack` and `bin/test blackomack` pass,
`bin/run blackomack` is fully playable, and every rule below is tested.

## 1. Engine (QtCore only) — do this first

Files: `src/cards.h/.cpp` (Card, Shoe), `src/hand.h/.cpp`, `src/blackjackrules.h/.cpp`,
`src/botplayer.h/.cpp`, `src/table.h/.cpp` (round state machine).

```cpp
enum class Suit { Clubs, Diamonds, Hearts, Spades };
struct Card { int rank; /* 1=A .. 13=K */ Suit suit; int value() const; /* 10 for J/Q/K, 1 for A */ };

class Shoe {  // 6 decks, seeded QRandomGenerator, reshuffles when < 25% remain
public:
    explicit Shoe(int decks = 6, quint32 seed = 0);
    Card draw(); int remaining() const; bool needsShuffle() const; void shuffle();
};

struct Hand {
    QVector<Card> cards; int bet = 0; bool doubled = false; bool fromSplit = false; bool stood = false;
    int total() const;       // best total, aces as 11 when it doesn't bust
    bool isSoft() const; bool isBust() const;
    bool isBlackjack() const;  // 2 cards, 21, not from a split
    bool canSplit() const;     // 2 cards same value(), not already fromSplit
    bool canDouble() const;    // exactly 2 cards
};
```

Rules (v1, fixed): 6-deck shoe; dealer stands on all 17s (including soft);
blackjack pays 3:2; push returns the bet; double on any first two cards (one
card only); split once per hand (no re-split); split aces receive one card each
and cannot hit; no insurance, no surrender. Dealer peeks for blackjack when
showing an ace or ten-value card (round ends immediately, non-blackjack hands lose).

Betting: minimum 10, maximum = bankroll, increments of 10. Doubling/splitting
requires enough bankroll; otherwise the action is unavailable.

### Bots (`BotPlayer`)
```cpp
struct BotPersonality {
    QString name;          // from a fixed list of ~20 fun names, unique at the table
    double skill;          // 0.0–1.0: probability of following basic strategy on each decision
    double aggression;     // 0.0–1.0: bet sizing and risk appetite
    quint32 seed;          // for reproducible "mistakes"
};
```
- Decision: with probability `skill` play **basic strategy** (encode the
  standard hard/soft/pair tables for S17, 6 decks; put the tables in
  `basicstrategy.cpp` and test a few known cells); otherwise deviate: low-skill
  bots hit too much on 12–16 vs low dealer cards, stand on soft totals, never
  double/split, etc. Aggression tilts borderline decisions toward hit/double.
- Bet: `aggression` maps to a fraction of the bot's bankroll (e.g. 2%–25%),
  rounded to a 10 multiple, min 10; every bot starts with 1,000 Omabucks.
- Bots' bankrolls persist; a broke bot "leaves the table" and a fresh random
  bot takes the seat next hand (log this as a table message).
- Personalities are rolled at creation (random within bands: e.g. skill and
  aggression each drawn uniformly, then labelled "cautious / steady / wild",
  "sharp / average / reckless" for display) and **stored**, so the same bot
  behaves consistently across hands and launches.

### Table (`Table` — pure state machine, no timers)
Seats: 0–5 bots + the human (always last to act before the dealer, like a
real "third base"? — simpler: the human sits in the middle; order of play is seat order).
Phases: `Betting → Dealing → PlayerTurns (seat by seat, hand by hand) → DealerTurn → Payout → Betting`.
API is step-wise so the UI can pace it: `placeBets()`, `deal()`, `act(seat, hand, Action)`,
`advance()` (performs the next automatic step: a bot decision, a dealer card, a
payout) and returns an `Event` describing what happened for the UI to animate/log.
Human actions: `Hit, Stand, Double, Split`.

## 2. Game bridge — `src/blackjackgame.h/.cpp` (QObject, `game` in QML)

Properties: `bankroll`, `bet`, `minBet`(10), `maxBet`, `phase`, `botCount` (0–5),
`canHit/canStand/canDouble/canSplit`, `message` (last table event text),
`isBroke`, `handsPlayed`, `netResult` for session stats (optional).
Models: `seats` (list model: name, personality label, bankroll, hands with cards
and totals, status: waiting/playing/bust/blackjack/won/lost/push),
`dealerHand` (cards, hidden hole card until dealer turn, total).
Invokables: `setBet(amount)`, `adjustBet(±10)`, `betMax()`, `dealRound()`,
`hit()`, `stand()`, `doubleDown()`, `split()`, `nextRound()`, `setBotCount(n)`
(only between rounds; adding creates a new random bot, removing drops the last
seat), `newGame()` (reset bankroll to 1,000, re-roll all bots; confirm in UI).

Pacing: the bridge drives `Table::advance()` with a `QTimer` (~500 ms between
bot/dealer actions) so people can follow the play; make the interval a property
so tests can set it to 0 and the UI could offer a speed setting later.

Persistence (`QSettings`, JSON under `state/v1`): bankroll, bot count, each
bot's personality + bankroll, hands played. Saved after every payout and on
`setBotCount`/`newGame`. If the human is broke on launch → show the "You're
out of Omabucks" screen with only "New game". Window geometry too.

## 3. UI — QML, simple but polished, theme-only colors

Single table screen (`Main.qml` + `TableSeat.qml`, `PlayingCard.qml`,
`BetControls.qml`, `DealerArea.qml`):
- Dealer area at top; seats in a row/arc below; the human's seat is visually
  distinct (larger, accent border) at the bottom center.
- Cards: rounded rectangles, rank + suit glyph (♠♣♥♦), red suits in
  `theme.red`, face-down card uses `theme.accent`/`theme.selection` pattern.
  Simple deal animation (slide/fade in) is enough.
- Each seat shows name, personality label ("cautious · sharp"), bankroll,
  current bet, hand total, and result badge (WIN / LOSE / PUSH / BUST / BLACKJACK)
  colored with `theme.green/red/yellow`.
- Bottom bar: bankroll in Omabucks (`Ø 1,000` or `1,000 Ø` — pick a glyph and stick
  to it), bet stepper (−10 / +10 / Max / typed entry), Deal, and during the
  human's turn Hit / Stand / Double / Split. Buttons disabled when not allowed.
- Table controls: bot count (0–5) and New game, in a small menu or header.
- Message line for events ("Zed splits 8s", "Dealer shows 10").
- Resizable; layout must survive 5 bots at ~1000 px width and 0 bots.

Keyboard: `H` hit, `S` stand, `D` double, `P` split, `Enter`/`Space` deal / next round,
`↑/↓` or `+/-` bet ±10, `M` max bet, `Ctrl+N` new game, `Ctrl+Q` quit.

## 4. Tests (`tests/tst_blackomack.cpp`, add sources to `tests/tests.pro`)
- Hand totals: soft/hard aces, bust, blackjack vs 21-from-split, canSplit/canDouble.
- Shoe: 312 cards, deterministic per seed, reshuffle threshold.
- Rules: payouts 3:2 / 1:1 / push / lose; dealer stands on soft 17; dealer peek;
  split aces get one card; double takes exactly one card.
- Betting validation: min/max/increment; double/split blocked when short of funds.
- Basic strategy table spot checks (e.g. 16 vs 10 → hit, A7 vs 2 → stand, 88 → split, 11 vs 6 → double).
- Bot: skill = 1.0 always follows basic strategy; skill = 0 deviates deterministically per seed;
  bet stays within band and is a multiple of 10; same seed → same decisions.
- Table: full scripted round with a seeded shoe reaches Payout with the expected results.
- Persistence: save/restore round-trips bankroll + bots; broke bot is replaced.

## 5. Finish
- Add `pkgbuild/blackomack.svg` (flat icon: rounded square with two cards).
- Update `games/blackomack/README.md` (rules, keys, install).
- Report: what's done, what's not, how you verified (paste `bin/test blackomack` totals).
