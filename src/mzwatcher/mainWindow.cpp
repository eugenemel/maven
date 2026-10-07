#include "mainWindow.h"

const QString MainWindow::kLegacyExtensionSettingsKey = QString("extention");

MainWindow::MainWindow(QWidget* parent):QMainWindow(parent) {

        //initialize
        timerId=0;


        readSettings();

	extension = settings->value("extension").toString();
	sourceFolder = settings->value("sourceFolder").toString();
	destFolder   = settings->value("destFolder").toString();
	convertCommand = settings->value("convertCommand").toString();
	maxDayDiff=settings->value("maxDayDiff").toInt();
	gcsKeyFile = settings->value("gcs_key_file").toString();
	targetBucket = settings->value("target_bucket").toString();

	warningComputerName = settings->value("warningComputerName").toString();
	if (warningComputerNameWasUnset) {
	    // First time this setting has ever existed -- pre-fill the widget
	    // (not the persisted default, which stays empty) with a sensible
	    // guess. The user may still clear it; once writeSettings() runs
	    // once, "contains" will be true forever after, so this only ever
	    // fires on a brand-new settings file.
	    warningComputerName = QSysInfo::machineHostName();
	}
	warningEmailAddresses = settings->value("warningEmailAddresses").toString();
	mailerConfigFile = settings->value("mailerConfigFile").toString();

	centralWidget = new QWidget(parent);
	guiForm = new Ui_mzWatcherGui();
	guiForm->setupUi(centralWidget);
	setCentralWidget(centralWidget);
	centralWidget->setVisible(true);

	guiForm->sourceFolderEdit->setText(sourceFolder);
	guiForm->destFolderEdit->setText(destFolder);
	guiForm->commandEdit->setText(convertCommand);
        guiForm->extensionEdit->setText(extension);
        guiForm->progressBar->hide();
        guiForm->remoteLoging->setChecked( settings->value("remoteLoging").toBool() );
        guiForm->remoteServerUrl->setText(settings->value("remoteServerUrl").toString());
        guiForm->instrumentId->setText(settings->value("instrumentId").toString());
        guiForm->monitorTimeout->setValue(settings->value("monitorTimeout").toInt());
        guiForm->dayDiffBox->setValue(maxDayDiff);
        guiForm->settingsLocationEdit->setText("\"" + settings->fileName() + "\"");
        guiForm->gcsKeyFileEdit->setText(gcsKeyFile);
        guiForm->targetBucketEdit->setText(targetBucket);

        guiForm->automaticWarningsCheckBox->setChecked(settings->value("automaticWarningsEnabled").toBool());
        guiForm->warningComputerNameEdit->setText(warningComputerName);
        guiForm->warningEmailAddressesEdit->setText(warningEmailAddresses);
        guiForm->mailerConfigFileEdit->setText(mailerConfigFile);

        {
            int ruleCount = settings->beginReadArray("warningSizeRules");
            for (int i = 0; i < ruleCount; ++i) {
                settings->setArrayIndex(i);
                QString regex = settings->value("regex").toString();
                int threshold = settings->value("threshold", 70).toInt();
                QString unit = settings->value("unit", "kB").toString();
                addWarningSizeRule(regex, threshold, unit);
            }
            settings->endArray();
        }

        guiForm->watchButton->setCheckable(true);
        guiForm->watchButton->setChecked(settings->value("watchButtonState").toBool());


        connect(guiForm->refreshButton,SIGNAL(pressed()),this,SLOT(updateFileList()));
        connect(guiForm->convertButton,SIGNAL(pressed()),this,SLOT(processSelectedFiles()));
        connect(guiForm->watchButton,SIGNAL(toggled(bool)),this,SLOT(monitor()));

        connect(guiForm->sourceFolderEdit,SIGNAL(textEdited(QString)),this,SLOT(getFormValues()));
        connect(guiForm->destFolderEdit,SIGNAL(textEdited(QString)),this,SLOT(getFormValues()));
        connect(guiForm->commandEdit,SIGNAL(textEdited(QString)),this,SLOT(getFormValues()));
	connect(guiForm->extensionEdit,SIGNAL(textEdited(QString)),this,SLOT(getFormValues()));
	connect(guiForm->dayDiffBox,SIGNAL(valueChanged(int)),this,SLOT(getFormValues()));
	connect(guiForm->destFolderButton,SIGNAL(pressed()),this,SLOT(selectDestFolder()));
	connect(guiForm->sourceFolderButton,SIGNAL(pressed()),this,SLOT(selectSourceFolder()));
        connect(guiForm->clearDatabaseButton,SIGNAL(pressed()),this,SLOT(clearTables()));
        connect(guiForm->gcsKeyFileButton,SIGNAL(pressed()),this,SLOT(selectGcsKeyFile()));
        connect(guiForm->targetBucketEdit,SIGNAL(textEdited(QString)),this,SLOT(getFormValues()));

        connect(guiForm->automaticWarningsCheckBox,SIGNAL(toggled(bool)),this,SLOT(getFormValues()));
        connect(guiForm->warningComputerNameEdit,SIGNAL(textEdited(QString)),this,SLOT(getFormValues()));
        connect(guiForm->warningEmailAddressesEdit,SIGNAL(textEdited(QString)),this,SLOT(getFormValues()));
        connect(guiForm->addWarningSizeRuleButton,SIGNAL(pressed()),this,SLOT(addWarningSizeRuleClicked()));
        connect(guiForm->mailerConfigFileButton,SIGNAL(pressed()),this,SLOT(selectMailerConfigFile()));
        connect(guiForm->mailerConfigFileEdit,SIGNAL(textEdited(QString)),this,SLOT(getFormValues()));

	converter = new BackgroundThread(this);
	connect(converter,SIGNAL(statusChanged(QString)),this,SLOT(setStatus(QString)));

	gcsUploader = new BackgroundThread(this);
	connect(gcsUploader,SIGNAL(statusChanged(QString)),this,SLOT(setStatus(QString)));

	mailer = new BackgroundThread(this);
	connect(mailer,SIGNAL(statusChanged(QString)),this,SLOT(setStatus(QString)));

	updateMailerConfigStatus();


        QString dbDir = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
        QString dbname = dbDir + "/mzWatcher.db";

        setStatus("Using Database: \"" + dbname + "\"");
        // QSqlDatabase/SQLite will not create a missing parent directory on
        // its own -- DB.open() fails silently if dbDir doesn't exist yet
        // (e.g. the first run after an applicationName/organizationName
        // change moves this path), and every later query then fails with a
        // generic "Unable to fetch row" rather than a specific error, since
        // there's no open connection to even check the schema against.
        QDir().mkpath(dbDir);
        DB = QSqlDatabase::addDatabase("QSQLITE", dbname);
        DB.setDatabaseName(dbname);
        if (! DB.open()) {
            setStatus("Failed to open database \"" + dbname + "\": " + DB.lastError().text());
        }
        createTables();
       //clearTables();

       /*
	DB = QSqlDatabase::addDatabase("QODBC");
	DB.setDatabaseName("MetabolomicsDB");

	if (DB.open()) {
		setStatus("Connected to Metabolomics DB!");
	} else {
		setStatus("Failed to connect to MetabolomicsDB");
	}
        */

        guiForm->tabWidget->setCurrentIndex(0);

        showDataFilesTable();
        getFormValues();

        //remoteLogMessage("service", "", 0 , "mzWatcher started");
        if(guiForm->watchButton->isChecked()) monitor();
}

