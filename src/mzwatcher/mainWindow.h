#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include<QMainWindow>
#include<QFileDialog>
#include<QMessageBox>
#include<QtGui>
#include<QtSql>
#include<QDebug>
#include<QSysInfo>
#include<QProcessEnvironment>
#include<QRegularExpression>
#include<QVector>
#include<QDateTime>
#include<QDirIterator>
#include<QRunnable>
#include<QThreadPool>
#include<functional>
#include<atomic>
#include "ui_mzWatcherGui.h"

// Common base for background work dispatched via commandQueue (a QThreadPool) --
// lets outstandingJobs/outstandingCommandKeys and enqueueCommand() track different
// kinds of work (an external command, a scan that builds more jobs, ...) generically.
class MzWatcherBackgroundJob : public QObject, public QRunnable
{
	Q_OBJECT

public:
    enum Type { CONVERSION, FILE_SYNC, WARNING_EMAIL, FILE_SYNC_GROUP };

	// Identifies "the same work" for dedup purposes -- enqueueCommand() skips a job
	// whose key is already outstanding.
	virtual QString commandKey() const = 0;

    virtual Type type() const = 0;

	// Called from the GUI thread; run() (on the worker thread) polls this rather than being killed directly.
	void cancel() { cancelled = true; }
	// For a job removed from the queue via QThreadPool::tryTake() before it ever ran -- run() will never fire finished() itself.
	void skipQueued() { emit finished(false); }

	// Lets a job carry its own completion callback (e.g. a FILE_SYNC job built inside
	// SyncScanWorker::run(), far from wherever enqueueCommand() eventually gets called
	// on it) in addition to whatever onFinished enqueueCommand()'s caller supplies.
	void setOnFinished(std::function<void(bool)> cb) { onFinishedCallback = std::move(cb); }
	const std::function<void(bool)>& onFinished() const { return onFinishedCallback; }

signals:
	void finished(bool success);

protected:
	std::atomic<bool> cancelled { false };

private:
	std::function<void(bool)> onFinishedCallback;
};


class MainWindow: public QMainWindow {
	Q_OBJECT

			public:
				MainWindow(QWidget* parent);

				// One row of the "Trigger Automatic Warning File Size Rules"
				// list: a filename regex plus the size threshold that
				// applies when a converted file's name matches it.
				struct SizeWarningRule {
					QString regex;
					int threshold;
					QString unit;
				};

				// Testable pure-logic helpers (item 1, R1; items 9-10, R13).
				// These take no GUI/filesystem/network state so they can be
				// exercised directly from QtTest.
				static bool isConvertibleMatch(const QString &entryName, const QString &extension);
				static QString buildGcsObjectPath(const QString &destFolder, const QString &convertedFilePath);
				static QStringList buildGcsUploadCommand(const QString &gcsKeyFile, const QString &targetBucket, const QString &localFilePath, const QString &objectPath);

				// Automatic Warnings feature: testable pure-logic helpers.
				static qint64 totalDirectorySize(const QString &dirPath);
				// QFileInfo::size() reports a directory's own (near-zero,
				// non-growing) metadata size, not its contents -- which
				// silently breaks any size-based check (watch-folder change
				// detection, backup-copy staleness, size warnings) for
				// directory-style convertible units like Agilent .d bundles.
				// Every such check in this class should go through this
				// instead of calling QFileInfo::size() directly.
				static qint64 effectiveFileSize(const QFileInfo &fi);
				static bool isRapidFireRunDisconnected(const QString &dotDPath);
				static qint64 thresholdInBytes(double threshold, const QString &unit);
				static QStringList parseEmailRecipients(const QString &commaSeparated);
				static QString buildWarningEmailSubject(const QString &fileName);
				static QString buildWarningEmailBody(const QString &fileName, double fileSize, double threshold, const QString &unit, const QString &computerName);
				// Rules are evaluated top-down; the first rule whose regex
				// matches fileName wins, mirroring the order rules appear in
				// the GUI. Returns -1 if none match (or the list is empty),
				// in which case no size check is performed at all -- a rule
				// list is an allowlist, not a filter with an implicit
				// catch-all. A blank or invalid regex never matches.
				static int findMatchingSizeWarningRule(const QVector<SizeWarningRule> &rules, const QString &fileName);
				// A user-supplied text file, not anything checked into this
				// repo: lines of KEY=VALUE (# comments and blank lines
				// ignored). EMAIL_ADDRESS and EMAIL_PASSWORD are required;
				// SMTP_SERVER/SMTP_PORT are optional, defaulting to Gmail's
				// (smtp.gmail.com:587) when absent, since that's the common
				// case, not because this tool is tied to any particular
				// provider. EMAIL_RECIPIENTS is also optional: when present,
				// it overrides the Recipients field in the GUI.
				static QHash<QString,QString> parseMailerConfigFile(const QString &filePath);

