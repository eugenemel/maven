#include <QtTest/QtTest>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QSettings>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QSysInfo>
#include <QTextStream>
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
    void init();
    void cleanup();

    void isConvertibleMatch_dotDDirectoryMatches();
    void isConvertibleMatch_nonMatchingDirectoryDoesNotMatch();
    void isConvertibleMatch_plainFileStillMatches();

    void extensionSettingsMigration();
    void maxDayDiffRoundTrip();
    void folderEditsAreNotReadOnly();

    void gcsObjectPath_examples();
    void gcsUploadCommand_examples();
    void gcsUploadCommand_emptyWhenSettingsUnset();

    void totalDirectorySize_sumsNestedFiles();
    void totalDirectorySize_plainFileReturnsItsOwnSize();
    void thresholdInBytes_examples();
    void parseEmailRecipients_splitsAndTrims();
    void parseEmailRecipients_emptyStringYieldsEmptyList();
    void buildWarningEmailSubject_includesFileName();
    void buildWarningEmailBody_includesAllFields();
    void parseMailerConfigFile_parsesKeyValueLines();
    void parseMailerConfigFile_ignoresBlankLinesAndComments();
    void parseMailerConfigFile_missingFileYieldsEmptyHash();
    void automaticWarningsSettingsDefaults();

private:
    QTemporaryDir *homeDir = nullptr;
    static void resetSettings();
};

void TestMzWatcher::init()
{
    // QSettings("mzWatch", "mzWatch Settings") always uses NativeFormat on
    // mac (the 2-arg organization/application constructor ignores
    // setDefaultFormat()), which is backed by CFPreferences. Redirecting
    // $HOME is a best-effort attempt to keep this off a real user's saved
    // preferences; note that on some macOS/CFPreferences combinations this
    // redirection is not fully honored, so resetSettings()'s explicit
    // clear() at the start of every settings-touching test is the load-
    // bearing isolation, not this alone. A fresh sandbox per test (via
    // QtTest's init()/cleanup(), called around every test slot) is still
    // the right default even so.
    homeDir = new QTemporaryDir();
    QVERIFY(homeDir->isValid());
    qputenv("HOME", homeDir->path().toUtf8());
}

void TestMzWatcher::cleanup()
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

void TestMzWatcher::totalDirectorySize_sumsNestedFiles()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString root = dir.path();

    {
        QFile f1(root + "/a.txt");
        QVERIFY(f1.open(QIODevice::WriteOnly));
        f1.write(QByteArray(100, 'x'));
        f1.close();
    }

    QVERIFY(QDir().mkpath(root + "/sub"));
    {
        QFile f2(root + "/sub/b.txt");
        QVERIFY(f2.open(QIODevice::WriteOnly));
        f2.write(QByteArray(250, 'y'));
        f2.close();
    }

    QCOMPARE(MainWindow::totalDirectorySize(root), (qint64)350);
}

void TestMzWatcher::totalDirectorySize_plainFileReturnsItsOwnSize()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString filePath = dir.path() + "/run1.raw";

    QFile f(filePath);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(QByteArray(42, 'z'));
    f.close();

    QCOMPARE(MainWindow::totalDirectorySize(filePath), (qint64)42);
}

void TestMzWatcher::thresholdInBytes_examples()
{
    QCOMPARE(MainWindow::thresholdInBytes(70, "kB"), (qint64)70000);
    QCOMPARE(MainWindow::thresholdInBytes(1.5, "MB"), (qint64)1500000);
    QCOMPARE(MainWindow::thresholdInBytes(2, "GB"), (qint64)2000000000);
    QCOMPARE(MainWindow::thresholdInBytes(5, "kb"), (qint64)5000); // case-insensitive
}

void TestMzWatcher::parseEmailRecipients_splitsAndTrims()
{
    // Recipients are completely unrestricted -- any address, any domain.
    QStringList result = MainWindow::parseEmailRecipients(" alice@example.com ,bob@example.org,  carol@example.net");
    QStringList expected;
    expected << "alice@example.com" << "bob@example.org" << "carol@example.net";
    QCOMPARE(result, expected);
}

void TestMzWatcher::parseEmailRecipients_emptyStringYieldsEmptyList()
{
    QVERIFY(MainWindow::parseEmailRecipients("").isEmpty());
    QVERIFY(MainWindow::parseEmailRecipients("   ").isEmpty());
}

