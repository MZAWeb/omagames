#pragma once

#include <QObject>
#include <QRandomGenerator>
#include <QRect>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include "table.h"

// The only bridge between the engine and QML (`game` context property). Owns
// the Table, paces automatic steps with a timer, mirrors state into plain
// properties/variant lists and persists the session with QSettings.
class BlackjackGame : public QObject {
    Q_OBJECT
    Q_PROPERTY(int bankroll READ bankroll NOTIFY stateChanged)
    Q_PROPERTY(int bet READ bet NOTIFY betChanged)
    Q_PROPERTY(int minBet READ minBet CONSTANT)
    Q_PROPERTY(int maxBots READ maxBots CONSTANT)
    Q_PROPERTY(int maxBet READ maxBet NOTIFY stateChanged)
    Q_PROPERTY(QString phase READ phase NOTIFY stateChanged)
    Q_PROPERTY(int botCount READ botCount NOTIFY stateChanged)
    Q_PROPERTY(bool canDeal READ canDeal NOTIFY stateChanged)
    Q_PROPERTY(bool canHit READ canHit NOTIFY stateChanged)
    Q_PROPERTY(bool canStand READ canStand NOTIFY stateChanged)
    Q_PROPERTY(bool canDouble READ canDouble NOTIFY stateChanged)
    Q_PROPERTY(bool canSplit READ canSplit NOTIFY stateChanged)
    Q_PROPERTY(bool roundOver READ roundOver NOTIFY stateChanged)
    Q_PROPERTY(bool waitingForHuman READ waitingForHuman NOTIFY stateChanged)
    Q_PROPERTY(bool isBroke READ isBroke NOTIFY stateChanged)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)
    Q_PROPERTY(QStringList log READ log NOTIFY messageChanged)
    Q_PROPERTY(int handsPlayed READ handsPlayed NOTIFY stateChanged)
    Q_PROPERTY(int netResult READ netResult NOTIFY stateChanged)
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
    int maxBots() const { return BlackjackRules::kMaxBots; }
    int maxBet() const { return bankroll() - bankroll() % BlackjackRules::kBetStep; }
    QString phase() const;
    int botCount() const { return m_table.botCount(); }
    bool canDeal() const;
    bool canHit() const { return m_table.canAct(m_table.humanSeat(), Table::Action::Hit) && m_table.waitingForHuman(); }
    bool canStand() const { return canHit(); }
    bool canDouble() const { return m_table.waitingForHuman() && m_table.canAct(m_table.humanSeat(), Table::Action::Double); }
    bool canSplit() const { return m_table.waitingForHuman() && m_table.canAct(m_table.humanSeat(), Table::Action::Split); }
    bool roundOver() const { return m_table.roundOver(); }
    bool waitingForHuman() const { return m_table.waitingForHuman(); }
    bool isBroke() const { return m_table.human().broke() && m_table.phase() == Table::Phase::Betting; }
    QString message() const { return m_log.isEmpty() ? QString() : m_log.last(); }
    QStringList log() const { return m_log; }
    int handsPlayed() const { return m_handsPlayed; }
    int netResult() const { return m_netResult; }
    int stepInterval() const { return m_stepMs; }
    void setStepInterval(int ms);
    QVariantList seats() const;
    QVariantMap dealerHand() const;
    int humanSeat() const { return m_table.humanSeat(); }
    int shoeRemaining() const { return m_table.shoeRemaining(); }
    int shoePercent() const;
    QString rulesSummary() const;
    bool coachEnabled() const { return m_coachEnabled; }
    void setCoachEnabled(bool enabled);
    // The play to make ("Hit") and the spot it applies to ("16 against a 10"),
    // both empty unless the coach is on and the human has a live decision.
    QString coachAction() const { return coachLookup().action; }
    QString coachSituation() const { return coachLookup().situation; }

    // Test seam for deterministic bridge scenarios; deliberately not exposed to QML.
    void stackDeck(const QVector<Card> &cards) { m_table.stackDeck(cards); }

    Q_INVOKABLE void setBet(int amount);
    Q_INVOKABLE void adjustBet(int delta);
    Q_INVOKABLE void betMax();
    Q_INVOKABLE void dealRound();
    Q_INVOKABLE void hit() { humanAct(Table::Action::Hit); }
    Q_INVOKABLE void stand() { humanAct(Table::Action::Stand); }
    Q_INVOKABLE void doubleDown() { humanAct(Table::Action::Double); }
    Q_INVOKABLE void split() { humanAct(Table::Action::Split); }
    Q_INVOKABLE void nextRound();
    Q_INVOKABLE void skipPacing();
    Q_INVOKABLE void setBotCount(int count);
    Q_INVOKABLE void newGame();
    Q_INVOKABLE void toggleCoach() { setCoachEnabled(!m_coachEnabled); }

    Q_INVOKABLE QRect windowGeometry() const;
    Q_INVOKABLE void saveWindowGeometry(const QRect &geometry);

signals:
    void stateChanged();
    void betChanged();
    void messageChanged();
    void stepIntervalChanged();
    void coachEnabledChanged();
    void coachChanged();

private:
    struct Advice {
        QString action;
        QString situation;
    };
    Advice coachLookup() const;
    void humanAct(Table::Action action);
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
    QTimer m_timer;
    QStringList m_log;
    int m_stepMs;
    int m_bet = 50;
    int m_handsPlayed = 0;
    int m_netResult = 0;
    int m_roundStake = 0;
    bool m_coachEnabled = false;
};
