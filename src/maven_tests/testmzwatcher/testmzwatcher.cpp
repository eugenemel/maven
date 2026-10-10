#include <QtTest/QtTest>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QSysInfo>
#include <QTextStream>
#include <QTreeWidget>
#include <QTextBrowser>
#include <QLabel>
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
    void effectiveFileSize_directoryReturnsRecursiveSize();
    void effectiveFileSize_plainFileReturnsOwnSize();
    void thresholdInBytes_examples();
    void parseEmailRecipients_splitsAndTrims();
    void parseEmailRecipients_emptyStringYieldsEmptyList();
    void buildWarningEmailSubject_includesFileName();
    void buildWarningEmailBody_includesAllFields();
    void parseMailerConfigFile_parsesKeyValueLines();
    void parseMailerConfigFile_ignoresBlankLinesAndComments();
    void parseMailerConfigFile_missingFileYieldsEmptyHash();
    void mailerConfigEmailRecipients_doesNotReapplyOnStartup();
    void mailerConfigEmailRecipients_importedOnceWhenFileIsSelected();
    void findMatchingSizeWarningRule_firstMatchWinsTopDown();
    void findMatchingSizeWarningRule_blankOrInvalidRegexNeverMatches();
    void findMatchingSizeWarningRule_noMatchReturnsNegativeOne();
    void warningSizeRule_addAndDeleteUpdatesContainer();
    void warningSizeRulesSettings_roundTrip();
    void automaticWarningsSettingsDefaults();
    void watchFolder_convertsDotDBundleThatNeverChangesSize();
    void watchFolder_doesNotConvertPrematurelyAfterLateGrowth();
    void watchFolder_doesNotConvertWhileContinuouslyGrowing();

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

void TestMzWatcher::effectiveFileSize_directoryReturnsRecursiveSize()
{
    // Regression test for the Agilent .d watch-folder bug: QFileInfo::size()
    // on a directory reports its own near-zero metadata size, not its
    // contents, which silently prevented .d bundles from ever being
    // recognized as "changed" and converted.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString bundlePath = dir.path() + "/sample1.d";
    QVERIFY(QDir().mkpath(bundlePath));

    QFile f(bundlePath + "/data.ms");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(QByteArray(1234, 'x'));
    f.close();

    QFileInfo bundleInfo(bundlePath);
    QVERIFY(bundleInfo.isDir());
    QCOMPARE(MainWindow::effectiveFileSize(bundleInfo), (qint64)1234);
    // Directly demonstrates the bug this guards against: QFileInfo::size()
    // itself does not reflect the bundle's contents.
    QVERIFY(bundleInfo.size() != 1234);
}

void TestMzWatcher::effectiveFileSize_plainFileReturnsOwnSize()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString filePath = dir.path() + "/run1.wiff";

    QFile f(filePath);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(QByteArray(99, 'y'));
    f.close();

    QCOMPARE(MainWindow::effectiveFileSize(QFileInfo(filePath)), (qint64)99);
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
        out << "EMAIL_ADDRESS=mailer@example.com\n";
        out << "EMAIL_PASSWORD=hunter2\n";
        out << "SMTP_SERVER=smtp.example.com\n";
        out << "SMTP_PORT=465\n";
    }
    f.close();

    QHash<QString,QString> config = MainWindow::parseMailerConfigFile(filePath);
    QCOMPARE(config.value("EMAIL_ADDRESS"), QString("mailer@example.com"));
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
        out << "EMAIL_ADDRESS=mailer@example.com\n";
        out << "   \n";
        out << "# EMAIL_PASSWORD=shouldNotBeUsed\n";
        out << "EMAIL_PASSWORD=hunter2\n";
    }
    f.close();

    QHash<QString,QString> config = MainWindow::parseMailerConfigFile(filePath);
    QCOMPARE(config.size(), 2);
    QCOMPARE(config.value("EMAIL_ADDRESS"), QString("mailer@example.com"));
    QCOMPARE(config.value("EMAIL_PASSWORD"), QString("hunter2"));
}