void TestMzWatcher::buildWarningEmailSubject_includesFileName()
{
    QVERIFY(MainWindow::buildWarningEmailSubject("run1.raw").contains("run1.raw"));
}

void TestMzWatcher::buildWarningEmailBody_includesAllFields()
{
    QString body = MainWindow::buildWarningEmailBody("run1.raw", 42.5, 70.0, "kB", "LAB-PC-01");
    QVERIFY(body.contains("run1.raw"));
    QVERIFY(body.contains("42.5"));
    QVERIFY(body.contains("70"));
    QVERIFY(body.contains("kB"));
    QVERIFY(body.contains("LAB-PC-01"));
}

void TestMzWatcher::parseMailerConfigFile_parsesKeyValueLines()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString filePath = dir.path() + "/mailer.txt";

    QFile f(filePath);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    {
        QTextStream out(&f);
        out << "EMAIL_NAME=mailer@example.com\n";
        out << "EMAIL_PASSWORD=hunter2\n";
        out << "SMTP_SERVER=smtp.example.com\n";
        out << "SMTP_PORT=465\n";
    }
    f.close();

    QHash<QString,QString> config = MainWindow::parseMailerConfigFile(filePath);
    QCOMPARE(config.value("EMAIL_NAME"), QString("mailer@example.com"));
    QCOMPARE(config.value("EMAIL_PASSWORD"), QString("hunter2"));
    QCOMPARE(config.value("SMTP_SERVER"), QString("smtp.example.com"));
    QCOMPARE(config.value("SMTP_PORT"), QString("465"));
}

void TestMzWatcher::parseMailerConfigFile_ignoresBlankLinesAndComments()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString filePath = dir.path() + "/mailer.txt";

    QFile f(filePath);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    {
        QTextStream out(&f);
        out << "# this is a comment\n";
        out << "\n";
        out << "EMAIL_NAME=mailer@example.com\n";
        out << "   \n";
        out << "# EMAIL_PASSWORD=shouldNotBeUsed\n";
        out << "EMAIL_PASSWORD=hunter2\n";
    }
    f.close();

    QHash<QString,QString> config = MainWindow::parseMailerConfigFile(filePath);
    QCOMPARE(config.size(), 2);
    QCOMPARE(config.value("EMAIL_NAME"), QString("mailer@example.com"));
    QCOMPARE(config.value("EMAIL_PASSWORD"), QString("hunter2"));
}

void TestMzWatcher::parseMailerConfigFile_missingFileYieldsEmptyHash()
{
    QVERIFY(MainWindow::parseMailerConfigFile("").isEmpty());
    QVERIFY(MainWindow::parseMailerConfigFile("/path/does/not/exist.txt").isEmpty());
}

void TestMzWatcher::automaticWarningsSettingsDefaults()
{
    resetSettings();
    MainWindow mw(0);

    QCheckBox *checkBox = mw.findChild<QCheckBox*>("automaticWarningsCheckBox");
    QLineEdit *emailEdit = mw.findChild<QLineEdit*>("warningEmailAddressesEdit");
    QSpinBox *thresholdSpin = mw.findChild<QSpinBox*>("warningThresholdSpinBox");
    QComboBox *unitBox = mw.findChild<QComboBox*>("warningThresholdUnitBox");
    QLineEdit *computerNameEdit = mw.findChild<QLineEdit*>("warningComputerNameEdit");

    QVERIFY(checkBox != nullptr);
    QCOMPARE(checkBox->isChecked(), false);
    QVERIFY(emailEdit != nullptr);
    QCOMPARE(emailEdit->text(), QString(""));
    QVERIFY(thresholdSpin != nullptr);
    QCOMPARE(thresholdSpin->value(), 70);
    QVERIFY(unitBox != nullptr);
    QCOMPARE(unitBox->currentText(), QString("kB"));

    // First-ever construction: the widget is pre-filled with the hostname
    // even though the persisted default stays empty.
    QVERIFY(computerNameEdit != nullptr);
    QCOMPARE(computerNameEdit->text(), QSysInfo::machineHostName());

    QVERIFY(!computerNameEdit->isReadOnly());
    QVERIFY(!emailEdit->isReadOnly());

    mw.close();

    QSettings after("mzWatch", "mzWatch Settings");
    QCOMPARE(after.value("warningComputerName").toString(), QSysInfo::machineHostName());
}

QTEST_MAIN(TestMzWatcher)
#include "testmzwatcher.moc"
