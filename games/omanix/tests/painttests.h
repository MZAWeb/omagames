#pragma once

#include <QObject>

class PaintTests : public QObject {
    Q_OBJECT

private slots:
    void groundRunsArePixelIdenticalToCellByCell();
    void trailRunsArePixelIdenticalToCellByCell();
};
