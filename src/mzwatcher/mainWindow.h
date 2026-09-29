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
#include "ui_mzWatcherGui.h"

class BackgroundThread : public QThread
{
	Q_OBJECT

public:
	BackgroundThread(QWidget*) { _stopped=false; _useArgumentList=false; }
	void setSystemCommand(QString cmd) { command = cmd; arguments.clear(); _useArgumentList=false; }
	void setSystemCommand(QString program, QStringList args) { command = program; arguments = args; _useArgumentList=true; }
	void killProcess() { _stopped=true; }

signals:
	void statusChanged(QString);

protected:
	void run(void) {
		QProcess converter;
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
	QString command;
	QStringList arguments;

};

class MainWindow: public QMainWindow {
	Q_OBJECT

			public:
				MainWindow(QWidget* parent);

				// Testable pure-logic helpers (item 1, R1; items 9-10, R13).
				// These take no GUI/filesystem/network state so they can be
				// exercised directly from QtTest.
				static bool isConvertibleMatch(const QString &entryName, const QString &extension);
				static QString buildGcsObjectPath(const QString &destFolder, const QString &convertedFilePath);
				static QStringList buildGcsUploadCommand(const QString &gcsKeyFile, const QString &targetBucket, const QString &localFilePath, const QString &objectPath);

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

			protected:
				void timerEvent(QTimerEvent *event);
				bool checkParameters();
				void closeEvent(QCloseEvent *event);

			private:
				QWidget* centralWidget;
				Ui_mzWatcherGui* guiForm;
				QSet<QString>directoryList;
                        QHash<QString,int>dbFiles;
                        QHash<QString,int>fileList;


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


                        //remote database connection
                        int connectionId;
                        QNetworkAccessManager http;
                        void remoteLogMessage(QString infotype, QString filename, int fileSize, QString msgText);
                        //void readRemoteData(const QHttpResponseHeader &resp);

};



#endif