void TestMzWatcher::parseMailerConfigFile_missingFileYieldsEmptyHash()
{
    QVERIFY(MainWindow::parseMailerConfigFile("").isEmpty());
    QVERIFY(MainWindow::parseMailerConfigFile("/path/does/not/exist.txt").isEmpty());
}

void TestMzWatcher::mailerConfigEmailRecipients_doesNotReapplyOnStartup()
{
    // Loading a mailer config file is a one-time value transfer, not a
    // persistent sync: EMAIL_RECIPIENTS should only overwrite the Recipients
    // field at the moment the file is freshly selected, never again just
    // because the app restarts with the same file still selected -- that
    // would silently discard whatever the user typed in and saved since.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString filePath = dir.path() + "/mailer.txt";

    QFile f(filePath);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    {
        QTextStream out(&f);
        out << "EMAIL_ADDRESS=mailer@example.com\n";
        out << "EMAIL_PASSWORD=hunter2\n";
        out << "EMAIL_RECIPIENTS=alice@example.com,bob@example.org\n";
    }
    f.close();

    resetSettings();
    {
        QSettings settings("mzWatch", "mzWatch Settings");
        settings.setValue("mailerConfigFile", filePath);
        settings.setValue("warningEmailAddresses", QString("typed-in@example.com"));
    }

    MainWindow mw(0);

    QLineEdit *emailEdit = mw.findChild<QLineEdit*>("warningEmailAddressesEdit");
    QVERIFY(emailEdit != nullptr);
    QCOMPARE(emailEdit->text(), QString("typed-in@example.com"));

    QLabel *statusLabel = mw.findChild<QLabel*>("mailerConfigStatusLabel");
    QVERIFY(statusLabel != nullptr);
    QCOMPARE(statusLabel->text(), QString("Mail service configured (mailer@example.com)"));

    mw.close();
}

void TestMzWatcher::mailerConfigEmailRecipients_importedOnceWhenFileIsSelected()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString filePath = dir.path() + "/mailer.txt";

    QFile f(filePath);
    QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
    {
        QTextStream out(&f);
        out << "EMAIL_ADDRESS=mailer@example.com\n";
        out << "EMAIL_PASSWORD=hunter2\n";
        out << "EMAIL_RECIPIENTS=alice@example.com,bob@example.org\n";
    }
    f.close();

    resetSettings();
    MainWindow mw(0);

    QLineEdit *emailEdit = mw.findChild<QLineEdit*>("warningEmailAddressesEdit");
    QLineEdit *configEdit = mw.findChild<QLineEdit*>("mailerConfigFileEdit");
    QVERIFY(emailEdit != nullptr);
    QVERIFY(configEdit != nullptr);
    QCOMPARE(emailEdit->text(), QString(""));

    // Simulates selecting a new mailer config file (the same code path
    // Browse... and manually editing the field both go through).
    configEdit->setText(filePath);
    mw.getFormValues();

    QCOMPARE(emailEdit->text(), QString("alice@example.com,bob@example.org"));

    // Now the user overrides it by hand -- this must stick, not get
    // silently reverted by anything re-reading the still-selected file.
    emailEdit->setText("custom@example.com");
    mw.getFormValues();
    QCOMPARE(emailEdit->text(), QString("custom@example.com"));

    mw.close();

    QSettings after("mzWatch", "mzWatch Settings");
    QCOMPARE(after.value("warningEmailAddresses").toString(), QString("custom@example.com"));
}

