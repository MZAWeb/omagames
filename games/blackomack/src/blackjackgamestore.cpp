// BlackjackGame: what it keeps between launches — the saved table and the
// window's place.
#include "blackjackgame.h"

#include <QSettings>

#include "gamestate.h"
#include "windowgeometry.h"

using namespace BlackjackRules;

QRect BlackjackGame::windowGeometry() const {
    return OmaGames::WindowGeometry::rect();
}

// Black Omack has never restored a maximized window, only the rect, so the
// flag it stores is always false.
void BlackjackGame::saveWindowGeometry(const QRect &geometry) {
    OmaGames::WindowGeometry::save(geometry, false);
}

// False when there is nothing saved yet, so the caller can set a table up.
bool BlackjackGame::load() {
    QSettings settings;
    const QString text = settings.value(QString::fromLatin1(GameState::kKey)).toString();
    if (text.isEmpty())
        return false;
    const GameState state = GameState::fromString(text);
    m_table.setHumanBankroll(state.bankroll);
    const int savedBots = qMin(state.bots.size(), kMaxBots);
    for (int i = 0; i < savedBots; ++i) {
        const GameState::Bot &b = state.bots[i];
        m_table.addBot(b.personality, b.bankroll, m_rng.generate());
    }
    m_stats.restore(state.handsPlayed, state.netResult, qMax(state.bestBankroll, bankroll()));
    if (state.bots.size() > kMaxBots)
        save();   // permanently trim state written by a newer or invalid configuration
    return true;
}

void BlackjackGame::save() const {
    GameState state;
    state.bankroll = bankroll();
    state.bestBankroll = m_stats.bestBankroll();
    for (const Seat &s : m_table.seats())
        if (!s.isHuman)
            state.bots.append({s.bot.personality(), s.bankroll});
    state.handsPlayed = m_stats.handsPlayed();
    state.netResult = m_stats.netResult();
    QSettings settings;
    settings.setValue(QString::fromLatin1(GameState::kKey), state.toString());
}