			public slots:
				void updateFileList();
				void getFileList(const QString &fromDir);
				void processChangedFiles();
				void processFile(QString filename);
				void processSelectedFiles();
				void getFormValues();
				void monitor();
				void setStatus(QString status);
				void stopAllJobs();
				void updateScheduledActionsLabel();
				void tickScheduledActionsCountdown();
				void selectDestFolder();
				void selectSourceFolder();
                        void clearTables();
                        void updateButtonColors();
                        void selectMailerConfigFile();
                        void addWarningSizeRuleClicked();
                        void addGcsSyncTargetClicked();
                        void addOtherSyncTargetClicked();

			protected:
				void timerEvent(QTimerEvent *event);
				bool checkParameters();
				void closeEvent(QCloseEvent *event);

			private:
				QWidget* centralWidget;
				Ui_mzWatcherGui* guiForm;
				QSet<QString>directoryList;
                        // qint64, not int: a directory's effectiveFileSize()
                        // is a real recursive byte count (unlike the old
                        // always-near-zero QFileInfo::size() for a
                        // directory), and Agilent .d bundles routinely run
                        // into the hundreds of MB to several GB -- well past
                        // what a 32-bit int can hold.
                        QHash<QString,qint64>dbFiles;
                        QHash<QString,qint64>fileList;
                        // Lets showDataFilesTable() update existing rows in place instead of
                        // destroying and recreating the whole tree every call.
                        QHash<QString,QTreeWidgetItem*> dataFileItems;

                        // "Never detected" and "detected, currently reporting
                        // size N" are different states, even when N is the
                        // same value twice in a row -- a file/.d bundle that
                        // is already finished the very first time mzWatcher
                        // ever sees it has dbFiles[file] == fileList[file]
                        // from that very first scan onward, and would
                        // otherwise never be recognized as "done" (see
                        // processChangedFiles()). firstDetectedTimes records
                        // when WE first saw each file, independent of the
                        // filesystem's own lastModified() -- which for a
                        // directory only updates on structural changes
                        // (entries added/removed), not on writes to files
                        // already inside it, so it can't be trusted to say
                        // "this bundle has stopped changing."
                        QHash<QString,QDateTime> firstDetectedTimes;
                        // When the size most recently actually changed, as
                        // continuously re-verified on every scan (updated in
                        // getFileList() whenever a file's size differs from
                        // what it was on the previous scan, not just the
                        // first one). This is what processChangedFiles() uses
                        // to decide "stable for the full wait period" -- a
                        // single comparison against the first-ever-seen size
                        // is not enough, since a bursty writer can
                        // coincidentally show no growth across one polling
                        // gap while still mid-acquisition.
                        QHash<QString,QDateTime> lastSizeChangeTimes;
                        // Tracked explicitly rather than inferred from the
                        // dbFiles/fileList size comparison: a stable,
                        // ALREADY-converted file also has dbFiles[file] ==
                        // fileList[file], so without this the "never
                        // changed" fallback below would try to reconvert
                        // every old, already-converted file on every scan.
                        QSet<QString> convertedFiles;


				QString extension;
				QString sourceFolder;
				QString destFolder;
				QString convertCommand;
				QThreadPool* commandQueue;
                QSet<MzWatcherBackgroundJob*> outstandingJobs;
				QSet<QString> outstandingCommandKeys;
                void enqueueCommand(MzWatcherBackgroundJob *job, std::function<void(bool success)> onFinished = std::function<void(bool success)>());

