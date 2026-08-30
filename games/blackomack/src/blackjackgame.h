#pragma once

#include <QObject>
#include <QRandomGenerator>
#include <QRect>
#include <QRectF>
#include <QSizeF>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include "coach.h"
#include "pacer.h"
#include "sessionstats.h"
#include "table.h"
#include "tablelog.h"

// The only bridge between the engine and QML (`game` context property). Owns
// the Table, paces automatic steps with a timer, mirrors state into plain
// properties/variant lists and persists the session with QSettings.
class BlackjackGame : public QObject {
    Q_OBJECT
    Q_PROPERTY(int bankroll READ bankroll NOTIFY stateChanged)
    Q_PROPERTY(int bet READ bet NOTIFY betChanged)
    Q_PROPERTY(int minBet READ minBet CONSTANT)
    // maxBots only ever moves with compactLayout, so they share a signal.
    Q_PROPERTY(int maxBots READ maxBots NOTIFY compactLayoutChanged)
    Q_PROPERTY(bool compactLayout READ compactLayout WRITE setCompactLayout NOTIFY compactLayoutChanged)
    Q_PROPERTY(int maxBet READ maxBet NOTIFY stateChanged)
    Q_PROPERTY(QVariantList betPresets READ betPresets NOTIFY stateChanged)
    Q_PROPERTY(QString phase READ phase NOTIFY stateChanged)
    Q_PROPERTY(int botCount READ botCount NOTIFY stateChanged)
    Q_PROPERTY(bool canDeal READ canDeal NOTIFY stateChanged)
    Q_PROPERTY(bool canHit READ canHit NOTIFY stateChanged)
    Q_PROPERTY(bool canStand READ canStand NOTIFY stateChanged)
    Q_PROPERTY(bool canDouble READ canDouble NOTIFY stateChanged)
    Q_PROPERTY(bool canSplit READ canSplit NOTIFY stateChanged)
    Q_PROPERTY(bool canInsure READ canInsure NOTIFY stateChanged)
    Q_PROPERTY(int insuranceCost READ insuranceCost NOTIFY stateChanged)
    Q_PROPERTY(bool roundOver READ roundOver NOTIFY stateChanged)
    Q_PROPERTY(bool waitingForHuman READ waitingForHuman NOTIFY stateChanged)
    Q_PROPERTY(bool isBroke READ isBroke NOTIFY stateChanged)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)
    Q_PROPERTY(QStringList log READ log NOTIFY messageChanged)
    Q_PROPERTY(int handsPlayed READ handsPlayed NOTIFY stateChanged)
    Q_PROPERTY(int netResult READ netResult NOTIFY stateChanged)
    Q_PROPERTY(int bestBankroll READ bestBankroll NOTIFY stateChanged)
    Q_PROPERTY(bool newBest READ newBest NOTIFY stateChanged)
    Q_PROPERTY(int stepInterval READ stepInterval WRITE setStepInterval NOTIFY stepIntervalChanged)
    Q_PROPERTY(QVariantList seats READ seats NOTIFY stateChanged)
    Q_PROPERTY(QVariantMap dealerHand READ dealerHand NOTIFY stateChanged)
    Q_PROPERTY(int humanSeat READ humanSeat NOTIFY stateChanged)
    Q_PROPERTY(int shoeRemaining READ shoeRemaining NOTIFY stateChanged)
    Q_PROPERTY(int shoePercent READ shoePercent NOTIFY stateChanged)
    Q_PROPERTY(QString rulesSummary READ rulesSummary CONSTANT)
    Q_PROPERTY(bool coachEnabled READ coachEnabled WRITE setCoachEnabled NOTIFY coachEnabledChanged)
    Q_PROPERTY(QString coachAction READ coachAction NOTIFY coachChanged)
    Q_PROPERTY(QString coachSituation READ coachSituation NOTIFY coachChanged)

