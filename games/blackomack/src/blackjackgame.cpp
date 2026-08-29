#include "blackjackgame.h"

#include <QSettings>

#include "gamestate.h"

using namespace BlackjackRules;

namespace {
constexpr int kDefaultStepMs = 500;
constexpr int kLogLength = 8;
}

BlackjackGame::BlackjackGame(QObject *parent)
    : QObject(parent), m_rng(QRandomGenerator::global()->generate()) {
    m_timer.setSingleShot(true);
    m_timer.setInterval(kDefaultStepMs);
    connect(&m_timer, &QTimer::timeout, this, &BlackjackGame::step);
    load();
    m_bet = clampBet(m_bet, bankroll());
}

QString BlackjackGame::phase() const {
    switch (m_table.phase()) {
    case Table::Phase::Betting: return QStringLiteral("betting");
    case Table::Phase::Dealing: return QStringLiteral("dealing");
    case Table::Phase::PlayerTurns: return QStringLiteral("playing");
    case Table::Phase::DealerTurn: return QStringLiteral("dealer");
    case Table::Phase::Payout: return QStringLiteral("payout");
    }
    return QString();
}

bool BlackjackGame::canDeal() const {
    return m_table.phase() == Table::Phase::Betting && validBet(m_bet, bankroll());
}

void BlackjackGame::setStepInterval(int ms) {
    if (ms == m_timer.interval())
        return;
    m_timer.setInterval(qMax(0, ms));
    emit stepIntervalChanged();
}

void BlackjackGame::setBet(int amount) {
    const int clamped = clampBet(amount, bankroll());
    if (clamped == m_bet)
        return;
    m_bet = clamped;
    emit betChanged();
    emit stateChanged();   // canDeal depends on the bet
}

void BlackjackGame::adjustBet(int delta) {
    setBet(m_bet + delta);
}

void BlackjackGame::betMax() {
    setBet(bankroll());
}

void BlackjackGame::dealRound() {
    if (!canDeal())
        return;
    m_roundStake = m_bet;
    record(m_table.placeBets(m_bet));
    record(m_table.deal());
    for (int s = 0; s < m_table.seats().size(); ++s)
        if (!m_table.seats()[s].hands.isEmpty())
            emit cardDealt(s, 0);
    emit stateChanged();
    schedule();
}

void BlackjackGame::humanAct(Table::Action action) {
    const auto events = m_table.act(action);
    if (events.isEmpty())
        return;
    if (action == Table::Action::Double || action == Table::Action::Split)
        m_roundStake += m_bet;
    record(events);
    emit cardDealt(m_table.humanSeat(), events.first().hand);
    emit stateChanged();
    schedule();
}

void BlackjackGame::nextRound() {
    if (!roundOver())
        return;
    record(m_table.nextRound(m_rng));
    setBet(m_bet);   // re-clamp to what is left
    emit stateChanged();
}

void BlackjackGame::setBotCount(int count) {
    if (m_table.phase() != Table::Phase::Betting)
        return;
    count = qBound(0, count, 5);
    while (m_table.botCount() > count)
        m_table.removeLastBot();
    while (m_table.botCount() < count) {
        const BotPersonality p = BotPersonality::roll(m_table.takenNames(), m_rng);
        m_table.addBot(p);
        record({{TableEvent::BotJoined, QStringLiteral("%1 sits down (%2)").arg(p.name, p.label())}});
    }
    save();
    emit stateChanged();
}

void BlackjackGame::newGame() {
    if (m_table.phase() != Table::Phase::Betting)
        return;
    const int bots = m_table.botCount();
    m_table = Table();
    m_handsPlayed = 0;
    m_netResult = 0;
    m_log.clear();
    setBotCount(bots);
    setBet(50);
    record({{TableEvent::BotJoined, QStringLiteral("New game: %1 Omabucks").arg(kStartingBankroll)}});
    save();
    emit stateChanged();
}

void BlackjackGame::record(const QVector<TableEvent> &events) {
    if (events.isEmpty())
        return;
    for (const TableEvent &e : events)
        m_log.append(e.text);
    while (m_log.size() > kLogLength)
        m_log.removeFirst();
    emit messageChanged();
}

// Automatic steps run on the timer so people can follow the bots and dealer;
// an interval of 0 runs them synchronously, which keeps tests deterministic.
void BlackjackGame::schedule() {
    if (m_table.waitingForHuman() || m_table.roundOver() || m_table.phase() == Table::Phase::Betting)
        return;
    if (m_timer.interval() == 0)
        step();
    else
        m_timer.start();
}

void BlackjackGame::step() {
    while (true) {
        const auto events = m_table.advance();
        if (events.isEmpty())
            break;
        record(events);
        const TableEvent &first = events.first();
        if (first.type == TableEvent::PlayerAction || first.type == TableEvent::DealerCard
            || first.type == TableEvent::DealerReveal)
            emit cardDealt(first.seat, first.hand);
        emit stateChanged();
        if (m_table.roundOver())
            finishRound();
        if (m_timer.interval() > 0) {
            schedule();
            return;
        }
    }
    emit stateChanged();
}

void BlackjackGame::finishRound() {
    int returned = 0;
    for (const Hand &h : m_table.human().hands)
        returned += h.returned;
    ++m_handsPlayed;
    m_netResult += returned - m_roundStake;
    save();
}

QRect BlackjackGame::windowGeometry() const {
    QSettings settings;
    return settings.value(QStringLiteral("window/geometry")).toRect();
}

void BlackjackGame::saveWindowGeometry(const QRect &geometry) {
    QSettings settings;
    settings.setValue(QStringLiteral("window/geometry"), geometry);
}

void BlackjackGame::load() {
    QSettings settings;
    const QString text = settings.value(QString::fromLatin1(GameState::kKey)).toString();
    if (text.isEmpty())
        return;
    const GameState state = GameState::fromString(text);
    m_table.setHumanBankroll(state.bankroll);
    for (const GameState::Bot &b : state.bots)
        m_table.addBot(b.personality, b.bankroll);
    m_handsPlayed = state.handsPlayed;
    m_netResult = state.netResult;
}

void BlackjackGame::save() const {
    GameState state;
    state.bankroll = bankroll();
    for (const Seat &s : m_table.seats())
        if (!s.isHuman)
            state.bots.append({s.bot.personality(), s.bankroll});
    state.handsPlayed = m_handsPlayed;
    state.netResult = m_netResult;
    QSettings settings;
    settings.setValue(QString::fromLatin1(GameState::kKey), state.toString());
}