				// One dynamically-added GCS sync target row: the widgets
				// themselves are the source of truth (read directly at
				// write-settings/upload time), mirroring the
				// warningSizeRuleWidgets pattern. A row with either field
				// empty is skipped at upload time (R11), same gating as the
				// old single-target settings.
				struct GcsSyncTargetWidgets {
					QWidget* rowWidget;
					QLineEdit* bucketEdit;
					QLineEdit* keyFileEdit;
					QLineEdit* additionalFilesEdit;
				};
				QList<GcsSyncTargetWidgets> gcsSyncTargetWidgets;
				void addGcsSyncTarget(const QString &bucket, const QString &keyFile, const QString &additionalFiles = QString());
				void removeGcsSyncTarget(QWidget *rowWidget);
				// Re-applies alternating backgrounds across all current groups
				// by list position -- called after every add/delete so the
				// stripe pattern stays correct even after a middle group is
				// removed (not just assigned once at creation time).
				void restripeGcsSyncTargets();

				struct GcsSyncTarget {
					QString bucket;
					QString keyFile;
					QString additionalFiles;
				};
				QVector<GcsSyncTarget> collectGcsSyncTargets() const;

				// Per-file state for the Watch Directory poller below -- lastSize
				// == -1 means "not seen yet"; lastUploadedSize tracks what was
				// last successfully uploaded so an unchanged file isn't
				// re-uploaded on every poll.
				struct WatchedFileState {
					QDateTime firstDetected;
					QDateTime lastSizeChangeTime;
					qint64 lastSize = -1;
					qint64 lastUploadedSize = -1;
				};
				struct OtherSyncTargetWidgets {
					QWidget* rowWidget;
					QLineEdit* bucketEdit;
					QLineEdit* keyFileEdit;
					QLineEdit* localFilePathEdit;
					QLineEdit* nameRegexEdit;
					int watchTimerId = 0;
					QHash<QString, WatchedFileState> watchedFiles;
				};
				QList<OtherSyncTargetWidgets> otherSyncTargetWidgets;
				void addOtherSyncTarget(const QString &bucket, const QString &keyFile, const QString &localFilePath, const QString &nameRegex, bool watching = false);
				void removeOtherSyncTarget(QWidget *rowWidget);
				void restripeOtherSyncTargets();
				void startOtherSyncWatch(QWidget *rowWidget);
				void stopOtherSyncWatch(QWidget *rowWidget);
				void scanOtherSyncTarget(QWidget *rowWidget);

				struct OtherSyncTarget {
					QString bucket;
					QString keyFile;
					QString localDirectory;
					QString nameRegex;
				};
				void syncOtherTarget(const OtherSyncTarget &target);
				void uploadFileToOtherTarget(const OtherSyncTarget &target, const QString &filePath, const QString &objectPath, std::function<void(bool success)> onFinished = std::function<void(bool success)>());
				void recordOtherSyncUpload(const QString &bucket, const QString &localDirectory, const QString &filePath, qint64 fileSize);
				qint64 lookupOtherSyncUpload(const QString &bucket, const QString &filePath);

				// Automatic Warnings settings. Both automaticWarningsCheckBox
				// (read directly off the widget) and a
				// mailerConfigFile that actually parses (EMAIL_ADDRESS and
				// EMAIL_PASSWORD both present) must hold for the feature to
				// do anything -- mirrors the GCS "both settings must be set"
				// gating pattern.
				QString warningComputerName;
				QString warningEmailAddresses;

				// One dynamically-added row of the size-rules list: the
				// widgets themselves are the source of truth (read directly
				// at write-settings/check time), mirroring how other
				// QLineEdit-backed settings in this class work.
				struct SizeWarningRuleWidgets {
					QWidget* rowWidget;
					QLineEdit* regexEdit;
					QSpinBox* thresholdSpinBox;
					QComboBox* unitBox;
				};
				QList<SizeWarningRuleWidgets> warningSizeRuleWidgets;
				void addWarningSizeRule(const QString &regex, int threshold, const QString &unit);
				void removeWarningSizeRule(QWidget *rowWidget);
				QVector<SizeWarningRule> collectSizeWarningRules() const;

				// Path to a user-supplied mailer config file (see
				// parseMailerConfigFile()). Only the path is persisted;
				// EMAIL_ADDRESS/EMAIL_PASSWORD are read from the file itself
				// each time a warning email is sent, never duplicated into
				// QSettings.
				QString mailerConfigFile;