void MainWindow::setStatus(QString status) {

        // "h:m:s" has no leading zeros, so e.g. 3 seconds past the minute
        // prints as "3" instead of "03" -- indistinguishable from "30" at a
        // glance. Zero-pad every field so the timestamp is unambiguous.
        QString nowTime = QTime::currentTime().toString("hh:mm:ss ap");
	//guiForm->statusLabel->setText(status);
        guiForm->logWidget->append(nowTime + ": " + status);
}

bool MainWindow::checkParameters() {
	QDir s(sourceFolder);

	bool ok=true;
	if (!s.exists()) { setStatus("Source Path doesn't exists"); return 0;
		guiForm->sourceFolderEdit->setFrame(true);
		ok=false;
	}

	QDir d(destFolder);
	if (!d.exists()) { setStatus("Destination Path doesn't exists"); return 0;
		guiForm->sourceFolderEdit->setFrame(true);
		ok=false;
	}


	return ok;
}

void MainWindow::stop_startConversion() {

	if (converter->isRunning()) {
		stopConversion();
	} else{
		startConversion();
	}
}

void MainWindow::selectDestFolder() {
	QString dir =QFileDialog::getExistingDirectory(this,".");
	destFolder = dir+ "/";
	guiForm->destFolderEdit->setText(dir);
}

void MainWindow::selectSourceFolder() {
	QString dir = QFileDialog::getExistingDirectory(this,".");
	sourceFolder = dir + "/";
	guiForm->sourceFolderEdit->setText(dir);
}

void MainWindow::selectGcsKeyFile() {
	QString file = QFileDialog::getOpenFileName(this, "Select GCS Service Account Key File", ".", "JSON Files (*.json)");
	if (file.isEmpty()) return;
	gcsKeyFile = file;
	guiForm->gcsKeyFileEdit->setText(gcsKeyFile);
}

void MainWindow::stopConversion() {
	if (converter->isRunning()) {
		converter->killProcess();
	}

        int selectedItemsCount = guiForm->treeWidget->selectedItems().size();
}


void MainWindow::startConversion() {
	if (converter->isRunning()) {
		stopConversion();
		return;
	}

        guiForm->convertButton->setText("Stop");

	processSelectedFiles();
	updateFileList();
        guiForm->convertButton->setText("Convert");
}

void MainWindow::processSelectedFiles() {
	QStringList selectedfiles;

	foreach(QTreeWidgetItem* item, guiForm->treeWidget->selectedItems() ) {
                selectedfiles << item->text(2);
	}

        int count=0;
        guiForm->progressBar->show();
        guiForm->progressBar->setMaximum(selectedfiles.size());
	foreach (QString filename, selectedfiles) {
                guiForm->progressBar->setValue(++count);
                processFile(filename);

	}
        guiForm->treeWidget->clearSelection();
        guiForm->progressBar->hide();
}

void MainWindow::monitor() {

        int timeoutMin = guiForm->monitorTimeout->value();
        setStatus("TimeOut=" + QString::number(timeoutMin));
        setStatus("watchButtonState=" + QString::number(guiForm->watchButton->isChecked()));
        setStatus("timeOut=" + QString::number(timeoutMin*1000));


        if (guiForm->watchButton->isChecked() && timeoutMin > 0) {
            if(timerId) this->killTimer(timerId);
            timerId = this->startTimer(timeoutMin*1000); // time in msec
            setStatus("Starting monitoring service." + QString::number(timerId));
        } else if (timerId!=0) {
            setStatus("Killing monitoring service." + QString::number(timerId));

            this->killTimer(timerId);
            timerId=0;
        }

       updateButtonColors();
}

void MainWindow::getFormValues() {
        //setStatus("Updating Form Values");
	sourceFolder = guiForm->sourceFolderEdit->text();
	destFolder = guiForm->destFolderEdit->text();
	convertCommand=guiForm->commandEdit->text();
	extension = guiForm->extensionEdit->text();
	maxDayDiff = guiForm->dayDiffBox->value();
	gcsKeyFile = guiForm->gcsKeyFileEdit->text();
	targetBucket = guiForm->targetBucketEdit->text();

	warningComputerName = guiForm->warningComputerNameEdit->text();
	warningEmailAddresses = guiForm->warningEmailAddressesEdit->text();
	QString newMailerConfigFile = guiForm->mailerConfigFileEdit->text();
	if (newMailerConfigFile != mailerConfigFile) {
	    mailerConfigFile = newMailerConfigFile;
	    importMailerConfigFileValues();
	}

        //convert windows backslash to unix forward slash
	sourceFolder = sourceFolder.replace("\\","\057"); // not "\/"
        destFolder = destFolder.replace("\\","\057");
	// didn't just do "/" as it messed up the Mac Emacs C++ mode


}

void MainWindow::updateFileList() {
        //setStatus("Updating file list");
	directoryList.clear();
        guiForm->monitorDial->setStyleSheet("background: yellow;");
        guiForm->monitorDial->setToolTip("Scanning the Source Folder right now.");
	setCursor(Qt::WaitCursor);
        getDBFileList();
        getFileList(sourceFolder);
	showDataFilesTable();
        processChangedFiles();
	setCursor(Qt::ArrowCursor);
        updateButtonColors();

}

void MainWindow::updateButtonColors() {

    if (timerId) {
        guiForm->monitorDial->setStyleSheet("background: green;");
        guiForm->monitorDial->setToolTip("Watch Folder is ON: mzWatcher is actively monitoring the Source Folder and will automatically convert files once they stop changing.");
    } else {
        guiForm->monitorDial->setStyleSheet("background: red;");
        guiForm->monitorDial->setToolTip("Watch Folder is OFF: mzWatcher is not monitoring the Source Folder. Nothing will be converted automatically until you click Watch Folder. This is not an error -- it just means automatic monitoring is currently disabled.");
    }
}