void TestMzWatcher::findMatchingSizeWarningRule_firstMatchWinsTopDown()
{
    QVector<MainWindow::SizeWarningRule> rules;
    rules.append({QString("\\.d$"), 70, QString("kB")});
    rules.append({QString("sample.*"), 5, QString("MB")});
    rules.append({QString(".*"), 1, QString("GB")});

    // Matches both rule 0 and rule 2 -- rule 0 (top-down) wins.
    QCOMPARE(MainWindow::findMatchingSizeWarningRule(rules, "sample1.d"), 0);
    // Doesn't match rule 0, matches rule 1 and rule 2 -- rule 1 wins.
    QCOMPARE(MainWindow::findMatchingSizeWarningRule(rules, "sample2.wiff"), 1);
    // Matches only the catch-all last rule.
    QCOMPARE(MainWindow::findMatchingSizeWarningRule(rules, "run3.raw"), 2);
}

void TestMzWatcher::findMatchingSizeWarningRule_blankOrInvalidRegexNeverMatches()
{
    QVector<MainWindow::SizeWarningRule> rules;
    rules.append({QString(""), 70, QString("kB")});
    rules.append({QString("["), 70, QString("kB")}); // invalid regex -- unterminated character class
    rules.append({QString(".*"), 5, QString("MB")});

    QCOMPARE(MainWindow::findMatchingSizeWarningRule(rules, "anything.d"), 2);
}

void TestMzWatcher::findMatchingSizeWarningRule_noMatchReturnsNegativeOne()
{
    QVector<MainWindow::SizeWarningRule> rules;
    rules.append({QString("\\.wiff$"), 70, QString("kB")});

    QCOMPARE(MainWindow::findMatchingSizeWarningRule(rules, "sample1.d"), -1);
    QCOMPARE(MainWindow::findMatchingSizeWarningRule(QVector<MainWindow::SizeWarningRule>(), "sample1.d"), -1);
}

void TestMzWatcher::warningSizeRule_addAndDeleteUpdatesContainer()
{
    resetSettings();
    MainWindow mw(0);

    QWidget *rulesContainer = mw.findChild<QWidget*>("warningSizeRulesContainer");
    QPushButton *addButton = mw.findChild<QPushButton*>("addWarningSizeRuleButton");
    QVERIFY(rulesContainer != nullptr);
    QVERIFY(addButton != nullptr);
    QCOMPARE(rulesContainer->layout()->count(), 0);

    addButton->click();
    QCOMPARE(rulesContainer->layout()->count(), 1);

    QWidget *row = rulesContainer->layout()->itemAt(0)->widget();
    QVERIFY(row != nullptr);
    QLineEdit *regexEdit = row->findChild<QLineEdit*>();
    QSpinBox *thresholdSpin = row->findChild<QSpinBox*>();
    QComboBox *unitBox = row->findChild<QComboBox*>();
    QPushButton *deleteButton = row->findChild<QPushButton*>();

    QVERIFY(regexEdit != nullptr);
    QCOMPARE(regexEdit->text(), QString(""));
    QVERIFY(thresholdSpin != nullptr);
    QCOMPARE(thresholdSpin->value(), 70);
    QVERIFY(unitBox != nullptr);
    QCOMPARE(unitBox->currentText(), QString("kB"));
    QVERIFY(deleteButton != nullptr);

    deleteButton->click();
    QCOMPARE(rulesContainer->layout()->count(), 0);

    mw.close();
}