				// Set by readSettings() to indicate "warningComputerName" had
				// never been persisted before this run, so the constructor
				// knows to pre-fill the *widget* (not the stored default)
				// with the machine's hostname.
				bool warningComputerNameWasUnset;

                        int timerId;
				unsigned int maxDayDiff;

				QSqlDatabase DB;

				QSettings* settings;
				void readSettings();
				void writeSettings();

				// Legacy misspelled settings key predating the R4 rename to
				// "extension". Used only by the one-time migration in
				// readSettings() so an existing user's saved value is carried
				// forward rather than silently reset to the default.
				static const QString kLegacyExtensionSettingsKey;


				//database functions
                        void createTables();
				void insertFileInfo(QString);
				void showDataFilesTable();
				void markFileConverted(QString filename);
                        void getDBFileList();

				void makeBackupCopy(QString filename);
				static bool copyRecursively(const QString &sourcePath, const QString &destPath);
				void uploadConvertedFileToGcs(const QString &localConvertedFile);
				void uploadFileToGcsTarget(const GcsSyncTarget &target, const QString &localConvertedFile);
				void recordConvertedFileSyncUpload(const QString &bucket, const QString &filePath, qint64 fileSize);
				qint64 lookupConvertedFileSyncUpload(const QString &bucket, const QString &filePath);
				void syncGcsTarget(const GcsSyncTarget &target);
				void checkAndSendSizeWarning(const QFileInfo &sourceFileInfo);
				void sendWarningEmail(const QStringList &recipients, const QString &subject, const QString &body);
				// Re-reads mailerConfigFile and updates mailerConfigStatusLabel
				// to reflect whether it currently parses to a usable
				// (EMAIL_ADDRESS + EMAIL_PASSWORD present) configuration. Safe
				// to call anytime, including on every startup with the same
				// file already selected -- never touches any other setting,
				// so it can't clobber a value the user has since edited by
				// hand. Only updateMailerConfigFile()'s one-time import does
				// that, and only when the file itself is newly selected.
				void updateMailerConfigStatus();
				// Called only when mailerConfigFile is newly selected/changed
				// (Browse... or typing a new path), never on ordinary
				// startup: copies EMAIL_RECIPIENTS (if present) into
				// warningEmailAddresses/the Recipients field as a one-time
				// value transfer, the same way picking a value from a preset
				// would. From that point on it's an ordinary GUI-backed
				// setting -- freely editable, and whatever the user leaves
				// there is what gets saved, not re-synced from the file
				// again until it's reselected.
				void importMailerConfigFileValues();

};

// Run via a QThreadPool (commandQueue) so jobs queue and run without blocking the GUI thread.
class CommandJob : public MzWatcherBackgroundJob
{
    Q_OBJECT

public:
    CommandJob(const QString &program, const QStringList &arguments, Type type)
        : program(program), arguments(arguments), jobType(type), useArgumentList(true) {}
    CommandJob(const QString &command, Type type)
        : program(command), jobType(type), useArgumentList(false) {}

    MzWatcherBackgroundJob::Type type() const override { return jobType; }

    // Identifies "the same command" for dedup purposes -- the full command line, reconstructed if given as (program, arguments).
    QString commandKey() const override {
        QString key = useArgumentList ? (program + " " + arguments.join(" ")) : program;
        // The mailer reuses one fixed script path for every email -- recipients/subject/body
        // vary only via environment variables, which must be in the key or every warning
        // email looks identical and only the first one of a batch would ever be sent.
        if (useCustomEnvironment) {
            QStringList envList = environment.toStringList();
            envList.sort();
            key += "\n" + envList.join("\n");
        }
        return key;
    }

    // For a command (e.g. the mailer) that needs a secret passed via environment variable rather than argv.
    void setProcessEnvironment(const QProcessEnvironment &env) { environment = env; useCustomEnvironment = true; }

