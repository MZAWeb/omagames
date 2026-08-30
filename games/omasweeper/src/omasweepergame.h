#pragma once

#include <QObject>
#include <QPoint>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>
#include <optional>

#include "board.h"
#include "generator.h"
#include "pacer.h"
#include "presets.h"
#include "scoretable.h"

// The cascade the last reveal opened, in breadth-first order, with each
// cell's Chebyshev distance from the cell that was acted on so the view can
// ripple outwards instead of flipping a whole region at once.
struct Ripple {
    QVector<int> cells;
    QVector<int> distances;
    int maxDistance = 0;
};

// The only bridge between the engine and QML: state as properties, actions as
// invokables, persistence and the clock. The board is generated on the first
// reveal, so the opening is always safe and always logically solvable.
class OmasweeperGame : public QObject {
    Q_OBJECT
    // "start" | "playing" | "won" | "lost": which screen QML should show.
    Q_PROPERTY(QString phase READ phase NOTIFY phaseChanged)
    // The engine's own status: "ready" | "playing" | "won" | "lost".
    Q_PROPERTY(QString status READ status NOTIFY phaseChanged)
    Q_PROPERTY(QString preset READ preset NOTIFY presetChanged)
    Q_PROPERTY(QString presetLabel READ presetLabel NOTIFY presetChanged)
    Q_PROPERTY(QVariantList presets READ presets CONSTANT)
    Q_PROPERTY(int width READ width NOTIFY presetChanged)
    Q_PROPERTY(int height READ height NOTIFY presetChanged)
    Q_PROPERTY(int mines READ mines NOTIFY presetChanged)
    Q_PROPERTY(int remainingMines READ remainingMines NOTIFY countersChanged)
    Q_PROPERTY(int flags READ flags NOTIFY countersChanged)
    Q_PROPERTY(int elapsedSeconds READ elapsedSeconds NOTIFY elapsedSecondsChanged)
    Q_PROPERTY(int cursorX READ cursorX NOTIFY cursorChanged)
    Q_PROPERTY(int cursorY READ cursorY NOTIFY cursorChanged)
    Q_PROPERTY(bool noGuess READ noGuess NOTIFY noGuessChanged)
    Q_PROPERTY(QVariantList bestTimes READ bestTimes NOTIFY bestTimesChanged)
    Q_PROPERTY(QVariantMap bests READ bests NOTIFY bestTimesChanged)
    Q_PROPERTY(int newBestRank READ newBestRank NOTIFY phaseChanged)
    Q_PROPERTY(int stepInterval READ stepInterval WRITE setStepInterval NOTIFY stepIntervalChanged)

public:
    // One tick of the clock per timer shot.
    static constexpr int kDefaultStepIntervalMs = 1000;

    explicit OmasweeperGame(QObject *parent = nullptr);

    QString phase() const;
    QString status() const;
    QString preset() const;
    QString presetLabel() const;
    // {id, label, width, height, mines} for the start screen, in play order.
    static QVariantList presets();
    int width() const { return spec().width; }
    int height() const { return spec().height; }
    int mines() const { return spec().mines; }
    // Mines minus flags; negative when the player over-flags.
    int remainingMines() const { return m_board ? m_board->remainingMines() : mines(); }
    int flags() const { return m_board ? m_board->flagCount() : 0; }
    int elapsedSeconds() const { return m_elapsedSeconds; }
    int cursorX() const { return m_cursor.x(); }
    int cursorY() const { return m_cursor.y(); }
    // False once the generator ran out of tries and handed back a board it
    // could not prove: that one may require a guess.
    bool noGuess() const { return m_noGuess; }
    // {preset, label, seconds, date} for every table, fastest first.
    QVariantList bestTimes() const;
    // preset id -> fastest time in seconds (0 when never won).
    QVariantMap bests() const;
    // Where the win just landed in its table (0 = fastest ever), or -1.
    int newBestRank() const { return m_newBestRank; }
    // Milliseconds per clock tick; 0 stops the timer so tests drive step().
    int stepInterval() const { return m_pacer.interval(); }
    void setStepInterval(int interval);

    // Read-only view for the renderer; null on the start screen.
    const Board *board() const { return m_board ? &*m_board : nullptr; }
    // The mine that ended the game, or -1.
    int explodedCell() const { return m_exploded; }
    const Ripple &lastReveal() const { return m_ripple; }

    // Seeded entry point, so tests get the same board every run.
    void startGame(Preset preset, quint32 seed);
    // Test seam: how many layouts the generator may try before giving up.
    void setMaxAttemptsForTests(int attempts) { m_maxAttempts = attempts; }

    Q_INVOKABLE void newGame(const QString &preset);
    Q_INVOKABLE void moveCursor(int dx, int dy);
    // Every coordinate action moves the cursor there first, so mouse and
    // keyboard never disagree about where "here" is.
    Q_INVOKABLE void reveal(int x, int y);
    Q_INVOKABLE void revealAtCursor();
    Q_INVOKABLE void toggleFlag(int x, int y);
    Q_INVOKABLE void flagAtCursor();
    Q_INVOKABLE void chord(int x, int y);
    Q_INVOKABLE void chordAtCursor();
    // Same preset, a fresh board.
    Q_INVOKABLE void restart();
    Q_INVOKABLE void backToStart();
    Q_INVOKABLE void step();

    Q_INVOKABLE QVariantMap windowGeometry() const;
    Q_INVOKABLE void saveWindowGeometry(int x, int y, int width, int height, bool maximized);

signals:
    void phaseChanged();
    void presetChanged();
    void countersChanged();
    void elapsedSecondsChanged();
    void cursorChanged();
    void noGuessChanged();
    void bestTimesChanged();
    void stepIntervalChanged();
    // Anything the renderer paints has changed.
    void fieldChanged();
    // A new cascade is available in lastReveal(), worth animating.
    void revealRippled();

private:
    const PresetSpec &spec() const { return Presets::spec(m_preset); }
    void generate(QPoint firstClick);
    void apply(const MoveResult &result, QPoint origin);
    void buildRipple(const std::vector<int> &cells, QPoint origin);
    void setCursor(QPoint cursor);
    void finishGame(Status status);
    void syncTimer();
    void loadSettings();

    std::optional<Board> m_board;
    OmaGames::ScoreTable m_times;
    OmaGames::Pacer m_pacer;
    Preset m_preset = Preset::Beginner;
    quint32 m_seed = 0;
    int m_maxAttempts = Generator::kMaxAttempts;
    int m_elapsedSeconds = 0;
    int m_newBestRank = -1;
    int m_exploded = -1;
    bool m_noGuess = true;
    Status m_status = Status::Ready;
    QPoint m_cursor;
    Ripple m_ripple;
};