public:
    explicit BlackjackGame(QObject *parent = nullptr);

    int bankroll() const { return m_table.human().bankroll; }
    int bet() const { return m_bet; }
    int minBet() const { return BlackjackRules::kMinBet; }
    int maxBet() const { return bankroll() - bankroll() % BlackjackRules::kBetStep; }
    // The stakes behind the 1/2/3 keys, smallest first. They follow the
    // bankroll, so a thin one offers fewer than three and the keys past the
    // end of the list do nothing.
    QVariantList betPresets() const;
    QString phase() const;
    int botCount() const { return m_table.botCount(); }
    bool canDeal() const;
    bool canHit() const { return m_table.canAct(m_table.humanSeat(), Table::Action::Hit) && m_table.waitingForHuman(); }
    bool canStand() const { return canHit(); }
    bool canDouble() const { return m_table.waitingForHuman() && m_table.canAct(m_table.humanSeat(), Table::Action::Double); }
    bool canSplit() const { return m_table.waitingForHuman() && m_table.canAct(m_table.humanSeat(), Table::Action::Split); }
    // The insurance offer is a decision like any other: while it stands the
    // table waits for the human and the dock swaps in its two commands.
    bool canInsure() const { return m_table.waitingForInsurance(); }
    int insuranceCost() const { return m_table.insuranceCost(m_table.humanSeat()); }
    bool roundOver() const { return m_table.roundOver(); }
    bool waitingForHuman() const { return m_table.waitingForHuman(); }
    bool isBroke() const { return m_table.human().broke() && m_table.phase() == Table::Phase::Betting; }
    QString message() const { return m_log.message(); }
    QStringList log() const { return m_log.lines(); }
    int handsPlayed() const { return m_stats.handsPlayed(); }
    int netResult() const { return m_stats.netResult(); }
    int bestBankroll() const { return m_stats.bestBankroll(); }
    bool newBest() const { return m_stats.newBest(); }
    int stepInterval() const { return m_pacer.interval(); }
    void setStepInterval(int ms);
    QVariantList seats() const;
    QVariantMap dealerHand() const;
    int humanSeat() const { return m_table.humanSeat(); }
    int shoeRemaining() const { return m_table.shoeRemaining(); }
    int shoePercent() const;
    QString rulesSummary() const;
    bool coachEnabled() const { return m_coachEnabled; }
    void setCoachEnabled(bool enabled);
    // The coach's two lines, both empty unless it is on and the human has a
    // live decision.
    QString coachAction() const { return advice().action; }
    QString coachSituation() const { return advice().situation; }

    // Test seam for deterministic bridge scenarios; deliberately not exposed to QML.
    void stackDeck(const QVector<Card> &cards) { m_table.stackDeck(cards); }

    Q_INVOKABLE void setBet(int amount);
    Q_INVOKABLE void setBetPreset(int index);
    Q_INVOKABLE void adjustBet(int delta);
    Q_INVOKABLE void betMax();
    Q_INVOKABLE void dealRound();
    Q_INVOKABLE void hit() { humanAct(Table::Action::Hit); }
    Q_INVOKABLE void stand() { humanAct(Table::Action::Stand); }
    Q_INVOKABLE void doubleDown() { humanAct(Table::Action::Double); }
    Q_INVOKABLE void split() { humanAct(Table::Action::Split); }
    Q_INVOKABLE void insurance();
    Q_INVOKABLE void declineInsurance();
    Q_INVOKABLE void nextRound();
    Q_INVOKABLE void skipPacing();
    Q_INVOKABLE void setBotCount(int count);
    Q_INVOKABLE void newGame();
    Q_INVOKABLE void toggleCoach() { setCoachEnabled(!m_coachEnabled); }

    // --- Table layout ---
    // A window too small for the oval's third seat seats two mates at most. A
    // larger table that was saved keeps its mates — they move to the roster
    // layout — but cannot grow again until the window does.
    static constexpr int kCompactMaxBots = 2;
    int maxBots() const { return m_compactLayout ? kCompactMaxBots : BlackjackRules::kMaxBots; }
    bool compactLayout() const { return m_compactLayout; }
    void setCompactLayout(bool compact);
    // Seat geometry lives in SeatLayout so it can be checked without QML.
    Q_INVOKABLE QRectF seatRect(int count, int index, const QSizeF &table, const QSizeF &seat) const;

    Q_INVOKABLE QRect windowGeometry() const;
    Q_INVOKABLE void saveWindowGeometry(const QRect &geometry);

signals:
    void stateChanged();
    void betChanged();
    void messageChanged();
    void stepIntervalChanged();
    void coachEnabledChanged();
    void coachChanged();
    void compactLayoutChanged();

private:
    Advice advice() const { return m_coachEnabled ? Coach::adviceFor(m_table) : Advice(); }
    void humanAct(Table::Action action);
    void seatBots(int count);
    int pace() const;
    void record(const QVector<TableEvent> &events);
    void schedule();
    void step();
    bool advanceOnce();
    void finishRound();
    bool load();
    void save() const;

    Table m_table;
    QRandomGenerator m_rng;
    OmaGames::Pacer m_pacer;
    TableLog m_log;
    SessionStats m_stats;
    int m_bet = 50;
    bool m_coachEnabled = false;
    bool m_compactLayout = false;
};