void MainWindow::processChangedFiles() {
    if(!guiForm->watchButton->isChecked()) return;

    int minFileSize = guiForm->minimumsFileSize->value();
    int waitTime    = guiForm->converter_waitTime->value()*60;
    QDateTime now = QDateTime::currentDateTime();
    int oneday = 3600*24; // if file is too old.. don't autoconvert

    foreach(QString file, dbFiles.keys()) {

        if ( dbFiles[file] != fileList[file]) {
            QFileInfo fi(file);
            qint64 currentSize = effectiveFileSize(fi);

            int ageSec = now.secsTo(fi.lastModified())*-1;

            if (ageSec < oneday && ageSec > waitTime and currentSize > minFileSize && currentSize != dbFiles[file]) {
                setStatus(tr("Processing changed file: \"%1\" ").arg(file));
                processFile(file);
            }
        } else if (!convertedFiles.contains(file)) {
            // "Never detected" and "detected, unchanging size" are different
            // states: dbFiles[file] == fileList[file] here doesn't mean
            // nothing is happening, it can also mean this file/.d bundle was
            // already completely written the very first time mzWatcher's
            // scan ever found it (size at insertFileInfo() time already
            // equals its current size) -- the common case for an Agilent .d
            // bundle, which is usually written in a burst rather than
            // steadily appended to. The branch above would never catch this,
            // since it requires having OBSERVED growth. Trigger here once
            // enough time has passed since WE first saw the file, instead of
            // relying on the filesystem's lastModified() (unreliable for a
            // directory -- see firstDetectedTimes in mainWindow.h). Safe from
            // converting something still mid-write: if the file were still
            // growing, fileList[file] (this scan) would already differ from
            // dbFiles[file] (first-detection size) and this branch wouldn't
            // run at all.
            int secsSinceFirstDetected = firstDetectedTimes.value(file).secsTo(now);
            if (secsSinceFirstDetected > waitTime && secsSinceFirstDetected < oneday) {
                QFileInfo fi(file);
                qint64 currentSize = effectiveFileSize(fi);
                if (currentSize > minFileSize) {
                    setStatus(tr("Processing unchanged file: \"%1\" ").arg(file));
                    processFile(file);
                }
            }
        }
    }

}


void MainWindow::timerEvent(QTimerEvent* event) {
       getFormValues();
       if(checkParameters()) updateFileList();
}


QString MainWindow::createTempPath() {
	//temporary path
	QString tempPathName = QDir::tempPath() + "/tmp" +  QString::number(qrand());
	QDir tempPath(tempPathName);
	if (! tempPath.exists()) { tempPath.mkpath(tempPathName); tempPath=QDir(tempPathName); }
	return tempPathName;
}

void MainWindow::convertFile(QString file) {
	if (checkParameters() == 0) return;
	QFileInfo sourceFileInfo(file);
	QString   sourceFileName = sourceFileInfo.fileName();
	QString   destFileFormat = guiForm->destFileFormatBox->currentText();

	if(! sourceFileInfo.exists()) return;
        QString tempPathName = createTempPath();
        QString tempConvertedFile = tempPathName + "/" + sourceFileName;
        tempConvertedFile.replace(extension,destFileFormat);

	setStatus("TempFile=\"" + tempConvertedFile + "\"");
	QDir tempPath(tempPathName);

	if (tempPath.exists()) {
		QString command = convertCommand.arg(file,tempConvertedFile);
		converter->setSystemCommand(command);
		converter->start();

		while(converter->isRunning()){
                        guiForm->convertButton->setText("Stop");
			QApplication::processEvents();
		}
		markFileConverted(file);
	}
}

bool MainWindow::isConvertibleMatch(const QString &entryName, const QString &extension) {
    if (extension.isEmpty()) return false;
    return entryName.endsWith(extension, Qt::CaseInsensitive);
}

bool MainWindow::copyRecursively(const QString &sourcePath, const QString &destPath) {
    QFileInfo sourceInfo(sourcePath);
    if (!sourceInfo.exists()) return false;

    if (sourceInfo.isDir()) {
        QDir destDir(destPath);
        if (!destDir.exists() && !destDir.mkpath(destPath)) return false;

        QDir sourceDir(sourcePath);
        const QFileInfoList entries = sourceDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        foreach (const QFileInfo &entry, entries) {
            const QString childDestPath = destPath + "/" + entry.fileName();
            if (!copyRecursively(entry.absoluteFilePath(), childDestPath)) return false;
        }
        return true;
    }

    if (QFile::exists(destPath)) QFile::remove(destPath);
    return QFile::copy(sourcePath, destPath);
}

QString MainWindow::buildGcsObjectPath(const QString &destFolder, const QString &convertedFilePath) {
    QString normalizedDestFolder = destFolder;
    if (!normalizedDestFolder.isEmpty() && !normalizedDestFolder.endsWith("/")) {
        normalizedDestFolder += "/";
    }

    QString relativePath = convertedFilePath;
    if (!normalizedDestFolder.isEmpty() && relativePath.startsWith(normalizedDestFolder)) {
        relativePath = relativePath.mid(normalizedDestFolder.length());
    }
    while (relativePath.startsWith("/")) {
        relativePath.remove(0, 1);
    }
    return relativePath;
}

QStringList MainWindow::buildGcsUploadCommand(const QString &gcsKeyFile, const QString &targetBucket, const QString &localFilePath, const QString &objectPath) {
    if (gcsKeyFile.isEmpty() || targetBucket.isEmpty()) return QStringList();

    QString destUrl = "gs://" + targetBucket + "/" + objectPath;
    return QStringList()
        << "gsutil"
        << "-o" << QString("Credentials:gs_service_key_file=%1").arg(gcsKeyFile)
        << "cp" << localFilePath << destUrl;
}

void MainWindow::uploadConvertedFileToGcs(const QString &localConvertedFile) {
    if (gcsKeyFile.isEmpty() || targetBucket.isEmpty()) return; // R11: no-op when unset

    QString objectPath = buildGcsObjectPath(destFolder, localConvertedFile);
    QStringList command = buildGcsUploadCommand(gcsKeyFile, targetBucket, localConvertedFile, objectPath);
    if (command.isEmpty()) return;

    setStatus("Uploading to \"gs://" + targetBucket + "/" + objectPath + "\"");
    gcsUploader->setSystemCommand(command.first(), command.mid(1));
    gcsUploader->start();

    while (gcsUploader->isRunning()) {
        QApplication::processEvents();
    }
}

qint64 MainWindow::totalDirectorySize(const QString &dirPath) {
    QFileInfo info(dirPath);
    if (!info.exists()) return 0;
    if (!info.isDir()) return info.size();

    qint64 total = 0;
    QDir dir(dirPath);
    const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (const QFileInfo &entry, entries) {
        if (entry.isDir()) {
            total += totalDirectorySize(entry.absoluteFilePath());
        } else {
            total += entry.size();
        }
    }
    return total;
}

qint64 MainWindow::effectiveFileSize(const QFileInfo &fi) {
    return fi.isDir() ? totalDirectorySize(fi.absoluteFilePath()) : fi.size();
}

