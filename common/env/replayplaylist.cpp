#include "replayplaylist.h"

namespace OmaGames {

std::optional<QStringList> ReplayPlaylist::fromArguments(const QStringList &arguments, QString *error) {
    const QString option = QStringLiteral("--replay");
    QStringList paths;
    for (int i = 0; i < arguments.size(); ++i) {
        if (arguments.at(i) != option)
            continue;
        if (i + 1 >= arguments.size() || arguments.at(i + 1).startsWith(QLatin1String("--"))) {
            *error = QStringLiteral("--replay needs a file");
            return std::nullopt;
        }
        paths.append(arguments.at(++i));
    }
    return paths;
}

bool ReplayPlaylist::next() {
    if (!hasNext())
        return false;
    ++m_index;
    return true;
}

bool ReplayPlaylist::previous() {
    if (!hasPrevious())
        return false;
    --m_index;
    return true;
}

QString ReplayPlaylist::position() const {
    if (m_paths.size() < 2)
        return {};
    return QStringLiteral("%1 of %2").arg(m_index + 1).arg(m_paths.size());
}

}  // namespace OmaGames
