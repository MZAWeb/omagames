#include "sudokustore.h"

#include <QJsonDocument>
#include <QSettings>

namespace {

const auto kStateKey = QStringLiteral("state/v1");
// Still the key it was first stored under, so an existing preference survives
// the control's rename.
const auto kValidateKey = QStringLiteral("play/checkAsYouGo");
const auto kAutoNotesKey = QStringLiteral("play/autoNotes");
// A new key: the old one stored what *every* digit did, which is not what the
// selector means any more.
const auto kClickModeKey = QStringLiteral("play/clickMode");
// The clock rides along inside the saved board rather than beside it, so a
// resumed puzzle and its elapsed time can never come apart.
const auto kElapsedKey = QStringLiteral("elapsed");

}  // namespace

bool SudokuStore::validateAsYouGo(bool fallback) const {
    return QSettings().value(kValidateKey, fallback).toBool();
}

void SudokuStore::setValidateAsYouGo(bool validateAsYouGo) const {
    QSettings().setValue(kValidateKey, validateAsYouGo);
}

bool SudokuStore::autoNotes(bool fallback) const {
    return QSettings().value(kAutoNotesKey, fallback).toBool();
}

void SudokuStore::setAutoNotes(bool autoNotes) const {
    QSettings().setValue(kAutoNotesKey, autoNotes);
}

QString SudokuStore::clickMode() const {
    return QSettings().value(kClickModeKey).toString();
}

void SudokuStore::setClickMode(const QString &clickMode) const {
    QSettings().setValue(kClickModeKey, clickMode);
}

SudokuStore::SavedGame SudokuStore::savedGame() const {
    const QByteArray raw = QSettings().value(kStateKey).toString().toUtf8();
    const QJsonObject json = QJsonDocument::fromJson(raw).object();
    return {json, json.value(kElapsedKey).toInt(0)};
}

void SudokuStore::saveGame(const QJsonObject &board, int elapsedSeconds) const {
    QJsonObject json = board;
    json.insert(kElapsedKey, elapsedSeconds);
    QSettings().setValue(kStateKey,
                         QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Compact)));
}

void SudokuStore::clearSavedGame() const {
    QSettings().remove(kStateKey);
}