void TestMzWatcher::warningSizeRulesSettings_roundTrip()
{
    resetSettings();
    {
        QSettings settings("mzWatch", "mzWatch Settings");
        settings.beginWriteArray("warningSizeRules");
        settings.setArrayIndex(0);
        settings.setValue("regex", "\\.d$");
        settings.setValue("threshold", 42);
        settings.setValue("unit", "MB");
        settings.setArrayIndex(1);
        settings.setValue("regex", ".*");
        settings.setValue("threshold", 1);
        settings.setValue("unit", "GB");
        settings.endArray();
    }

    MainWindow mw(0);

    QWidget *rulesContainer = mw.findChild<QWidget*>("warningSizeRulesContainer");
    QVERIFY(rulesContainer != nullptr);
    QCOMPARE(rulesContainer->layout()->count(), 2);

    QWidget *row0 = rulesContainer->layout()->itemAt(0)->widget();
    QCOMPARE(row0->findChild<QLineEdit*>()->text(), QString("\\.d$"));
    QCOMPARE(row0->findChild<QSpinBox*>()->value(), 42);
    QCOMPARE(row0->findChild<QComboBox*>()->currentText(), QString("MB"));

    QWidget *row1 = rulesContainer->layout()->itemAt(1)->widget();
    QCOMPARE(row1->findChild<QLineEdit*>()->text(), QString(".*"));
    QCOMPARE(row1->findChild<QSpinBox*>()->value(), 1);
    QCOMPARE(row1->findChild<QComboBox*>()->currentText(), QString("GB"));

    mw.close(); // triggers writeSettings()

    QSettings after("mzWatch", "mzWatch Settings");
    int count = after.beginReadArray("warningSizeRules");
    QCOMPARE(count, 2);
    after.setArrayIndex(0);
    QCOMPARE(after.value("regex").toString(), QString("\\.d$"));
    QCOMPARE(after.value("threshold").toInt(), 42);
    QCOMPARE(after.value("unit").toString(), QString("MB"));
    after.setArrayIndex(1);
    QCOMPARE(after.value("regex").toString(), QString(".*"));
    QCOMPARE(after.value("threshold").toInt(), 1);
    QCOMPARE(after.value("unit").toString(), QString("GB"));
    after.endArray();
}

void TestMzWatcher::automaticWarningsSettingsDefaults()
{
    resetSettings();
    MainWindow mw(0);

    QCheckBox *checkBox = mw.findChild<QCheckBox*>("automaticWarningsCheckBox");
    QLineEdit *emailEdit = mw.findChild<QLineEdit*>("warningEmailAddressesEdit");
    QLineEdit *computerNameEdit = mw.findChild<QLineEdit*>("warningComputerNameEdit");
    QWidget *rulesContainer = mw.findChild<QWidget*>("warningSizeRulesContainer");

    QVERIFY(checkBox != nullptr);
    QCOMPARE(checkBox->isChecked(), true);
    QVERIFY(emailEdit != nullptr);
    QCOMPARE(emailEdit->text(), QString(""));

    // Fresh settings: no size rules exist until the user clicks "Add Rule".
    QVERIFY(rulesContainer != nullptr);
    QVERIFY(rulesContainer->layout() != nullptr);
    QCOMPARE(rulesContainer->layout()->count(), 0);

    // First-ever construction: the widget is pre-filled with the hostname
    // even though the persisted default stays empty.
    QVERIFY(computerNameEdit != nullptr);
    QCOMPARE(computerNameEdit->text(), QSysInfo::machineHostName());

    QVERIFY(!computerNameEdit->isReadOnly());
    QVERIFY(!emailEdit->isReadOnly());

    mw.close();

    QSettings after("mzWatch", "mzWatch Settings");
    QCOMPARE(after.value("warningComputerName").toString(), QSysInfo::machineHostName());
    // Enabled by default on a brand-new settings file -- the .ui file's own
    // "checked" property has no effect here, since the constructor always
    // overwrites it with this persisted value right after setupUi() runs.
    QCOMPARE(after.value("automaticWarningsEnabled").toBool(), true);
}

