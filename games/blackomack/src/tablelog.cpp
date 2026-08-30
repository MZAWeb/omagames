#include "tablelog.h"

#include "table.h"

// Most cards of the opening deal speak for themselves, so events without text
// (they still drive the animation) leave the log alone.
bool TableLog::record(const QVector<TableEvent> &events) {
    const int before = m_lines.size();
    for (const TableEvent &e : events)
        if (!e.text.isEmpty())
            m_lines.append(e.text);
    if (m_lines.size() == before)
        return false;
    while (m_lines.size() > kLength)
        m_lines.removeFirst();
    return true;
}
