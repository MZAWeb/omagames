#include "blackjackgame.h"

#include <QSettings>

#include "gamestate.h"

using namespace BlackjackRules;

namespace {
constexpr int kDefaultStepMs = 500;
// The opening deal is a formality rather than a decision to follow, so the
// cards are pitched faster than the bot and dealer steps.
constexpr int kDealStepMs = 180;
constexpr int kLogLength = 8;
}

BlackjackGame::BlackjackGame(QObject *parent)
    : QObject(parent), m_rng(QRandomGenerator::global()->generate()), m_stepMs(kDefaultStepMs) {
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &BlackjackGame::step);
    if (!load())
        setBotCount(kDefaultBots);   // a fresh table is no fun on your own
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
    ms = qMax(0, ms);
    if (ms == m_stepMs)
        return;
    m_stepMs = ms;
    emit stepIntervalChanged();
}

int BlackjackGame::pace() const {
    if (m_stepMs == 0)
        return 0;
    return m_table.phase() == Table::Phase::Dealing ? qMin(kDealStepMs, m_stepMs) : m_stepMs;
}

void BlackjackGame::setBet(int amount) {
    if (m_table.phase() != Table::Phase::Betting)
        return;   // the stake is locked from the deal until the round is settled
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
    count = qBound(0, count, kMaxBots);
    while (m_table.botCount() > count)
        m_table.removeLastBot();
    while (m_table.botCount() < count) {
        const BotPersonality p = BotPersonality::roll(m_table.takenNames(), m_rng);
        m_table.addBot(p, kStartingBankroll, m_rng.generate());
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

// Most cards of the opening deal speak for themselves, so events without text
// (they still drive the animation) leave the log alone.
void BlackjackGame::record(const QVector<TableEvent> &events) {
    const int before = m_log.size();
    for (const TableEvent &e : events)
        if (!e.text.isEmpty())
            m_log.append(e.text);
    if (m_log.size() == before)
        return;
    while (m_log.size() > kLogLength)
        m_log.removeFirst();
    emit messageChanged();
}

// Automatic steps run on the timer so people can follow the bots and dealer;
// an interval of 0 runs them synchronously, which keeps tests deterministic.
void BlackjackGame::schedule() {
    if (m_table.waitingForHuman() || m_table.roundOver() || m_table.phase() == Table::Phase::Betting)
        return;
    const int ms = pace();
    if (ms == 0)
        step();
    else
        m_timer.start(ms);
}

void BlackjackGame::step() {
    while (true) {
        const auto events = m_table.advance();
        if (events.isEmpty())
            break;
        record(events);
        if (m_table.roundOver())
            finishRound();   // before the signal, so the stats settle with the round
        emit stateChanged();
        if (m_stepMs > 0) {
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

// False when there is nothing saved yet, so the caller can set a table up.
bool BlackjackGame::load() {
    QSettings settings;
    const QString text = settings.value(QString::fromLatin1(GameState::kKey)).toString();
    if (text.isEmpty())
        return false;
    const GameState state = GameState::fromString(text);
    m_table.setHumanBankroll(state.bankroll);
    for (const GameState::Bot &b : state.bots)
        m_table.addBot(b.personality, b.bankroll, m_rng.generate());
    m_handsPlayed = state.handsPlayed;
    m_netResult = state.netResult;
    return true;
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
