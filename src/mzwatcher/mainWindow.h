#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include<QMainWindow>
#include<QFileDialog>
#include<QtGui>
#include<QtSql>
#include<QtNetwork/QNetworkAccessManager>
#include<QFileSystemWatcher>
#include<QDebug>
#include<QUrlQuery>
#include<QSysInfo>
#include<QProcessEnvironment>
#include<QRegularExpression>
#include<QVector>
#include<QDateTime>
#include "ui_mzWatcherGui.h"

class BackgroundThread : public QThread
{
	Q_OBJECT

public:
	BackgroundThread(QWidget*) { _stopped=false; _useArgumentList=false; _useCustomEnvironment=false; }
	void setSystemCommand(QString cmd) { command = cmd; arguments.clear(); _useArgumentList=false; }
	void setSystemCommand(QString program, QStringList args) { command = program; arguments = args; _useArgumentList=true; }
	// Issue 859: the mailer needs to pass a password to a child process
	// without it ever appearing on that process's own command line (visible
	// to any user via Task Manager/tasklist without special privileges) --
	// an inherited-but-overridden environment variable is a meaningfully
	// better (if not perfect) place for a short-lived secret than argv.
	void setProcessEnvironment(const QProcessEnvironment &env) { environment = env; _useCustomEnvironment=true; }
	void killProcess() { _stopped=true; }

signals:
	void statusChanged(QString);

protected:
	void run(void) {
		QProcess converter;
		if (_useCustomEnvironment) {
			converter.setProcessEnvironment(environment);
		}
		if (_useArgumentList) {
			converter.start(command, arguments);
		} else {
			converter.start(command);
		}

		_stopped=false;

		emit statusChanged("Running " + command);

		if (!converter.waitForStarted()) {
                        emit statusChanged("Command Failed!");
			converter.kill();
			return;
		}

		while (!converter.waitForFinished()) {
			emit statusChanged("Running conversion");
			if (_stopped) {
				converter.terminate();;
				emit statusChanged("Job killed");
				_stopped=false;
				return;
			}
			sleep(1);
		};

		emit statusChanged("Conversion done!");
	}

private:
	bool _stopped;
	bool _useArgumentList;
	bool _useCustomEnvironment;
	QString command;
	QStringList arguments;
	QProcessEnvironment environment;

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
                        void stop_startConversion();
				void startConversion();
				void stopConversion();
				void selectDestFolder();
				void selectSourceFolder();
				void selectGcsKeyFile();
                        void clearTables();
                        void updateButtonColors();
                        void selectMailerConfigFile();
                        void addWarningSizeRuleClicked();

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
				BackgroundThread* converter;
				BackgroundThread* gcsUploader;

				// GCS upload settings (R8). Both empty/unset means the upload
				// path is never taken (R11).
				QString gcsKeyFile;
				QString targetBucket;

				// Automatic Warnings settings. Both automaticWarningsCheckBox
				// (read directly off the widget, same as remoteLoging) and a
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
				// QSettings -- same trust model as gcsKeyFile.
				QString mailerConfigFile;

				BackgroundThread* mailer;

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

				QString createTempPath();

				void convertFile(QString filename);
				void makeBackupCopy(QString filename);
				static bool copyRecursively(const QString &sourcePath, const QString &destPath);
				void uploadConvertedFileToGcs(const QString &localConvertedFile);
				void checkAndSendSizeWarning(const QFileInfo &sourceFileInfo);
				void sendWarningEmail(const QStringList &recipients, const QString &subject, const QString &body);
				// Re-reads mailerConfigFile and updates mailerConfigStatusLabel
				// to reflect whether it currently parses to a usable
				// (EMAIL_ADDRESS + EMAIL_PASSWORD present) configuration. Called
				// on startup and whenever the user picks a new file.
				void updateMailerConfigStatus();


                        //remote database connection
                        int connectionId;
                        QNetworkAccessManager http;
                        void remoteLogMessage(QString infotype, QString filename, int fileSize, QString msgText);
                        //void readRemoteData(const QHttpResponseHeader &resp);

};



#endif
