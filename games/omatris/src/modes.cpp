#include "modes.h"

#include <QCoreApplication>
#include <QVector>

namespace Modes {
namespace {

const auto kMarathonId = QStringLiteral("marathon");
const auto kSprintId = QStringLiteral("sprint");
const auto kZenId = QStringLiteral("zen");

// A namespace cannot carry Q_OBJECT's tr(), so the words below name their
// translation context themselves.
QString tr(const char *text) {
    return QCoreApplication::translate("Modes", text);
}

struct ModeInfo {
    Mode mode;
    QString label;
    QString description;
};

QVector<ModeInfo> modeInfos() {
    return {
        {Mode::Marathon, tr("Marathon"), tr("Endless. The levels keep coming and so does gravity.")},
        {Mode::Sprint, tr("Sprint"), tr("Forty lines at the first level's pace. The clock is the score.")},
        {Mode::Zen, tr("Zen"), tr("Endless and never faster. Stack for as long as you like.")},
    };
}

}  // namespace

QString id(Mode mode) {
    switch (mode) {
    case Mode::Sprint:
        return kSprintId;
    case Mode::Zen:
        return kZenId;
    case Mode::Marathon:
        break;
    }
    return kMarathonId;
}

bool fromId(const QString &wanted, Mode *mode) {
    for (int i = 0; i < kModeCount; ++i) {
        if (id(Mode(i)) == wanted) {
            *mode = Mode(i);
            return true;
        }
    }
    return false;
}

QString label(Mode mode) {
    for (const ModeInfo &info : modeInfos()) {
        if (info.mode == mode)
            return info.label;
    }
    return {};
}

QVariantList list() {
    QVariantList list;
    for (const ModeInfo &info : modeInfos()) {
        list.append(QVariantMap {
            {QStringLiteral("id"), id(info.mode)},
            {QStringLiteral("label"), info.label},
            {QStringLiteral("description"), info.description},
            {QStringLiteral("goal"), Rules::params(info.mode).lineGoal},
        });
    }
    return list;
}

OmaGames::ScoreTable scoreTable() {
    std::vector<OmaGames::ScoreTable::Category> categories;
    for (int i = 0; i < kModeCount; ++i) {
        const Mode mode = Mode(i);
        const bool byTime = Rules::params(mode).rankByTime;
        categories.push_back({id(mode),
                              byTime ? OmaGames::ScoreTable::LowerIsBetter
                                     : OmaGames::ScoreTable::HigherIsBetter,
                              byTime ? QStringLiteral("millis") : QString()});
    }
    return OmaGames::ScoreTable({QStringLiteral("score"), QStringLiteral("lines"),
                                 QStringLiteral("level"), QStringLiteral("millis")},
                                10, std::move(categories));
}

QVariantList scoreRows(const OmaGames::ScoreTable &scores) {
    QVariantList rows;
    for (const ModeInfo &info : modeInfos()) {
        const QString mode = id(info.mode);
        for (const QVariant &entry : scores.toVariantList(mode)) {
            QVariantMap row = entry.toMap();
            row.insert(QStringLiteral("mode"), mode);
            row.insert(QStringLiteral("label"), info.label);
            rows.append(row);
        }
    }
    return rows;
}

QVariantMap bests(const OmaGames::ScoreTable &scores) {
    QVariantMap map;
    for (int i = 0; i < kModeCount; ++i)
        map.insert(id(Mode(i)), scores.best(id(Mode(i))));
    return map;
}

}  // namespace Modes
