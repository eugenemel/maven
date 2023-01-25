#include "testsimpleparsimony.h"
#include "mzUtils.h"

using namespace std;

void TestSimpleParsimony::simpleParsimonyReduce()
{
    QFETCH(vector<vector<int>>, input);
    QFETCH(vector<vector<int>>, output);

    auto x = mzUtils::simpleParsimonyReducer(input);
    cout << "mzUtils::simpleParsimonyReducer(): ";

    for (unsigned int i = 0; i < x.size(); i++) {
        auto x1 = x[i];
        if (i>0) cout << ", ";
        cout << "{";
        for (unsigned int j = 0; j < x1.size(); j++) {
            if (j>0) {
                cout << ", ";
            }
            cout << x1[j];
        }
        cout << "}";
    }
    cout << endl;

    QCOMPARE(input.size()-2, output.size());
}

void TestSimpleParsimony::simpleParsimonyReduce_data()
{

    QTest::addColumn<vector<vector<int>>>("input");
    QTest::addColumn<vector<vector<int>>>("output");

    QTest::newRow("Merge Perfect Overlaps")
           << vector<vector<int>>{{1,2}, {1,2,3}, {4, 5}, {4, 5, 6}, {7, 8}, {7, 9}, {7, 10}}
           << vector<vector<int>>{{7, 8}, {7, 9}, {7, 10}, {1,2,3}, {4, 5, 6}};
}

QTEST_MAIN(TestSimpleParsimony)
#include "testsimpleparsimony.moc"