void TestMzWatcher::watchFolder_convertsDotDBundleThatNeverChangesSize()
{
    // Regression test for the "Size Change stuck at 0, never converts" bug:
    // an Agilent .d bundle that is already fully written the very first time
    // mzWatcher's scan ever finds it has dbFiles[file] == fileList[file] from
    // that first scan onward, since both are measured within the same scan.
    // The old logic required an *observed* size change and so never
    // converted such a bundle, no matter how long you waited.
    resetSettings();

    QTemporaryDir sourceDir;
    QTemporaryDir destDir;
    QVERIFY(sourceDir.isValid());
    QVERIFY(destDir.isValid());

    const QString bundlePath = sourceDir.path() + "/sample1.d";
    QVERIFY(QDir().mkpath(bundlePath));
    QFile f(bundlePath + "/data.ms");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(QByteArray(500, 'x'));
    f.close();

    MainWindow mw(0);

    QLineEdit *sourceEdit = mw.findChild<QLineEdit*>("sourceFolderEdit");
    QLineEdit *destEdit = mw.findChild<QLineEdit*>("destFolderEdit");
    QLineEdit *extensionEdit = mw.findChild<QLineEdit*>("extensionEdit");
    QLineEdit *commandEdit = mw.findChild<QLineEdit*>("commandEdit");
    QSpinBox *minSizeSpin = mw.findChild<QSpinBox*>("minimumsFileSize");
    QSpinBox *waitTimeSpin = mw.findChild<QSpinBox*>("converter_waitTime");
    QPushButton *watchButton = mw.findChild<QPushButton*>("watchButton");
    QTreeWidget *treeWidget = mw.findChild<QTreeWidget*>("treeWidget");
    QVERIFY(sourceEdit && destEdit && extensionEdit && commandEdit && minSizeSpin && waitTimeSpin && watchButton && treeWidget);

    sourceEdit->setText(sourceDir.path());
    destEdit->setText(destDir.path());
    extensionEdit->setText(".d");
    // "true" ignores its arguments and exits 0 immediately -- this test only
    // needs markFileConverted() to run, not a real conversion.
    commandEdit->setText("true %1 %2");
    mw.getFormValues(); // syncs sourceFolder/destFolder/extension/convertCommand

    minSizeSpin->setValue(0);
    waitTimeSpin->setValue(0); // waitTime = 0 seconds

    // Set checked without going through monitor() (toggled -> startTimer()):
    // this test drives scans manually via updateFileList(), it doesn't need
    // a real QTimer running in the background.
    watchButton->blockSignals(true);
    watchButton->setChecked(true);
    watchButton->blockSignals(false);

    // Cycle 1: first discovery. insertFileInfo() records the bundle's
    // current (already-final) size as both the DB's fileSize and
    // firstDetected. dbFiles isn't refreshed from the DB until the *next*
    // getDBFileList() call, so this cycle's processChangedFiles() doesn't
    // see this file at all yet -- by design, same one-cycle lag as any
    // newly-discovered file.
    mw.updateFileList();

    // firstDetected is stored with 1-second resolution ("yyyy-MM-dd
    // hh:mm:ss"), so secsSinceFirstDetected needs at least a full second to
    // read back as > 0 (> waitTime, which is 0 here).
    QTest::qWait(1500);

    // Cycle 2: dbFiles now has the bundle (size unchanged since cycle 1, so
    // dbFiles[file] == fileList[file]) and enough time has passed since
    // firstDetected -- this is exactly the new fallback path.
    mw.updateFileList();

    // Cycle 3: re-render the tree so it reflects the fileConverted=1 that
    // markFileConverted() wrote to the DB during cycle 2's processChangedFiles().
    mw.updateFileList();

    bool foundConvertedRow = false;
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = treeWidget->topLevelItem(i);
        if (item->text(2) == QFileInfo(bundlePath).absoluteFilePath()) {
            foundConvertedRow = (item->background(0).color() == QColor(Qt::green));
        }
    }
    QVERIFY2(foundConvertedRow, "expected the never-changing .d bundle to be auto-converted and shown green");

    watchButton->blockSignals(true);
    watchButton->setChecked(false);
    watchButton->blockSignals(false);

    mw.close();
}