    void run() override {
        QProcess process;
        if (useCustomEnvironment) {
            process.setProcessEnvironment(environment);
        }
        if (useArgumentList) {
            process.start(program, arguments);
        } else {
            process.start(program);
        }
        if (!process.waitForStarted()) { emit finished(false); return; }

        while (!process.waitForFinished(100)) {
            if (cancelled) {
                process.terminate();
                process.waitForFinished(1000);
                emit finished(false);
                return;
            }
        }

        emit finished(process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0);
    }

private:
    QString program;
    QStringList arguments;
    MzWatcherBackgroundJob::Type jobType;
    bool useArgumentList;
    QProcessEnvironment environment;
    bool useCustomEnvironment = false;
};


// Used with Converted File Sync 'Sync' button - sync a large batch of already-converted files
class SyncScanWorker : public MzWatcherBackgroundJob
{
    Q_OBJECT

public:
    // recordUpload is called (on the GUI thread, via enqueueCommand()'s onFinished handling)
    // after a given FILE_SYNC job succeeds -- injected rather than taking a MainWindow*
    // directly, since MainWindow::recordConvertedFileSyncUpload() is private and this is
    // the one piece of it SyncScanWorker actually needs.
    SyncScanWorker(const QStringList &candidateFiles, const QHash<QString, qint64> &knownUploads,
                   const QString &bucket, const QString &keyFile, const QString &sourceFolder,
                   const QString &destFolder, const QString &extension, const QString &destFileFormat,
                   const QString &additionalFiles,
                   std::function<void(const QString&, const QString&, qint64)> recordUpload)
        : candidateFiles(candidateFiles), knownUploads(knownUploads),
        bucket(bucket), keyFile(keyFile), sourceFolder(sourceFolder), destFolder(destFolder),
        extension(extension), destFileFormat(destFileFormat), additionalFiles(additionalFiles),
        recordUpload(std::move(recordUpload)) {}

    MzWatcherBackgroundJob::Type type() const override { return FILE_SYNC_GROUP; }

    QString commandKey() const override {
        return "FILE_SYNC_GROUP: " + bucket;
    }

    void run() override {
        foreach (const QString &sourceFile, candidateFiles) {
            QString file = sourceFile;
            file.replace(sourceFolder, destFolder);
            file.replace(QRegularExpression("\\." + QRegularExpression::escape(extension) + "$", QRegularExpression::CaseInsensitiveOption), "." + destFileFormat);

            evaluateFile(file);

            QString sourceDir = QFileInfo(file).absolutePath();
            foreach (const QString &additionalFile, additionalFiles.split(",")) {
                QString fileName = additionalFile.trimmed();
                if (fileName.isEmpty()) continue;
                evaluateFile(sourceDir + "/" + fileName);
            }
        }
        emit finished(true);
    }

    // Keyed by local file path, value is the size observed during the scan.
    QHash<QString, qint64> takeFilesToUpload() { return filesToUpload; }
    QSet<CommandJob*> takeFileSyncJobs() { return fileSyncs; }

private:
    // Stats one candidate file (the main converted file or one of its additional files) and,
    // if it needs uploading, builds its FILE_SYNC job with the record-on-success callback attached.
    void evaluateFile(const QString &file) {
        QFileInfo fi(file);
        if (!fi.exists()) return;
        qint64 size = fi.size();
        if (knownUploads.value(file, -1) == size) return;
        filesToUpload.insert(file, size);

        QString objectPath = MainWindow::buildGcsObjectPath(destFolder, file);
        QStringList command = MainWindow::buildGcsUploadCommand(keyFile, bucket, file, objectPath);
        if (command.isEmpty()) return;

        CommandJob *job = new CommandJob(command.first(), command.mid(1), CommandJob::FILE_SYNC);
        QString uploadBucket = bucket;
        auto record = recordUpload;
        job->setOnFinished([record, uploadBucket, file, size](bool success) {
            if (success) record(uploadBucket, file, size);
        });
        fileSyncs.insert(job);
    }

    QStringList candidateFiles;
    QHash<QString, qint64> knownUploads;
    QString bucket;
    QString keyFile;
    QString sourceFolder;
    QString destFolder;
    QString extension;
    QString destFileFormat;
    QString additionalFiles;
    std::function<void(const QString&, const QString&, qint64)> recordUpload;
    QHash<QString, qint64> filesToUpload;
    QSet<CommandJob*> fileSyncs;
};

#endif
