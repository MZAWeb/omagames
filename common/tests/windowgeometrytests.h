#pragma once

#include <QObject>
#include <QString>

// Saving and restoring the window's place (common/src/windowgeometry.h).
class WindowGeometryTests : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void init();

    void nothingSavedYetIsInvalidRatherThanZeroed();
    void aSavedRectComesBack();
    void negativePositionsSurvive();
    void maximizedIsRememberedSeparately();
};