void TestMzWatcher::watchFolder_doesNotConvertPrematurelyAfterLateGrowth()
{
    // Regression test for a false size-warning / premature-conversion bug:
    // a file whose size merely happened to match its first-detected size
    // across one polling gap -- then grew again -- must NOT be treated as
    // "stable," even once secsSinceFirstDetected alone would already exceed
    // waitTime. It was only coincidentally unchanged between two particular
    // polls while still actively being written by a bursty writer (data
    // flushed to disk every few minutes, not continuously), observed in
    // practice on a non-rapidfire system whose runs took far longer than the
    // configured wait period. Growth observed on any scan must reset the
    // stability clock.
    resetSettings();

    QTemporaryDir sourceDir;
    QTemporaryDir destDir;
    QVERIFY(sourceDir.isValid());
    QVERIFY(destDir.isValid());

    const QString bundlePath = sourceDir.path() + "/sample1.d";
    QVERIFY(QDir().mkpath(bundlePath));
    {
        QFile f(bundlePath + "/data.ms");
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArray(100, 'x'));
    }

    MainWindow mw(0);

    QLineEdit *sourceEdit = mw.findChild<QLineEdit*>("sourceFolderEdit");
    QLineEdit *destEdit = mw.findChild<QLineEdit*>("destFolderEdit");
    QLineEdit *extensionEdit = mw.findChild<QLineEdit*>("extensionEdit");
    QLineEdit *commandEdit = mw.findChild<QLineEdit*>("commandEdit");
    QSpinBox *minSizeSpin = mw.findChild<QSpinBox*>("minimumsFileSize");
    QSpinBox *waitTimeSpin = mw.findChild<QSpinBox*>("converter_waitTime");
    QPushButton *watchButton = mw.findChild<QPushButton*>("watchButton");
    QTreeWidget *treeWidget = mw.findChild<QTreeWidget*>("treeWidget");
    QVERIFY(sourceEdit && destEdit && extensionEdit && commandEdit && minSizeSpin && waitTimeSpin && watchButton && treeWidget);

    sourceEdit->setText(sourceDir.path());
    destEdit->setText(destDir.path());
    extensionEdit->setText(".d");
    commandEdit->setText("true %1 %2");
    mw.getFormValues();

    minSizeSpin->setValue(0);
    waitTimeSpin->setValue(0);

    watchButton->blockSignals(true);
    watchButton->setChecked(true);
    watchButton->blockSignals(false);

    auto isConvertedGreen = [&]() {
        for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
            QTreeWidgetItem *item = treeWidget->topLevelItem(i);
            if (item->text(2) == QFileInfo(bundlePath).absoluteFilePath()) {
                return item->background(0).color() == QColor(Qt::green);
            }
        }
        return false;
    };

    // Cycle 1: first discovery (one-cycle lag, same as the test above).
    mw.updateFileList();

    QTest::qWait(1500);

    // Grow the bundle right before the next scan -- simulates a bursty
    // writer still actively mid-acquisition, well past what a naive
    // secsSinceFirstDetected-only check would already consider "old enough."
    {
        QFile f(bundlePath + "/data.ms");
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Append));
        f.write(QByteArray(400, 'y'));
    }

    // Cycle 2: observes the growth. The growth and this check happen within
    // the same scan, so secsSinceLastChange reads back as 0 -- must NOT
    // convert, even though secsSinceFirstDetected is already well past
    // waitTime (0 here).
    mw.updateFileList();
    QVERIFY2(!isConvertedGreen(), "must not convert in the same cycle growth was observed");

    // One more immediate cycle, no further growth, no elapsed time: still
    // must not have converted -- confirms the previous cycle's growth
    // observation is what's being honored, not a fluke of timing.
    mw.updateFileList();
    QVERIFY2(!isConvertedGreen(), "must not convert immediately after the cycle that observed growth");

    QTest::qWait(1500);

    // Now it has genuinely been stable (no further growth) for the full
    // wait period since the growth was observed -- this is where it should
    // finally convert.
    mw.updateFileList();
    mw.updateFileList(); // re-render to reflect this cycle's conversion
    QVERIFY2(isConvertedGreen(), "should convert once genuinely stable after the growth settled");

    watchButton->blockSignals(true);
    watchButton->setChecked(false);
    watchButton->blockSignals(false);

    mw.close();
}

