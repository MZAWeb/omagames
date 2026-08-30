#include "sudokulevels.h"

#include <QCoreApplication>
#include <QStringList>
#include <QVariantMap>

namespace {

QString tr(const char *text) {
    return QCoreApplication::translate("SudokuLevels", text);
}

// The rungs a level adds on top of the level below: everything between the
// two ceilings, which is exactly what its puzzles can demand and the easier
// level's never do.
QStringList techniquesIntroducedBy(Difficulty difficulty) {
    const int from = difficulty == Difficulty::Easy
        ? 0 : int(SudokuGenerator::ceiling(Difficulty(int(difficulty) - 1))) + 1;
    QStringList names;
    for (int t = from; t <= int(SudokuGenerator::ceiling(difficulty)); ++t)
        names << SudokuLevels::techniqueName(SudokuGrader::Technique(t));
    return names;
}

}  // namespace

QString SudokuLevels::id(Difficulty difficulty) {
    switch (difficulty) {
    case Difficulty::Easy:
        return QStringLiteral("easy");
    case Difficulty::Medium:
        return QStringLiteral("medium");
    case Difficulty::Hard:
        return QStringLiteral("hard");
    case Difficulty::ExtraHard:
        break;
    }
    return QStringLiteral("extrahard");
}

bool SudokuLevels::fromId(const QString &id, Difficulty *difficulty) {
    for (int i = 0; i < kDifficultyCount; ++i) {
        if (SudokuLevels::id(Difficulty(i)) == id) {
            *difficulty = Difficulty(i);
            return true;
        }
    }
    return false;
}

QString SudokuLevels::label(const QString &id) {
    for (const QVariant &entry : all()) {
        const QVariantMap level = entry.toMap();
        if (level.value(QStringLiteral("id")).toString() == id)
            return level.value(QStringLiteral("label")).toString();
    }
    return {};
}

QVariantList SudokuLevels::all() {
    struct Level {
        Difficulty difficulty;
        QString label;
        QString description;
    };
    const QVector<Level> levels {
        {Difficulty::Easy, tr("Easy"), tr("Plenty of clues; a box scan or a lone digit always moves you on.")},
        {Difficulty::Medium, tr("Medium"), tr("Fewer clues; scan rows and columns too, or spot a naked pair.")},
        {Difficulty::Hard, tr("Hard"), tr("Needs pencil marks: a hidden pair, naked triple or hidden triple.")},
        {Difficulty::ExtraHard, tr("Extra hard"), tr("Needs an X-wing, swordfish or XY-wing somewhere.")},
    };
    QVariantList list;
    for (const Level &level : levels) {
        list.append(QVariantMap {
            {QStringLiteral("id"), id(level.difficulty)},
            {QStringLiteral("label"), level.label},
            {QStringLiteral("techniques"), techniquesIntroducedBy(level.difficulty)},
            {QStringLiteral("description"), level.description},
        });
    }
    return list;
}

QString SudokuLevels::techniqueName(SudokuGrader::Technique technique) {
    using SudokuGrader::Technique;
    switch (technique) {
    case Technique::LastDigit:
        return tr("Last digit");
    case Technique::HiddenSingleBox:
        return tr("Hidden single (box)");
    case Technique::NakedSingle:
        return tr("Naked single");
    case Technique::HiddenSingleLine:
        return tr("Hidden single (line)");
    case Technique::NakedPair:
        return tr("Naked pair");
    case Technique::HiddenPair:
        return tr("Hidden pair");
    case Technique::NakedTriple:
        return tr("Naked triple");
    case Technique::HiddenTriple:
        return tr("Hidden triple");
    case Technique::XWing:
        return tr("X-wing");
    case Technique::Swordfish:
        return tr("Swordfish");
    case Technique::XYWing:
        break;
    }
    return tr("XY-wing");
}

OmaGames::ScoreTable SudokuLevels::timesTable() {
    QStringList ids;
    for (int i = 0; i < kDifficultyCount; ++i)
        ids << id(Difficulty(i));
    OmaGames::ScoreTable table({QStringLiteral("seconds")}, 5,
                               OmaGames::ScoreTable::sameOrder(ids, OmaGames::ScoreTable::LowerIsBetter));
    table.setMinimumValue(1);
    return table;
}