qint64 MainWindow::thresholdInBytes(double threshold, const QString &unit) {
    // Decimal/SI units (kB=1000, MB=1,000,000, GB=1,000,000,000), not
    // binary KiB/MiB/GiB -- matches what end users expect from these labels.
    if (unit.compare("GB", Qt::CaseInsensitive) == 0) return (qint64)(threshold * 1000000000.0);
    if (unit.compare("MB", Qt::CaseInsensitive) == 0) return (qint64)(threshold * 1000000.0);
    return (qint64)(threshold * 1000.0); // kB (also the fallback for anything unrecognized)
}

int MainWindow::findMatchingSizeWarningRule(const QVector<SizeWarningRule> &rules, const QString &fileName) {
    for (int i = 0; i < rules.size(); ++i) {
        const QString &pattern = rules.at(i).regex;
        if (pattern.isEmpty()) continue; // a blank regex never matches, rather than matching everything
        QRegularExpression re(pattern, QRegularExpression::CaseInsensitiveOption);
        if (!re.isValid()) continue; // an unparsable regex is ignored, not a hard error
        if (re.match(fileName).hasMatch()) return i;
    }
    return -1;
}

QStringList MainWindow::parseEmailRecipients(const QString &commaSeparated) {
    QStringList result;
    const QStringList parts = commaSeparated.split(",");
    foreach (const QString &part, parts) {
        QString trimmed = part.trimmed();
        if (!trimmed.isEmpty()) result << trimmed;
    }
    return result;
}

QString MainWindow::buildWarningEmailSubject(const QString &fileName) {
    return "mzWatcher warning: " + fileName;
}

QString MainWindow::buildWarningEmailBody(const QString &fileName, double fileSize, double threshold, const QString &unit, const QString &computerName) {
    QString text = QString("WARNING: Conversion of file %1 has size %2 %3, which is below warning level of %4 %3.")
        .arg(fileName)
        .arg(fileSize)
        .arg(unit)
        .arg(threshold);
    if (!computerName.isEmpty()) {
        text += QString("\n\nThis Warning was delivered from the computer named '%1'.").arg(computerName);
    }
    return text;
}

QHash<QString,QString> MainWindow::parseMailerConfigFile(const QString &filePath) {
    QHash<QString,QString> result;
    if (filePath.isEmpty()) return result;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return result;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("#")) continue;

        int eq = line.indexOf('=');
        if (eq <= 0) continue; // not a KEY=VALUE line -- ignore rather than fail the whole file

        QString key = line.left(eq).trimmed();
        QString value = line.mid(eq + 1).trimmed();
        if (!key.isEmpty()) result[key] = value;
    }
    file.close();
    return result;
}

QVector<MainWindow::SizeWarningRule> MainWindow::collectSizeWarningRules() const {
    QVector<SizeWarningRule> rules;
    rules.reserve(warningSizeRuleWidgets.size());
    foreach (const SizeWarningRuleWidgets &w, warningSizeRuleWidgets) {
        SizeWarningRule rule;
        rule.regex = w.regexEdit->text();
        rule.threshold = w.thresholdSpinBox->value();
        rule.unit = w.unitBox->currentText();
        rules.append(rule);
    }
    return rules;
}

void MainWindow::checkAndSendSizeWarning(const QFileInfo &sourceFileInfo) {
    // Both settings must be set, mirroring the GCS upload gating pattern (R11).
    if (!guiForm->automaticWarningsCheckBox->isChecked()) return;

    QHash<QString,QString> config = parseMailerConfigFile(mailerConfigFile);
    if (config.value("EMAIL_ADDRESS").isEmpty() || config.value("EMAIL_PASSWORD").isEmpty()) return;

    // Rules are an allowlist, evaluated top-down: a file that matches no
    // rule's regex gets no size check at all, not a fallback threshold.
    QVector<SizeWarningRule> rules = collectSizeWarningRules();
    int matchIndex = findMatchingSizeWarningRule(rules, sourceFileInfo.fileName());
    if (matchIndex < 0) return;
    const SizeWarningRule &rule = rules.at(matchIndex);

    qint64 sourceSizeBytes = effectiveFileSize(sourceFileInfo);
    qint64 thresholdBytes = thresholdInBytes(rule.threshold, rule.unit);

    if (sourceSizeBytes >= thresholdBytes) return;

    // Recipients are unrestricted -- any address the user configures, not
    // limited to any particular domain.
    QStringList recipients = parseEmailRecipients(warningEmailAddresses);
    if (recipients.isEmpty()) {
        setStatus("Skipping size-warning email for \"" + sourceFileInfo.fileName() + "\": no recipients configured");
        return;
    }

    double unitDivisor = 1000.0;
    if (rule.unit.compare("MB", Qt::CaseInsensitive) == 0) unitDivisor = 1000000.0;
    else if (rule.unit.compare("GB", Qt::CaseInsensitive) == 0) unitDivisor = 1000000000.0;

    double fileSizeInUnit = (double)sourceSizeBytes / unitDivisor;

    QString subject = buildWarningEmailSubject(sourceFileInfo.fileName());
    QString body = buildWarningEmailBody(sourceFileInfo.fileName(), fileSizeInUnit, (double)rule.threshold, rule.unit, warningComputerName);

    // Logged unconditionally, independent of whether sendWarningEmail() can
    // actually deliver it (e.g. non-Windows, where it can't) -- so the
    // warning is always visible in the log even when no email goes out.
    setStatus("SIZE WARNING: " + subject + " -- " + body);

    setStatus("Sending size-warning email for \"" + sourceFileInfo.fileName() + "\" to \"" + recipients.join(", ") + "\"");
    sendWarningEmail(recipients, subject, body);
}