void TestMzWatcher::watchFolder_doesNotConvertWhileContinuouslyGrowing()
{
    // The classic pre-rapidfire case: a file observed growing across several
    // consecutive scans (not just one growth-then-stop) must never be
    // converted while that growth is ongoing, however many times it's
    // observed, and must still correctly convert once it genuinely settles.
    resetSettings();

    QTemporaryDir sourceDir;
    QTemporaryDir destDir;
    QVERIFY(sourceDir.isValid());
    QVERIFY(destDir.isValid());

    const QString filePath = sourceDir.path() + "/run1.wiff";
    {
        QFile f(filePath);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArray(100, 'a'));
    }

    MainWindow mw(0);

    QLineEdit *sourceEdit = mw.findChild<QLineEdit*>("sourceFolderEdit");
    QLineEdit *destEdit = mw.findChild<QLineEdit*>("destFolderEdit");
    QLineEdit *extensionEdit = mw.findChild<QLineEdit*>("extensionEdit");
    QLineEdit *commandEdit = mw.findChild<QLineEdit*>("commandEdit");
    QSpinBox *minSizeSpin = mw.findChild<QSpinBox*>("minimumsFileSize");
    QSpinBox *waitTimeSpin = mw.findChild<QSpinBox*>("converter_waitTime");
    QPushButton *watchButton = mw.findChild<QPushButton*>("watchButton");
    QTreeWidget *treeWidget = mw.findChild<QTreeWidget*>("treeWidget");
    QVERIFY(sourceEdit && destEdit && extensionEdit && commandEdit && minSizeSpin && waitTimeSpin && watchButton && treeWidget);

    sourceEdit->setText(sourceDir.path());
    destEdit->setText(destDir.path());
    extensionEdit->setText(".wiff");
    commandEdit->setText("true %1 %2");
    mw.getFormValues();

    minSizeSpin->setValue(0);
    waitTimeSpin->setValue(0);

    watchButton->blockSignals(true);
    watchButton->setChecked(true);
    watchButton->blockSignals(false);

    auto isConvertedGreen = [&]() {
        for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
            QTreeWidgetItem *item = treeWidget->topLevelItem(i);
            if (item->text(2) == QFileInfo(filePath).absoluteFilePath()) {
                return item->background(0).color() == QColor(Qt::green);
            }
        }
        return false;
    };

    auto appendBytes = [&](int n) {
        QFile f(filePath);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Append));
        f.write(QByteArray(n, 'a'));
    };

    // Cycle 1: first discovery (one-cycle lag).
    mw.updateFileList();
    QTest::qWait(1500);

    // Several consecutive scans, each observing further growth -- simulates
    // a long, continuously-written acquisition. Must never convert while
    // this keeps happening, no matter how many scans go by. The short gap
    // between bursts is well under a second, so each burst's own growth
    // reliably reads back as "just now" (secsSinceLastChange == 0) at the
    // scan that observes it.
    for (int i = 0; i < 4; ++i) {
        if (i > 0) QTest::qWait(300);
        appendBytes(50);
        mw.updateFileList();
        QVERIFY2(!isConvertedGreen(), "must not convert while still being observed to grow");
    }

    // Growth stops here, checked with no elapsed time since the last scan
    // that observed it -- must not convert yet, since no scan has confirmed
    // stability (this is the exact bug: converting here, right after the
    // last growth, is what a single-snapshot "equal to first detected size"
    // check would have incorrectly allowed once secsSinceFirstDetected alone
    // passed waitTime).
    mw.updateFileList();
    QVERIFY2(!isConvertedGreen(), "must not convert on the very first scan after growth stops");

    QTest::qWait(1500);

    // Now genuinely stable for the full wait period since the last real
    // change -- should convert.
    mw.updateFileList();
    mw.updateFileList(); // re-render to reflect this cycle's conversion
    QVERIFY2(isConvertedGreen(), "should convert once genuinely stable after growth stops");

    watchButton->blockSignals(true);
    watchButton->setChecked(false);
    watchButton->blockSignals(false);

    mw.close();
}

QTEST_MAIN(TestMzWatcher)
#include "testmzwatcher.moc"
