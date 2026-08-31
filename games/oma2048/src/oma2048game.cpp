#include "oma2048game.h"

#include <QJsonDocument>
#include <QRandomGenerator>
#include <QRect>
#include <QSettings>

#include "windowgeometry.h"

namespace {

const auto kStateKey = QStringLiteral("state/v1");
const auto kClassicId = QStringLiteral("classic");

// The top ten runs, ranked on the score, the highest tile reached beside it.
OmaGames::ScoreTable scoreTable() {
    return OmaGames::ScoreTable(
        {QStringLiteral("score"), QStringLiteral("tile")}, 10,
        OmaGames::ScoreTable::sameOrder({kClassicId}, OmaGames::ScoreTable::HigherIsBetter));
}

}  // namespace

Oma2048Game::Oma2048Game(QObject *parent)
    : Oma2048Game(QRandomGenerator::global()->generate(), parent) {}

Oma2048Game::Oma2048Game(quint32 seed, QObject *parent)
    : QObject(parent), m_game(seed), m_tiles(this), m_scores(scoreTable()) {
    m_scores.load();
    if (!restoreSavedState())
        m_game.newGame();
    m_tiles.reset(m_game.board().tiles());
    m_shown = {score(), over(), won(), keepPlaying(), canUndo()};
    saveState();
}

int Oma2048Game::bestScore() const {
    return m_scores.best(kClassicId);
}

QVariantList Oma2048Game::scores() const {
    QVariantList list;
    for (const QVariant &entry : m_scores.toVariantList(kClassicId)) {
        QVariantMap row = entry.toMap();
        row.insert(QStringLiteral("category"), kClassicId);
        list.append(row);
    }
    return list;
}

void Oma2048Game::applyMove(Direction direction) {
    if (!m_game.move(direction))
        return;
    m_tiles.sync(m_game.board().tiles());
    if (m_game.over())
        finishRun();
    else
        saveState();
    announce();
}

void Oma2048Game::newGame() {
    m_game.newGame();
    if (m_lastRank != -1) {
        m_lastRank = -1;
        emit lastRankChanged();
    }
    m_tiles.reset(m_game.board().tiles());
    saveState();
    announce();
}

void Oma2048Game::undo() {
    if (!m_game.undo())
        return;
    m_tiles.sync(m_game.board().tiles());
    saveState();
    announce();
}

void Oma2048Game::keepGoing() {
    m_game.keepGoing();
    announce();
}

void Oma2048Game::announce() {
    const Shown was = m_shown;
    m_shown = {score(), over(), won(), keepPlaying(), canUndo()};
    if (m_shown.score != was.score)
        emit scoreChanged();
    if (m_shown.over != was.over)
        emit overChanged();
    if (m_shown.won != was.won)
        emit wonChanged();
    if (m_shown.keepPlaying != was.keepPlaying)
        emit keepPlayingChanged();
    if (m_shown.canUndo != was.canUndo)
        emit canUndoChanged();
}

// The one place a result is kept: the move that ended the run just landed.
void Oma2048Game::finishRun() {
    m_lastRank = m_scores.insert(
        kClassicId, {m_game.score(), QDate::currentDate(),
                     {{QStringLiteral("tile"), m_game.board().highestValue()}}});
    if (m_lastRank >= 0) {
        m_scores.save();
        emit scoresChanged();
        emit bestScoreChanged();
    }
    emit lastRankChanged();
    clearState();
}

bool Oma2048Game::restoreSavedState() {
    const QSettings settings;
    const QByteArray raw = settings.value(kStateKey).toString().toUtf8();
    if (raw.isEmpty())
        return false;
    const QJsonDocument doc = QJsonDocument::fromJson(raw);
    if (!doc.isObject() || !m_game.restore(doc.object()))
        return false;
    // A finished run was recorded and cleared when it ended, so a saved dead
    // board is hand-made: start over rather than re-record it.
    return !m_game.over();
}

void Oma2048Game::saveState() const {
    QSettings settings;
    settings.setValue(kStateKey, QString::fromUtf8(
        QJsonDocument(m_game.toJson()).toJson(QJsonDocument::Compact)));
}

void Oma2048Game::clearState() const {
    QSettings settings;
    settings.remove(kStateKey);
}

QVariantMap Oma2048Game::windowGeometry() const {
    return OmaGames::WindowGeometry::toVariantMap();
}

void Oma2048Game::saveWindowGeometry(int x, int y, int width, int height, bool maximized) {
    OmaGames::WindowGeometry::save(QRect(x, y, width, height), maximized);
}
