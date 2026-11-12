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
    enum Type { CONVERSION, FILE_SYNC, WARNING_EMAIL, FILE_SYNC_GROUP, SOURCE_FOLDER_SCAN };

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
	// Emitted as the first thing run() does, on the worker thread -- queued back to whoever
	// connects from the GUI thread, same as finished() below.
	void started();
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
				static QStringList additionalFileCandidates(const QString &sourceFile, const QString &destFileDir, const QString &fileName);

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
                        void clearLog();
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
				// Mirrors every setStatus() line to disk (same directory as the
				// sqlite DB) so the Process Log survives the app being closed --
				// the GUI's logWidget is in-memory only.
				QFile logFile;

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
				void uploadConvertedFileToGcs(const QString &sourceFilePath, const QString &localConvertedFile);
				void uploadFileToGcsTarget(const GcsSyncTarget &target, const QString &sourceFilePath, const QString &localConvertedFile);
				void recordConvertedFileSyncUpload(const QString &bucket, const QString &filePath, qint64 fileSize);
				qint64 lookupConvertedFileSyncUpload(const QString &bucket, const QString &filePath);
				void syncGcsTarget(const GcsSyncTarget &target);

                // Issue 872: watch-timer-driven dispatch of a SourceFileScanWorker
				void enqueueSourceFolderScan();

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

            evaluateFile(file, file);

            QString destDir = QFileInfo(file).absolutePath();
            foreach (const QString &additionalFile, additionalFiles.split(",")) {
                QString fileName = additionalFile.trimmed();
                if (fileName.isEmpty()) continue;
                foreach (const QString &candidate, MainWindow::additionalFileCandidates(sourceFile, destDir, fileName)) {
                    evaluateFile(candidate, destDir + "/" + fileName);
                }
            }
        }
        emit finished(true);
    }

    QSet<CommandJob*> takeFileSyncJobs() { return fileSyncs; }