void MainWindow::sendWarningEmail(const QStringList &recipients, const QString &subject, const QString &body) {
#ifndef Q_OS_WIN
    Q_UNUSED(recipients);
    Q_UNUSED(subject);
    Q_UNUSED(body);
    // Email delivery is implemented via PowerShell's Send-MailMessage (see
    // the #else branch below), which only exists on Windows. Rather than
    // silently doing nothing on macOS/Linux, say so explicitly -- the
    // warning itself was already logged unconditionally by the caller.
    setStatus("Email not available on this platform (Windows only) -- warning was not emailed.");
    return;
#else
    QHash<QString,QString> config = parseMailerConfigFile(mailerConfigFile);
    QString emailAddress = config.value("EMAIL_ADDRESS");
    QString emailPassword = config.value("EMAIL_PASSWORD");
    // Optional, defaulting to Gmail's: this file can point at any SMTP
    // provider, it just assumes Gmail's when the user doesn't say otherwise.
    QString smtpServer = config.value("SMTP_SERVER", "smtp.gmail.com");
    QString smtpPort = config.value("SMTP_PORT", "587");

    if (emailAddress.isEmpty() || emailPassword.isEmpty()) {
        setStatus("Cannot send size-warning email: mailer config file \"" + mailerConfigFile + "\" is missing EMAIL_ADDRESS or EMAIL_PASSWORD");
        return;
    }

    // A fixed, static script: every dynamic value (recipients, subject, body
    // -- any of which may contain arbitrary user-entered text) is passed
    // through environment variables PowerShell reads as plain strings, never
    // interpolated into the script's own text, so there's no shell-injection
    // vector here. The password travels the same way specifically so it
    // never appears on this (or any) process's own command line, which is
    // visible to any other user on the machine via Task Manager/tasklist
    // without special privileges -- an environment variable scoped to this
    // one child process is a meaningfully better, if not perfect, place for
    // a short-lived secret.
    QString scriptContents =
        "$ErrorActionPreference = \"Stop\"\n"
        "try {\n"
        "    $to = $env:MZWATCHER_MAIL_TO -split \",\"\n"
        "    $securePwd = ConvertTo-SecureString $env:MZWATCHER_MAIL_PASSWORD -AsPlainText -Force\n"
        "    $cred = New-Object System.Management.Automation.PSCredential($env:MZWATCHER_MAIL_FROM, $securePwd)\n"
        "    Send-MailMessage -To $to -From $env:MZWATCHER_MAIL_FROM -Subject $env:MZWATCHER_MAIL_SUBJECT -Body $env:MZWATCHER_MAIL_BODY -SmtpServer $env:MZWATCHER_MAIL_SERVER -Port ([int]$env:MZWATCHER_MAIL_PORT) -UseSsl -Credential $cred\n"
        "} catch {\n"
        "    Write-Error $_.Exception.Message\n"
        "    exit 1\n"
        "}\n";

    QString scriptPath = QDir::tempPath() + "/mzwatcher_send_mail.ps1";
    QFile scriptFile(scriptPath);
    if (!scriptFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        setStatus("Cannot send size-warning email: failed to write temporary mailer script \"" + scriptPath + "\"");
        return;
    }
    {
        QTextStream out(&scriptFile);
        out << scriptContents;
    }
    scriptFile.close();

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("MZWATCHER_MAIL_TO", recipients.join(","));
    env.insert("MZWATCHER_MAIL_FROM", emailAddress);
    env.insert("MZWATCHER_MAIL_PASSWORD", emailPassword);
    env.insert("MZWATCHER_MAIL_SUBJECT", subject);
    env.insert("MZWATCHER_MAIL_BODY", body);
    env.insert("MZWATCHER_MAIL_SERVER", smtpServer);
    env.insert("MZWATCHER_MAIL_PORT", smtpPort);

    mailer->setProcessEnvironment(env);
    mailer->setSystemCommand("powershell.exe", QStringList() << "-ExecutionPolicy" << "Bypass" << "-NoProfile" << "-File" << scriptPath);
    mailer->start();
    // Fire-and-forget: BackgroundThread runs this on its own QThread, so this
    // never blocks the conversion pipeline, and any failure just reaches the
    // log via the existing statusChanged -> setStatus connection -- same
    // resilience principle as the GCS upload feature (R12).
#endif
}

void MainWindow::updateMailerConfigStatus() {
    if (mailerConfigFile.isEmpty()) {
        guiForm->mailerConfigStatusLabel->setText("Mail service not configured");
        return;
    }
    QHash<QString,QString> config = parseMailerConfigFile(mailerConfigFile);

    if (config.value("EMAIL_ADDRESS").isEmpty() || config.value("EMAIL_PASSWORD").isEmpty()) {
        guiForm->mailerConfigStatusLabel->setText("Mail service not configured (file is missing EMAIL_ADDRESS or EMAIL_PASSWORD)");
    } else {
        guiForm->mailerConfigStatusLabel->setText("Mail service configured (" + config.value("EMAIL_ADDRESS") + ")");
    }
}

void MainWindow::importMailerConfigFileValues() {
    QHash<QString,QString> config = parseMailerConfigFile(mailerConfigFile);

    // A one-time value transfer, not a persistent sync: EMAIL_RECIPIENTS (if
    // present) is copied into warningEmailAddresses/the Recipients field
    // right now, at the moment this file is selected. From here on it's an
    // ordinary GUI-backed setting like any other -- the user can freely
    // overwrite it, and whatever they leave there is what gets saved to
    // QSettings on close. It is NOT reapplied just because the app restarts
    // with the same file still selected (see updateMailerConfigStatus()).
    if (!config.value("EMAIL_RECIPIENTS").isEmpty()) {
        warningEmailAddresses = config.value("EMAIL_RECIPIENTS");
        guiForm->warningEmailAddressesEdit->setText(warningEmailAddresses);
    }

    updateMailerConfigStatus();
}

void MainWindow::selectMailerConfigFile() {
    QString file = QFileDialog::getOpenFileName(this, "Select Mailer Config File", ".", "Text Files (*.txt);;All Files (*)");
    if (file.isEmpty()) return;
    mailerConfigFile = file;
    guiForm->mailerConfigFileEdit->setText(mailerConfigFile);
    importMailerConfigFileValues();
}

void MainWindow::addWarningSizeRuleClicked() {
    addWarningSizeRule(QString(), 70, "kB");
}

void MainWindow::addWarningSizeRule(const QString &regex, int threshold, const QString &unit) {
    QWidget *row = new QWidget(guiForm->warningSizeRulesContainer);
    QHBoxLayout *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);

    QLineEdit *regexEdit = new QLineEdit(row);
    regexEdit->setPlaceholderText("Filename regex (e.g. \\.d$)");
    regexEdit->setText(regex);

    QLabel *belowLabel = new QLabel("below:", row);

    QSpinBox *thresholdSpinBox = new QSpinBox(row);
    thresholdSpinBox->setMaximum(1000000000);
    thresholdSpinBox->setValue(threshold);

    QComboBox *unitBox = new QComboBox(row);
    unitBox->addItem("kB");
    unitBox->addItem("MB");
    unitBox->addItem("GB");
    unitBox->setCurrentText(unit);

    QPushButton *deleteButton = new QPushButton("Delete", row);

    rowLayout->addWidget(regexEdit, 1);
    rowLayout->addWidget(belowLabel);
    rowLayout->addWidget(thresholdSpinBox);
    rowLayout->addWidget(unitBox);
    rowLayout->addWidget(deleteButton);

    guiForm->warningSizeRulesContainerLayout->addWidget(row);

    SizeWarningRuleWidgets w;
    w.rowWidget = row;
    w.regexEdit = regexEdit;
    w.thresholdSpinBox = thresholdSpinBox;
    w.unitBox = unitBox;
    warningSizeRuleWidgets.append(w);

    connect(deleteButton, &QPushButton::clicked, this, [this, row]() {
        removeWarningSizeRule(row);
    });
}

