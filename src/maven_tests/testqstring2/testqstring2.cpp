#include <QtTest/QtTest>

class TestQString2: public QObject
{
    Q_OBJECT
private slots:
    void toUpper();
    void toUpper_data();
};


void TestQString2::toUpper()
{
    QFETCH(QString, string);
    QFETCH(QString, result);

    QCOMPARE(string.toUpper(), result);
}

void TestQString2::toUpper_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<QString>("result");

    QTest::newRow("all lower") << "hello" << "HELLO";
    QTest::newRow("mixed")     << "Hello" << "HELLO";
    QTest::newRow("all upper") << "HELLO" << "HELLO";
}

QTEST_MAIN(TestQString2)
#include "testqstring2.moc"
