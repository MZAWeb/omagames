#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

struct TableEvent;

// The table's running commentary: the last few lines the dock and the felt
// show. It only ever grows through the events a round produces, so the bridge
// hands it every batch and asks whether anything was worth saying.
class TableLog {
public:
    // How many lines are kept; the dock shows the last of them.
    static constexpr int kLength = 8;

    // True when the log changed, which is when the bridge tells QML.
    bool record(const QVector<TableEvent> &events);
    void clear() { m_lines.clear(); }

    const QStringList &lines() const { return m_lines; }
    QString message() const { return m_lines.isEmpty() ? QString() : m_lines.last(); }

private:
    QStringList m_lines;
};
