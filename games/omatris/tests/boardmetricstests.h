#pragma once

#include <QObject>

// The numbers an agent judges a stack by (src/boardmetrics.h).
class BoardMetricsTests : public QObject {
    Q_OBJECT

private slots:
    void anEmptyBoardMeasuresNothing();
    void heightsHolesAndBumpiness();
};