private:
    // Stats one candidate file (the main converted file or one of its additional files) and,
    // if it needs uploading, builds its FILE_SYNC job with the record-on-success callback
    // attached. localFilePath is what actually gets read and uploaded; objectPathSource is only
    // used to name the GCS object, which lets an Additional File read from inside a .d bundle
    // still land at the same bucket location as if it had been sitting flat next to the mzML.
    void evaluateFile(const QString &localFilePath, const QString &objectPathSource) {
        QFileInfo fi(localFilePath);
        if (!fi.exists()) return;
        qint64 size = fi.size();
        if (knownUploads.value(localFilePath, -1) == size) return;

        QString objectPath = MainWindow::buildGcsObjectPath(destFolder, objectPathSource);
        QStringList command = MainWindow::buildGcsUploadCommand(keyFile, bucket, localFilePath, objectPath);
        if (command.isEmpty()) return;

        CommandJob *job = new CommandJob(command.first(), command.mid(1), CommandJob::FILE_SYNC);
        QString uploadBucket = bucket;
        auto record = recordUpload;
        job->setOnFinished([record, uploadBucket, localFilePath, size](bool success) {
            if (success) record(uploadBucket, localFilePath, size);
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
    QSet<CommandJob*> fileSyncs;
};

// Used with Other File Sync 'Sync' button
class OtherFileSyncScanWorker : public MzWatcherBackgroundJob
{
    Q_OBJECT

public:
    OtherFileSyncScanWorker(const QString &localDirectory, const QString &nameRegex,
                   const QHash<QString, qint64> &knownUploads,
                   const QString &bucket, const QString &keyFile,
                   std::function<void(const QString&, const QString&, qint64)> recordUpload)
        : localDirectory(localDirectory), nameRegex(nameRegex), knownUploads(knownUploads),
        bucket(bucket), keyFile(keyFile), recordUpload(std::move(recordUpload)) {}

    MzWatcherBackgroundJob::Type type() const override { return FILE_SYNC_GROUP; }

    QString commandKey() const override {
        return "OTHER_FILE_SYNC_GROUP: " + bucket + ": " + localDirectory;
    }

    void run() override {
        QRegularExpression regex(nameRegex);

        QDirIterator it(localDirectory, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString file = it.next();
            QString fileName = QFileInfo(file).fileName();
            if (!nameRegex.isEmpty() && !regex.match(fileName).hasMatch()) continue;

            QFileInfo fi(file);
            if (!fi.exists()) continue;
            qint64 size = fi.size();
            if (knownUploads.value(file, -1) == size) continue;

            QString objectPath = QDir(localDirectory).relativeFilePath(file);
            QStringList command = MainWindow::buildGcsUploadCommand(keyFile, bucket, file, objectPath);
            if (command.isEmpty()) continue;

            CommandJob *job = new CommandJob(command.first(), command.mid(1), CommandJob::FILE_SYNC);
            QString uploadBucket = bucket;
            auto record = recordUpload;
            job->setOnFinished([record, uploadBucket, file, size](bool success) {
                if (success) record(uploadBucket, file, size);
            });
            fileSyncs.insert(job);
        }
        emit finished(true);
    }

    QSet<CommandJob*> takeFileSyncJobs() { return fileSyncs; }

private:
    QString localDirectory;
    QString nameRegex;
    QHash<QString, qint64> knownUploads;
    QString bucket;
    QString keyFile;
    std::function<void(const QString&, const QString&, qint64)> recordUpload;
    QSet<CommandJob*> fileSyncs;
};

// Issue 872
// Automatically triggered on a Source Folder that is being watched. (File Converter Settings tab)
//
// Takes over getFileList()'s directory walk and processChangedFiles()'s readiness checks,
// and also runs the Converted File Sync catch-up (main file + Additional Files) that
// syncGcsTarget()/SyncScanWorker only ever ran from the manual Sync button -- so a companion
// Additional File that doesn't exist yet the instant its mzML finishes converting gets picked
// up on the very next scan instead of only when a human clicks Sync (Issue 872).
//
// SQL and widget access have to stay on the GUI thread (see syncGcsTarget()'s comment on this),
// so this class never touches MainWindow::DB or any QWidget. Everything it needs comes in as a
// plain-data snapshot via the constructor, and everything it finds comes back out through the
// take*() accessors below for the GUI thread to act on.
class SourceFileScanWorker : public MzWatcherBackgroundJob {
     Q_OBJECT
public:
    // Mirrors MainWindow::GcsSyncTarget's shape -- that struct is private to MainWindow, so it
    // can't be reused here directly; the caller translates collectGcsSyncTargets() into these.
    struct GcsTarget {
        QString bucket;
        QString keyFile;
        QString additionalFiles;
    };

    SourceFileScanWorker(const QString &sourceFolder, const QString &destFolder, const QString &extension,
                   const QString &destFileFormat, int minFileSize, int waitTimeSecs, int maxDayDiff,
                   const QHash<QString, qint64> &dbFiles, const QSet<QString> &convertedFiles,
                   const QHash<QString, QDateTime> &firstDetectedTimes,
                   const QHash<QString, qint64> &fileList, const QHash<QString, QDateTime> &lastSizeChangeTimes,
                   const QVector<GcsTarget> &gcsSyncTargets, const QStringList &convertedFilenames,
                   const QHash<QString, QHash<QString, qint64>> &knownUploadsByBucket,
                   std::function<void(const QString&, const QString&, qint64)> recordUpload)
        : sourceFolder(sourceFolder), destFolder(destFolder), extension(extension), destFileFormat(destFileFormat),
        minFileSize(minFileSize), waitTimeSecs(waitTimeSecs), maxDayDiff(maxDayDiff),
        dbFiles(dbFiles), convertedFiles(convertedFiles), firstDetectedTimes(firstDetectedTimes),
        fileList(fileList), lastSizeChangeTimes(lastSizeChangeTimes),
        gcsSyncTargets(gcsSyncTargets), convertedFilenames(convertedFilenames),
        knownUploadsByBucket(knownUploadsByBucket), recordUpload(std::move(recordUpload)) {}

    MzWatcherBackgroundJob::Type type() const override { return SOURCE_FOLDER_SCAN; }

    // Key is intentionally set up so there is never more than one key of this type in the queue.
    // A new key will try to be added periodically, this explicitly prevents piling up too many scan jobs.
    QString commandKey() const override {
        return QString("SOURCE_FILE_SCAN_WORKER");
    }

    void run() override {
        emit started();
        scanDirectory(sourceFolder);
        evaluateReadiness();
        evaluateGcsCatchUp();
        emit finished(true);
    }

    QSet<CommandJob*> takeJobs() { return jobs; }
    QStringList takeNewFiles() const { return newFiles; }
    QStringList takeReadyToConvertFiles() const { return readyToConvertFiles; }
    QHash<QString, qint64> takeUpdatedFileList() const { return updatedFileList; }
    QHash<QString, QDateTime> takeUpdatedLastSizeChangeTimes() const { return updatedLastSizeChangeTimes; }

private:
    // Mirrors getFileList()'s recursive walk, minus the QApplication::processEvents() calls it
    // used to need to keep the GUI responsive while blocking it on the GUI thread -- unneeded
    // now that this runs on a background thread instead.
    void scanDirectory(const QString &fromDir) {
        QDir dir(fromDir);
        if (!dir.exists()) return;

        QFileInfo dirInfo(fromDir);
        if (visitedDirectories.contains(dirInfo.absoluteFilePath())) return;
        visitedDirectories.insert(dirInfo.absoluteFilePath());

        const QFileInfoList entries = dir.entryInfoList(QStringList(),
                                              QDir::Files | QDir::Dirs | QDir::NoSymLinks | QDir::Readable,
                                              QDir::Name | QDir::DirsFirst | QDir::IgnoreCase);

        QDateTime now = QDateTime::currentDateTime();
        foreach (const QFileInfo &fi, entries) {
            if (fi.fileName() == "." || fi.fileName() == "..") continue;

            if (MainWindow::isConvertibleMatch(fi.fileName(), extension)) {
                QString absFilePath = fi.absoluteFilePath();
                qint64 size = MainWindow::effectiveFileSize(fi);

                if (!fileList.contains(absFilePath) || fileList.value(absFilePath) != size) {
                    updatedLastSizeChangeTimes[absFilePath] = now;
                } else if (lastSizeChangeTimes.contains(absFilePath)) {
                    updatedLastSizeChangeTimes[absFilePath] = lastSizeChangeTimes.value(absFilePath);
                }
                updatedFileList[absFilePath] = size;

                if ((now.daysTo(fi.lastModified())) * -1 < maxDayDiff && !dbFiles.contains(absFilePath)) {
                    newFiles << absFilePath;
                }
            } else if (fi.isDir() && !visitedDirectories.contains(fi.absoluteFilePath())) {
                scanDirectory(fi.absoluteFilePath());
            }
        }
    }

    // Mirrors processChangedFiles()'s readiness checks, reading the size/change-time
    // bookkeeping scanDirectory() just produced above, the same way the original ran
    // getFileList() and then processChangedFiles() in sequence on every timer tick.
    void evaluateReadiness() {
        static const int kOneDaySecs = 3600 * 24;
        QDateTime now = QDateTime::currentDateTime();

        foreach (const QString &file, dbFiles.keys()) {
            if (convertedFiles.contains(file)) continue;

            QFileInfo fi(file);
            int ageSec = now.secsTo(fi.lastModified()) * -1;
            if (ageSec >= kOneDaySecs) continue;

            qint64 currentSize = MainWindow::effectiveFileSize(fi);
            if (currentSize <= minFileSize) continue;

            if (fi.isDir() && fi.suffix().compare("d", Qt::CaseInsensitive) == 0
                    && MainWindow::isRapidFireRunDisconnected(file)) {
                readyToConvertFiles << file;
                continue;
            }

            int secsSinceFirstDetected = firstDetectedTimes.value(file, now).secsTo(now);
            if (secsSinceFirstDetected <= waitTimeSecs) continue;

            QDateTime lastChange = updatedLastSizeChangeTimes.value(file, lastSizeChangeTimes.value(file, now));
            int secsSinceLastChange = lastChange.secsTo(now);
            if (secsSinceLastChange <= waitTimeSecs) continue;

            readyToConvertFiles << file;
        }
    }

    // Mirrors SyncScanWorker's catch-up scan (main converted file + its configured Additional
    // Files), run here for every configured GCS sync target on every automatic scan rather than
    // only when the manual Sync button builds a SyncScanWorker -- this is the actual Issue 872
    // fix once this worker is wired in.
    void evaluateGcsCatchUp() {
        foreach (const GcsTarget &target, gcsSyncTargets) {
            QHash<QString, qint64> knownUploads = knownUploadsByBucket.value(target.bucket);

            foreach (const QString &sourceFile, convertedFilenames) {
                QString file = sourceFile;
                file.replace(sourceFolder, destFolder);
                file.replace(QRegularExpression("\\." + QRegularExpression::escape(extension) + "$", QRegularExpression::CaseInsensitiveOption), "." + destFileFormat);

                evaluateFile(file, target.bucket, target.keyFile, knownUploads, file);

                QString destDir = QFileInfo(file).absolutePath();
                foreach (const QString &additionalFile, target.additionalFiles.split(",")) {
                    QString fileName = additionalFile.trimmed();
                    if (fileName.isEmpty()) continue;
                    foreach (const QString &candidate, MainWindow::additionalFileCandidates(sourceFile, destDir, fileName)) {
                        evaluateFile(candidate, target.bucket, target.keyFile, knownUploads, destDir + "/" + fileName);
                    }
                }
            }
        }
    }

    // Stats one candidate file (the main converted file or one of its additional files) and,
    // if it needs uploading, builds its FILE_SYNC job with the record-on-success callback
    // attached -- same shape as SyncScanWorker::evaluateFile(). localFilePath is what actually
    // gets read and uploaded; objectPathSource only names the GCS object, so an Additional File
    // read from inside a .d bundle still lands at the same bucket location as a flat one would.
    void evaluateFile(const QString &localFilePath, const QString &bucket, const QString &keyFile, const QHash<QString, qint64> &knownUploads, const QString &objectPathSource) {
        QFileInfo fi(localFilePath);
        if (!fi.exists()) return;
        qint64 size = fi.size();
        if (knownUploads.value(localFilePath, -1) == size) return;

        QString objectPath = MainWindow::buildGcsObjectPath(destFolder, objectPathSource);
        QStringList command = MainWindow::buildGcsUploadCommand(keyFile, bucket, localFilePath, objectPath);
        if (command.isEmpty()) return;

        CommandJob *job = new CommandJob(command.first(), command.mid(1), CommandJob::FILE_SYNC);
        QString uploadBucket = bucket;
        auto record = recordUpload;
        job->setOnFinished([record, uploadBucket, localFilePath, size](bool success) {
            if (success) record(uploadBucket, localFilePath, size);
        });
        jobs.insert(job);
    }

    QString sourceFolder;
    QString destFolder;
    QString extension;
    QString destFileFormat;
    int minFileSize;
    int waitTimeSecs;
    int maxDayDiff;

    QHash<QString, qint64> dbFiles;
    QSet<QString> convertedFiles;
    QHash<QString, QDateTime> firstDetectedTimes;
    QHash<QString, qint64> fileList;
    QHash<QString, QDateTime> lastSizeChangeTimes;

    QVector<GcsTarget> gcsSyncTargets;
    QStringList convertedFilenames;
    QHash<QString, QHash<QString, qint64>> knownUploadsByBucket;
    std::function<void(const QString&, const QString&, qint64)> recordUpload;

    QSet<QString> visitedDirectories;
    QStringList newFiles;
    QStringList readyToConvertFiles;
    QHash<QString, qint64> updatedFileList;
    QHash<QString, QDateTime> updatedLastSizeChangeTimes;
    QSet<CommandJob*> jobs;
};



#endif