void MainWindow::removeWarningSizeRule(QWidget *rowWidget) {
    for (int i = 0; i < warningSizeRuleWidgets.size(); ++i) {
        if (warningSizeRuleWidgets.at(i).rowWidget == rowWidget) {
            warningSizeRuleWidgets.removeAt(i);
            break;
        }
    }
    // Remove from the layout immediately so the container's row count is
    // accurate right away; the widget itself (and the Delete button whose
    // click handler is still on the call stack) is destroyed once control
    // returns to the event loop.
    guiForm->warningSizeRulesContainerLayout->removeWidget(rowWidget);
    rowWidget->deleteLater();
}

void MainWindow::makeBackupCopy(QString file) {

	QFileInfo sourceFileInfo(file);
	QString   sourceFileDir =  sourceFileInfo.absolutePath();
	QString   sourceFileName = sourceFileInfo.fileName();
        setStatus("\n\n\nProcessing \"" + sourceFileName + "\"");

	//source file is no longer available
	if(! sourceFileInfo.exists()) return;

	checkAndSendSizeWarning(sourceFileInfo);

	QString   destFile = file;
	destFile.replace(sourceFolder,destFolder);
        QFileInfo destFileInfo(destFile);
        QString   destFileDir =  destFileInfo.absolutePath();
        QString   destFileName(destFileDir + "/" + sourceFileName);
        QString destFileFormat = guiForm->destFileFormatBox->currentText();
        QString	  convertedFileName = destFile;
        convertedFileName = convertedFileName.replace(extension,destFileFormat);

        setStatus("Source=\""+sourceFolder+"\"");
        setStatus("Dest=\""+destFolder+"\"");
        setStatus("Dest Dir=\"" + destFileDir + "\"");
        setStatus("Source Dir=\"" + sourceFileDir + "\"");
        setStatus("Source File=\"" + sourceFileName + "\"");
        setStatus("Destination File=\"" + destFileName + "\"");


	if (destFile == file) {	//path is identical..
		setStatus("Dest and Source Path are identical.. copy failed");
	}


	//create destination path
	QDir destPath(destFileDir);
	if (! destPath.exists()) {
		bool ok = destPath.mkpath(destFileDir);
		if( !ok ){
			setStatus("Failed to make path=\"" + destFolder + "\"");
			return;
		}
	}

	//copy file if it doesn't exists, size changed, or modification time changed
        bool makeFileCopy = guiForm->makeCopyCheckBox->isChecked();
        if (makeFileCopy) {
            if(! destFileInfo.exists() ||
               destFileInfo.lastModified() != sourceFileInfo.lastModified() ||
               effectiveFileSize(destFileInfo) != effectiveFileSize(sourceFileInfo) )
            {
                setStatus("Making file copy");
                if (sourceFileInfo.isDir()) {
                    copyRecursively(file, destFileName);
                } else {
                    QFile::copy(file, destFileName);
                }
         }}

        //convert file
        if (destPath.exists()) {
                setStatus("Running conversion");

                // QString::replace() mutates in place, so file/convertedFileName/
                // destFileDir below become backslash-mangled Windows-style paths
                // for the converter's command line. Capture the real filesystem
                // path of the converted output, and of the source file itself,
                // *before* that mutation: the former is what needs to be handed
                // to gsutil for the GCS upload, and the latter is what
                // markFileConverted() needs to match the DB's filename column
                // (always forward-slash, per QFileInfo::absoluteFilePath()) --
                // passing it the backslash-mangled form means its UPDATE's WHERE
                // clause never matches any row, so fileConverted (and fileSize)
                // never actually get persisted and the file looks never-converted
                // forever, on every platform, not just Windows.
                QString localConvertedFile = convertedFileName;
                QString sourceFilePath = file;

                QString infile = file.replace("\057", "\\"); // # not "\/"
                QString outfile = convertedFileName.replace("\057","\\");
                QString outputdir = destFileDir.replace("\057","\\");


                if (convertCommand.contains("msconvert",Qt::CaseInsensitive)) {
                        outfile = outputdir;
                }

                QString command = convertCommand.arg(infile,outfile);
                converter->setSystemCommand(command);
                converter->start();

                while(converter->isRunning()){
                        guiForm->convertButton->setText("Stop");
                        QApplication::processEvents();
                }
                guiForm->convertButton->setText("Convert");
                markFileConverted(sourceFilePath);

                uploadConvertedFileToGcs(localConvertedFile);
        }

	//refresh destination file information
	destFileInfo = QFileInfo(destFile);
}

void MainWindow::processFile(QString file) {
	if (checkParameters() == 0) return;

        makeBackupCopy(file);
        //convertFile(file);

}


void MainWindow::getFileList(const QString &fromDir) {
    //	qDebug() << "getFileList() " << fromDir;
    QDir d(fromDir);
    if(!d.exists()) return;	//directory doesn't exists

    //check if directory has already been processed
    QFileInfo dirInfo(fromDir);
    if(directoryList.contains(dirInfo.absoluteFilePath()) == true ) return;
    directoryList.insert(dirInfo.absoluteFilePath());

    QStringList filters;
    QFileInfoList list = d.entryInfoList( filters,
                                          QDir::Files | QDir::Dirs | QDir::NoSymLinks  | QDir::Readable,
                                          QDir::Name | QDir::DirsFirst | QDir::IgnoreCase);
    if (list.size() == 0) return;

    int fileCount=0;
    foreach (QFileInfo fi, list ) {
        fileCount++;
        if(fileCount % 10==0) { QApplication::processEvents(); }
        if (fi.fileName() == "." || fi.fileName() == ".." ) continue;

        if (isConvertibleMatch(fi.fileName(), extension)) {
            // Leaf match: either a plain file or an Agilent .d-style
            // directory bundle whose name ends in the configured extension.
            // Either way this is a convertible unit, not a folder to walk
            // into further (R1).
            QString absfilepath=fi.absoluteFilePath();
            qint64 fsize = effectiveFileSize(fi);

            fileList[absfilepath]=fsize;
            QDateTime now = QDateTime::currentDateTime();

            if((now.daysTo(fi.lastModified()))*-1 < maxDayDiff ){
                if(!fileList.contains(absfilepath) || !dbFiles.contains(absfilepath)) {
                    insertFileInfo(absfilepath);
                    //remoteLogMessage("newfile", absfilepath, fsize,"New File Detected");
                }
            }
        } else if (fi.isDir() && ! directoryList.contains(fi.absoluteFilePath()) ) {
            //qDebug() << "Dir=" << fi.absoluteFilePath();
            getFileList(fi.absoluteFilePath()); //recurse
        }
    }
}

