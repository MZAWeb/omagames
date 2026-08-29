#include <QtTest>

class OmadokuTest : public QObject {
    Q_OBJECT
private slots:
    void placeholder() { QVERIFY(true); }
};

QTEST_GUILESS_MAIN(OmadokuTest)
#include "tst_omadoku.moc"
