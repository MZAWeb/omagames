#include "blackjackgame.h"

#include <QSettings>

#include "gamestate.h"
#include "seatlayout.h"
#include "windowgeometry.h"

using namespace BlackjackRules;

namespace {
constexpr int kDefaultStepMs = 500;
// The opening deal is a formality rather than a decision to follow, so the
// cards are pitched faster than the bot and dealer steps.
constexpr int kDealStepMs = 180;
constexpr const char *kCoachEnabledKey = "coach/enabled";
}

BlackjackGame::BlackjackGame(QObject *parent)
    : QObject(parent), m_rng(QRandomGenerator::global()->generate()),
      m_pacer(OmaGames::Pacer::SingleShot, [this]() { step(); }, this) {
    m_pacer.setInterval(kDefaultStepMs);
    connect(this, &BlackjackGame::stateChanged, this, &BlackjackGame::coachChanged);
    m_coachEnabled = QSettings().value(QString::fromLatin1(kCoachEnabledKey), false).toBool();
    if (!load())
        setBotCount(kDefaultBots);   // a fresh table is no fun on your own
    m_bet = clampBet(m_bet, bankroll());
}

void BlackjackGame::setCoachEnabled(bool enabled) {
    if (enabled == m_coachEnabled)
        return;
    m_coachEnabled = enabled;
    QSettings().setValue(QString::fromLatin1(kCoachEnabledKey), enabled);
    emit coachEnabledChanged();
    emit coachChanged();
}

QVariantList BlackjackGame::betPresets() const {
    QVariantList presets;
    for (int amount : BlackjackRules::betPresets(bankroll()))
        presets.append(amount);
    return presets;
}

QString BlackjackGame::phase() const {
    switch (m_table.phase()) {
    case Table::Phase::Betting: return QStringLiteral("betting");
    case Table::Phase::Dealing: return QStringLiteral("dealing");
    case Table::Phase::Insurance: return QStringLiteral("insurance");
    case Table::Phase::PlayerTurns: return QStringLiteral("playing");
    case Table::Phase::DealerTurn: return QStringLiteral("dealer");
    case Table::Phase::Payout: return QStringLiteral("payout");
    }
    return QString();
}

int BlackjackGame::shoePercent() const {
    return qRound(100.0 * shoeRemaining() / (kDecks * 52));
}

QString BlackjackGame::rulesSummary() const {
    return QStringLiteral("Blackjack pays %1 to %2 · Dealer stands on all %3")
        .arg(kBlackjackPayoutNumerator)
        .arg(kBlackjackPayoutDenominator)
        .arg(kDealerStandTotal);
}

bool BlackjackGame::canDeal() const {
    return m_table.phase() == Table::Phase::Betting && validBet(m_bet, bankroll());
}

void BlackjackGame::setStepInterval(int ms) {
    if (!m_pacer.setInterval(qMax(0, ms)))
        return;
    emit stepIntervalChanged();
}

int BlackjackGame::pace() const {
    const int ms = m_pacer.interval();
    if (ms == 0)
        return 0;
    return m_table.phase() == Table::Phase::Dealing ? qMin(kDealStepMs, ms) : ms;
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

void BlackjackGame::setBetPreset(int index) {
    const QVector<int> presets = BlackjackRules::betPresets(bankroll());
    if (index < 0 || index >= presets.size())
        return;
    setBet(presets[index]);
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
    m_stats.stake(m_bet);
    record(m_table.placeBets(m_bet));
    emit stateChanged();
    schedule();
}

void BlackjackGame::humanAct(Table::Action action) {
    const auto events = m_table.act(action);
    if (events.isEmpty())
        return;
    if (action == Table::Action::Double || action == Table::Action::Split)
        m_stats.addStake(m_bet);
    record(events);
    emit stateChanged();
    schedule();
}

// Half the original stake goes down before the dealer peeks; the round's
// stake has to grow with it or the session net would read the returned side
// bet as pure profit.
void BlackjackGame::insurance() {
    if (!canInsure())
        return;
    const int cost = insuranceCost();
    const auto events = m_table.takeInsurance();
    if (events.isEmpty())
        return;
    m_stats.addStake(cost);
    record(events);
    emit stateChanged();
    schedule();
}

void BlackjackGame::declineInsurance() {
    const auto events = m_table.declineInsurance();
    if (events.isEmpty())
        return;
    record(events);
    emit stateChanged();
    schedule();
}

void BlackjackGame::nextRound() {
    if (!roundOver())
        return;
    m_stats.clearCelebration();
    record(m_table.nextRound(m_rng));
    setBet(m_bet);   // re-clamp to what is left
    emit stateChanged();
}

void BlackjackGame::skipPacing() {
    if (m_table.waitingForHuman() || canInsure() || m_table.roundOver()
        || m_table.phase() == Table::Phase::Betting)
        return;
    m_pacer.stop();
    while (!m_table.waitingForHuman() && !canInsure() && !m_table.roundOver())
        if (!advanceOnce())
            break;
    emit stateChanged();
}

void BlackjackGame::setBotCount(int count) {
    if (m_table.phase() != Table::Phase::Betting)
        return;
    // Never past the cap, but a table that is already over it (saved larger
    // than a compact window allows) may still shrink a seat at a time.
    seatBots(qBound(0, count, qMax(maxBots(), m_table.botCount())));
}

void BlackjackGame::seatBots(int count) {
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
    m_stats.reset();   // the best bankroll deliberately survives: it is a high score
    m_log.clear();
    seatBots(bots);   // a new game reseats the table it replaces, cap or no cap
    setBet(50);
    record({{TableEvent::BotJoined, QStringLiteral("New game: %1 Omabucks").arg(kStartingBankroll)}});
    save();
    emit stateChanged();
}

void BlackjackGame::record(const QVector<TableEvent> &events) {
    if (m_log.record(events))
        emit messageChanged();
}

// Automatic steps run on the timer so people can follow the bots and dealer;
// an interval of 0 runs them synchronously, which keeps tests deterministic.
void BlackjackGame::schedule() {
    if (m_table.waitingForHuman() || canInsure() || m_table.roundOver()
        || m_table.phase() == Table::Phase::Betting)
        return;
    m_pacer.runIn(pace());
}

void BlackjackGame::step() {
    while (advanceOnce()) {
        emit stateChanged();
        if (m_pacer.interval() > 0) {
            schedule();
            return;
        }
    }
    emit stateChanged();
}

bool BlackjackGame::advanceOnce() {
    const auto events = m_table.advance();
    if (events.isEmpty())
        return false;
    record(events);
    if (m_table.roundOver())
        finishRound();   // before the signal, so the stats settle with the round
    return true;
}

void BlackjackGame::finishRound() {
    int returned = m_table.human().insuranceReturned;
    for (const Hand &h : m_table.human().hands)
        returned += h.returned;
    m_stats.settle(returned, bankroll());
    save();
}

// --- Table layout ---

void BlackjackGame::setCompactLayout(bool compact) {
    if (compact == m_compactLayout)
        return;
    m_compactLayout = compact;
    emit compactLayoutChanged();
}

QRectF BlackjackGame::seatRect(int count, int index, const QSizeF &table, const QSizeF &seat) const {
    return SeatLayout::rect(count, index, table, seat);
}

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
