#include <QtTest/QtTest>
#include <QLineEdit>
#include <QSettings>
#include <QTemporaryDir>
#include "mainWindow.h"

namespace {
// QTEST_MAIN's generated main() constructs QApplication before any of our
// code runs, so the offscreen platform must be selected via a static
// initializer (which runs before main()) rather than from inside a test slot.
struct ForceOffscreenPlatform {
    ForceOffscreenPlatform() {
        if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
            qputenv("QT_QPA_PLATFORM", "offscreen");
        }
    }
} forceOffscreenPlatform;
}

class TestMzWatcher: public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();

    void isConvertibleMatch_dotDDirectoryMatches();
    void isConvertibleMatch_nonMatchingDirectoryDoesNotMatch();
    void isConvertibleMatch_plainFileStillMatches();

    void extensionSettingsMigration();
    void maxDayDiffRoundTrip();
    void folderEditsAreNotReadOnly();

    void gcsObjectPath_examples();
    void gcsUploadCommand_examples();
    void gcsUploadCommand_emptyWhenSettingsUnset();

private:
    QTemporaryDir *homeDir = nullptr;
    static void resetSettings();
};

void TestMzWatcher::initTestCase()
{
    // Force QSettings to a sandboxed ini file instead of the real per-user
    // "mzWatch"/"mzWatch Settings" store (native format on mac would touch
    // ~/Library/Preferences regardless of $HOME, ini format respects it).
    QSettings::setDefaultFormat(QSettings::IniFormat);
    homeDir = new QTemporaryDir();
    QVERIFY(homeDir->isValid());
    qputenv("HOME", homeDir->path().toUtf8());
}

void TestMzWatcher::cleanupTestCase()
{
    delete homeDir;
    homeDir = nullptr;
}

void TestMzWatcher::resetSettings()
{
    QSettings settings("mzWatch", "mzWatch Settings");
    settings.clear();
}

void TestMzWatcher::isConvertibleMatch_dotDDirectoryMatches()
{
    QVERIFY(MainWindow::isConvertibleMatch("sample1.d", ".d"));
    QVERIFY(MainWindow::isConvertibleMatch("SAMPLE1.D", ".d"));
}

void TestMzWatcher::isConvertibleMatch_nonMatchingDirectoryDoesNotMatch()
{
    QVERIFY(!MainWindow::isConvertibleMatch("someFolder", ".d"));
    QVERIFY(!MainWindow::isConvertibleMatch("sample1.wiff", ".d"));
}

void TestMzWatcher::isConvertibleMatch_plainFileStillMatches()
{
    QVERIFY(MainWindow::isConvertibleMatch("run42.wiff", ".wiff"));
    QVERIFY(MainWindow::isConvertibleMatch("RUN42.WIFF", ".wiff"));
}

void TestMzWatcher::extensionSettingsMigration()
{
    resetSettings();
    {
        QSettings settings("mzWatch", "mzWatch Settings");
        settings.setValue("extention", "mzXML");
    }

    MainWindow mw(0);

    QSettings after("mzWatch", "mzWatch Settings");
    QCOMPARE(after.value("extension").toString(), QString("mzXML"));
    QVERIFY(!after.contains("extention"));

    mw.close();
}

void TestMzWatcher::maxDayDiffRoundTrip()
{
    resetSettings();
    {
        QSettings settings("mzWatch", "mzWatch Settings");
        settings.setValue("maxDayDiff", 42);
    }

    MainWindow mw(0);
    mw.close(); // triggers closeEvent() -> writeSettings()

    QSettings after("mzWatch", "mzWatch Settings");
    QCOMPARE(after.value("maxDayDiff").toInt(), 42);
    QVERIFY(!after.contains("maxdayDiff"));
}

void TestMzWatcher::folderEditsAreNotReadOnly()
{
    resetSettings();
    MainWindow mw(0);

    QLineEdit *sourceEdit = mw.findChild<QLineEdit*>("sourceFolderEdit");
    QLineEdit *destEdit = mw.findChild<QLineEdit*>("destFolderEdit");
    QVERIFY(sourceEdit != nullptr);
    QVERIFY(destEdit != nullptr);
    QCOMPARE(sourceEdit->isReadOnly(), false);
    QCOMPARE(destEdit->isReadOnly(), false);

    mw.close();
}

void TestMzWatcher::gcsObjectPath_examples()
{
    QCOMPARE(MainWindow::buildGcsObjectPath("/dest/", "/dest/subdir/file.mzML"),
             QString("subdir/file.mzML"));
    QCOMPARE(MainWindow::buildGcsObjectPath("Y:/Metabolomics/Data/", "Y:/Metabolomics/Data/2024/run1.mzXML"),
             QString("2024/run1.mzXML"));
}

void TestMzWatcher::gcsUploadCommand_examples()
{
    QStringList cmd1 = MainWindow::buildGcsUploadCommand("/keys/service.json", "my-bucket", "/tmp/out/run1.mzML", "run1.mzML");
    QStringList expected1;
    expected1 << "gsutil" << "-o" << "Credentials:gs_service_key_file=/keys/service.json"
              << "cp" << "/tmp/out/run1.mzML" << "gs://my-bucket/run1.mzML";
    QCOMPARE(cmd1, expected1);

    QStringList cmd2 = MainWindow::buildGcsUploadCommand("C:/keys/key.json", "instrument-data", "C:/dest/2024/run2.mzXML", "2024/run2.mzXML");
    QStringList expected2;
    expected2 << "gsutil" << "-o" << "Credentials:gs_service_key_file=C:/keys/key.json"
              << "cp" << "C:/dest/2024/run2.mzXML" << "gs://instrument-data/2024/run2.mzXML";
    QCOMPARE(cmd2, expected2);
}

void TestMzWatcher::gcsUploadCommand_emptyWhenSettingsUnset()
{
    QVERIFY(MainWindow::buildGcsUploadCommand("", "my-bucket", "/tmp/out/run1.mzML", "run1.mzML").isEmpty());
    QVERIFY(MainWindow::buildGcsUploadCommand("/keys/service.json", "", "/tmp/out/run1.mzML", "run1.mzML").isEmpty());
    QVERIFY(MainWindow::buildGcsUploadCommand("", "", "/tmp/out/run1.mzML", "run1.mzML").isEmpty());
}

QTEST_MAIN(TestMzWatcher)
#include "testmzwatcher.moc"