void MainWindow::readSettings() {
          settings = new QSettings("mzWatch", "mzWatch Settings");

         if (! settings || settings->status() != QSettings::NoError ) {
             return;
         }

	 QPoint pos = settings->value("pos", QPoint(200, 200)).toPoint();
	 QSize size = settings->value("size", QSize(1000, 400)).toSize();
	 resize(size);
	 move(pos);

	 if( ! settings->contains("extension") ) {
	     if( settings->contains(kLegacyExtensionSettingsKey) ) {
	         // One-time migration: carry an existing user's saved value
	         // forward under the corrected key name instead of resetting
	         // it to the default (R4).
	         settings->setValue("extension", settings->value(kLegacyExtensionSettingsKey));
	         settings->remove(kLegacyExtensionSettingsKey);
	     } else {
	         settings->setValue("extension",QString("wiff"));
	     }
	 }

	 if( ! settings->contains("sourceFolder") )
			 settings->setValue("sourceFolder",QString("C:/Analyst Data/Projects/Metabolomics/"));

	 if( ! settings->contains("destFolder") )
			 settings->setValue("destFolder",QString("Y:/Metabolomics/Data/"));

	 // Issue 858: "if not contains" alone only ever seeds a brand-new
	 // settings file -- anyone who already launched an earlier build has
	 // convertCommand already persisted with the old factory default, so
	 // changing the string literal here has no effect for them. Migrate
	 // forward only when the saved value still matches the *old* factory
	 // default exactly, so a deliberately customized command is left alone.
	 static const QString kOldDefaultConvertCommand = QString("mzWiff.exe --mzXML \"%1\" \"%2\" ");
	 static const QString kDefaultConvertCommand = QString("\"msconvert.exe\" --32 --filter \"peakPicking true [1,2]\" --mzML \"%1\" -o \"%2\"");
	 if( ! settings->contains("convertCommand") ) {
			 settings->setValue("convertCommand", kDefaultConvertCommand);
	 } else if( settings->value("convertCommand").toString() == kOldDefaultConvertCommand ) {
			 settings->setValue("convertCommand", kDefaultConvertCommand);
	 }

         if( ! settings->contains("monitorTimeout") )
                         settings->setValue("monitorTimeout", 60);


	 if( ! settings->contains("maxDayDiff") )
			 settings->setValue("maxDayDiff", 5);

         if( ! settings->contains("watchButtonState") )
                         settings->setValue("watchButtonState", false);

         if( ! settings->contains("gcs_key_file") )
                         settings->setValue("gcs_key_file", QString(""));

         if( ! settings->contains("target_bucket") )
                         settings->setValue("target_bucket", QString(""));

         if( ! settings->contains("automaticWarningsEnabled") )
                         settings->setValue("automaticWarningsEnabled", true);

         // See the warningComputerNameWasUnset comment at its declaration --
         // this flag must be captured *before* the default is seeded below,
         // since afterward "contains" is true forever and the distinction
         // between "never set" and "set to empty" would be lost.
         warningComputerNameWasUnset = ! settings->contains("warningComputerName");
         if( warningComputerNameWasUnset )
                         settings->setValue("warningComputerName", QString(""));

         if( ! settings->contains("warningEmailAddresses") )
                         settings->setValue("warningEmailAddresses", QString(""));

         if( ! settings->contains("mailerConfigFile") )
                         settings->setValue("mailerConfigFile", QString(""));


 }

 void MainWindow::writeSettings() {
	 settings->setValue("pos", pos());
	 settings->setValue("size", size());
	 settings->setValue("geometry", saveGeometry());
	 settings->setValue("extension",extension);
	 settings->setValue("sourceFolder", sourceFolder);
	 settings->setValue("destFolder", destFolder);
	 settings->setValue("convertCommand", convertCommand);
	 settings->setValue("maxDayDiff", maxDayDiff);
         settings->setValue("remoteServerUrl", guiForm->remoteServerUrl->text());
         settings->setValue("instrumentId", guiForm->instrumentId->text());
         settings->setValue("remoteLoging", guiForm->remoteLoging->isChecked());
         settings->setValue("monitorTimeout", guiForm->monitorTimeout->value());
         settings->setValue("watchButtonState", guiForm->watchButton->isChecked());
         settings->setValue("gcs_key_file", gcsKeyFile);
         settings->setValue("target_bucket", targetBucket);

         settings->setValue("automaticWarningsEnabled", guiForm->automaticWarningsCheckBox->isChecked());
         settings->setValue("warningComputerName", warningComputerName);
         settings->setValue("warningEmailAddresses", warningEmailAddresses);
         settings->setValue("mailerConfigFile", mailerConfigFile);

         settings->beginWriteArray("warningSizeRules");
         for (int i = 0; i < warningSizeRuleWidgets.size(); ++i) {
             settings->setArrayIndex(i);
             settings->setValue("regex", warningSizeRuleWidgets[i].regexEdit->text());
             settings->setValue("threshold", warningSizeRuleWidgets[i].thresholdSpinBox->value());
             settings->setValue("unit", warningSizeRuleWidgets[i].unitBox->currentText());
         }
         settings->endArray();

	 qDebug() << "Settings saved to " << settings->fileName();
 }

 void MainWindow::clearTables() {
     QSqlQuery query(DB);
     bool ok = query.exec("delete from datafiles");
     if(!ok) setStatus(query.lastError().text());
     query.clear();

     dbFiles.clear();
     fileList.clear();
     directoryList.clear();
     firstDetectedTimes.clear();
     convertedFiles.clear();
     showDataFilesTable();
 }

 void MainWindow::closeEvent(QCloseEvent *event) {
        //remoteLogMessage("service", "", 0 , "mzWatcher stopped");
         writeSettings();
	  DB.close();
	  event->accept();
 }


 void MainWindow::createTables() {
   setStatus("Creating Tables");
   QSqlQuery query(DB);

   // Issue 858: QStandardPaths::DataLocation (and therefore the path to
   // mzWatcher.db) is derived from QCoreApplication::applicationName(),
   // which defaults to the built executable's own filename when not set
   // explicitly. Because this program's target name has changed across
   // versions, a datafiles table created by an older/differently-built copy
   // of this program can already exist on disk with a column layout this
   // build doesn't expect. "create table if not exists" silently does
   // nothing against such a table, which then makes every later prepared
   // statement fail ("Parameter count mismatch", "Unable to fetch row").
   // Detect that mismatch here and rebuild the table rather than assume
   // "if not exists" is sufficient. This table only caches which files have
   // already been scanned/converted -- not the converted files themselves --
   // so rebuilding it is safe; a Refresh repopulates it.
   static const QStringList kExpectedColumns = {"filename", "fileConverted", "fileAnalyzed", "fileSize", "modTime", "firstDetected"};

   QSqlQuery schemaCheck(DB);
   schemaCheck.exec("pragma table_info(datafiles)");
   QStringList actualColumns;
   while (schemaCheck.next()) {
       actualColumns << schemaCheck.value(1).toString(); // pragma table_info: cid,name,type,notnull,dflt_value,pk
   }
   schemaCheck.clear();

   if (!actualColumns.isEmpty() && actualColumns != kExpectedColumns) {
       setStatus("Existing datafiles table from a previous version has an incompatible layout -- rebuilding it (this only resets the conversion-tracking cache, not your converted files)");
       query.exec("drop table datafiles");
   }

   // fileSize is bigint, not int: a directory's recursive size
   // (effectiveFileSize()) routinely exceeds what a 32-bit column could
   // hold. SQLite's type affinity doesn't actually bound storage by this
   // declaration, so this is for clarity; the schema self-heal above only
   // compares column names, so this never forces a rebuild of an existing
   // "int"-declared table.
   // firstDetected: when mzWatcher itself first saw this file, independent
   // of the file's own modTime -- see firstDetectedTimes in mainWindow.h.
   bool ok = query.exec("create table if not exists datafiles(filename varchar(255), fileConverted int, fileAnalyzed int, fileSize bigint, modTime timestamp, firstDetected timestamp );");
   if(!ok) setStatus(query.lastError().text());
   query.clear();
 }


 void MainWindow::insertFileInfo(QString filename) {
	QSqlQuery query(DB);
	QFileInfo fi(filename);
        QString absfilepath=fi.absoluteFilePath();
        query.prepare("insert into datafiles(filename,fileConverted,fileAnalyzed,fileSize,modTime,firstDetected) values(?,0,0,?,?,?)");
        query.addBindValue(absfilepath);
	query.addBindValue(effectiveFileSize(fi));
	query.addBindValue(fi.lastModified().toString("yyyy-MM-dd hh:mm:ss"));
	// Not fi.lastModified() -- this is when WE first noticed the file, which
	// is the only thing the "never changed since detection" fallback in
	// processChangedFiles() can trust for a directory (see firstDetectedTimes).
	query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        if(!query.exec()) setStatus("insertFileInfo: " + query.lastError().text());
	query.clear();
}

 void MainWindow::markFileConverted(QString filename) {
         QSqlQuery query(DB);
         QFileInfo fi(filename);
         QString absfilepath=fi.absoluteFilePath();

         query.prepare(tr("update datafiles set fileConverted=1, fileSize=%1 where filename=\"%2\"")
                       .arg(effectiveFileSize(fi))
                       .arg(absfilepath));
         // .arg(fi.lastModified().toString("yyyy-MM-dd hh:mm:ss")


         if(!query.exec()) setStatus("SQL ERROR: " + query.lastError().text());
         query.clear();

         // Updated immediately, not left to the next getDBFileList() refresh:
         // once converted, dbFiles[file] is rewritten above to match the
         // current size, which also makes it equal fileList[file] -- exactly
         // the condition the "never changed" fallback in processChangedFiles()
         // looks for. Without this, a just-converted file would be
         // immediately reconverted on every subsequent scan until the next
         // DB refresh caught up.
         convertedFiles.insert(absfilepath);

         //remoteLogMessage("fileconverted", absfilepath,fi.size(), "File Converted");
 }


 void MainWindow::getDBFileList() {
     QSqlQuery query(DB);
     // Explicit column list, not "select *": a positional index into "*" would
     // silently shift (and misread fileConverted/firstDetected as something
     // else) the next time a column is added to the table.
     query.prepare("select filename, fileConverted, fileSize, firstDetected from datafiles where filename like \"%" + extension + "%\"");
     if(!query.exec()) { setStatus("Error: showDataFilesTable:" + query.lastError().text()); return; }
     while (query.next()) {
         QString filename = query.value(0).toString();
         bool converted = query.value(1).toBool();
         qint64 filesize = query.value(2).toLongLong();
         QDateTime firstDetected = QDateTime::fromString(query.value(3).toString(), "yyyy-MM-dd hh:mm:ss");

         dbFiles[filename]=filesize;
         firstDetectedTimes[filename]=firstDetected;
         if (converted) convertedFiles.insert(filename);
     }
 }

 void MainWindow::showDataFilesTable() {
	 QSqlQuery query(DB);
         // Explicit column list, not "select *" -- see getDBFileList().
         query.prepare("select filename, fileConverted, fileSize, modTime, strftime('%s','now') - strftime('%s',modTime) from datafiles where filename like \"%" + extension + "%\"");
         if(!query.exec()) { setStatus("Error: showDataFilesTable:" + query.lastError().text()); return; }

	 guiForm->treeWidget->clear();
	 while (query.next()) {
             QString filename = query.value(0).toString();
             qint64 filesize = query.value(2).toLongLong();
             QString modTime  = query.value(3).toString();
             int age = query.value(4).toInt();

             bool fileChanged=false;
             if (fileList.contains(filename) && fileList[filename] != filesize) {
                    fileChanged=true;

             }


             if (age > maxDayDiff*24*60*60) continue;
                bool converted   = query.value(1).toBool();

                 QTreeWidgetItem *item = new QTreeWidgetItem(guiForm->treeWidget);
                 item->setText(0, modTime);
                 item->setText(1, QString::number( fileList[filename]  - filesize ));
                 item->setText(2,filename);
                 if(converted) item->setBackground(0,Qt::green);

                 if(fileChanged) {
                     item->setBackground(0,Qt::yellow);
                     //remoteLogMessage("filesizechange", filename, fileList[filename], "File Size Changed");
                 }
	 }
	 query.clear();
}

 void MainWindow::remoteLogMessage(QString infotype, QString filename, int fileSize, QString msgText) {

     setStatus(infotype + "| \"" + filename + "\" | " + QString::number(fileSize) + " | " + msgText);

     if (guiForm->watchButton->isChecked() == false) return;
     if (guiForm->remoteLoging->isChecked() == false) return;
     if (guiForm->remoteServerUrl->text().isEmpty()) return;
     if (guiForm->instrumentId->text().isEmpty()) return;


     QString serverURL = guiForm->remoteServerUrl->text();
     QString instrumentId = guiForm->instrumentId->text();

     QUrl url(serverURL);

     QUrlQuery urlquery(serverURL);
     urlquery.addQueryItem("action",  "writelog");
     urlquery.addQueryItem("infotype", infotype);
     urlquery.addQueryItem("instrumentId", instrumentId);
     urlquery.addQueryItem("filename", filename);
     urlquery.addQueryItem("filesize", QString::number(fileSize));
     urlquery.addQueryItem("msgText",  msgText);

     url.setQuery(urlquery);

     auto reply = http.get(QNetworkRequest(url));

     //connect(reply, SIGNAL(readyRead()), this, SLOT(httpReadyRead()))


     //http.setHost(url.host());
     //connectionId = http.get(url.toEncoded());
     //setStatus( url.toEncoded());

 }

/*
 void MainWindow::readRemoteData(const QHttpResponseHeader &resp)
 {
     //qDebug() << "readRemoteData() << " << resp.statusCode();

     if (resp.statusCode() == 200 ) { //redirect
         QString response=http.readAll();
     } else {
         http.abort();
     }
 }
*/
