#pragma once

#include <QString>
#include <QStringList>

#include <optional>

namespace OmaGames {

// The replays an app was asked to show, in the order given: every
// `--replay <file>` on its command line. A trainer passes several to show
// one agent at each stage of its training, and the app steps between them.
class ReplayPlaylist {
public:
    ReplayPlaylist() = default;
    explicit ReplayPlaylist(QStringList paths) : m_paths(std::move(paths)) {}

    // Every value of --replay in `arguments`, in order: empty when there is
    // none, and nothing and `error` for a --replay with no file after it.
    static std::optional<QStringList> fromArguments(const QStringList &arguments, QString *error);

    const QStringList &paths() const { return m_paths; }
    bool isEmpty() const { return m_paths.isEmpty(); }
    int index() const { return m_index; }
    QString current() const { return isEmpty() ? QString() : m_paths.at(m_index); }
    bool hasNext() const { return m_index + 1 < m_paths.size(); }
    bool hasPrevious() const { return m_index > 0; }
    // False, and still on the same one, at either end.
    bool next();
    bool previous();
    // "3 of 10"; empty for one replay, which is not a list to step through.
    QString position() const;

private:
    QStringList m_paths;
    int m_index = 0;
};

}  // namespace OmaGames
