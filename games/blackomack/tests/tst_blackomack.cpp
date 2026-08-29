#include <QtTest>

class BlackOmackTest : public QObject {
    Q_OBJECT
private slots:
    void placeholder() { QVERIFY(true); }
};

QTEST_GUILESS_MAIN(BlackOmackTest)
#include "tst_blackomack.moc"
