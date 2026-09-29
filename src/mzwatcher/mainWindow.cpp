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
        guiForm->gcsKeyFileEdit->setText(gcsKeyFile);
        guiForm->targetBucketEdit->setText(targetBucket);

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

	converter = new BackgroundThread(this);
	connect(converter,SIGNAL(statusChanged(QString)),this,SLOT(setStatus(QString)));

	gcsUploader = new BackgroundThread(this);
	connect(gcsUploader,SIGNAL(statusChanged(QString)),this,SLOT(setStatus(QString)));


        QString dbname=QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/mzWatcher.db";

        setStatus("Using Database: " + dbname);
        DB = QSqlDatabase::addDatabase("QSQLITE", dbname);
        DB.setDatabaseName(dbname);
        DB.open();
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

        QString nowTime = QTime::currentTime().toString("h:m:s ap");
	//guiForm->statusLabel->setText(status);
        guiForm->logWidget->append(nowTime + ":" + status);
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

        //convert windows backslash to unix forward slash
	sourceFolder = sourceFolder.replace("\\","\057"); // not "\/"
        destFolder = destFolder.replace("\\","\057");
	// didn't just do "/" as it messed up the Mac Emacs C++ mode


}

void MainWindow::updateFileList() {
        //setStatus("Updating file list");
	directoryList.clear();
        guiForm->monitorDial->setStyleSheet("background: yellow;");
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
    } else {
        guiForm->monitorDial->setStyleSheet("background: red;");
    }
}

void MainWindow::processChangedFiles() {
    if(!guiForm->watchButton->isChecked()) return;

    int minFileSize = guiForm->minimumsFileSize->value();
    int waitTime    = guiForm->converter_waitTime->value()*60;

    foreach(QString file, dbFiles.keys()) {

        if ( dbFiles[file] != fileList[file]) {
            QFileInfo fi(file);

            QDateTime now = QDateTime::currentDateTime();
            int ageSec = now.secsTo(fi.lastModified())*-1;
            int oneday = 3600*24; // if file is too old.. don't autoconvert

            if (ageSec < oneday && ageSec > waitTime and fi.size() > minFileSize && fi.size() != dbFiles[file]) {
                setStatus(tr("Processing changed file: %1 ").arg(file));
                processFile(file);
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

	setStatus("TempFile=" + tempConvertedFile );
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

    setStatus("Uploading to gs://" + targetBucket + "/" + objectPath);
    gcsUploader->setSystemCommand(command.first(), command.mid(1));
    gcsUploader->start();

    while (gcsUploader->isRunning()) {
        QApplication::processEvents();
    }
}

void MainWindow::makeBackupCopy(QString file) {

	QFileInfo sourceFileInfo(file);
	QString   sourceFileDir =  sourceFileInfo.absolutePath();
	QString   sourceFileName = sourceFileInfo.fileName();
        setStatus("\n\n\nProcessing " + sourceFileName);

	//source file is no longer available
	if(! sourceFileInfo.exists()) return;


	QString   destFile = file;
	destFile.replace(sourceFolder,destFolder);
        QFileInfo destFileInfo(destFile);
        QString   destFileDir =  destFileInfo.absolutePath();
        QString   destFileName(destFileDir + "/" + sourceFileName);
        QString destFileFormat = guiForm->destFileFormatBox->currentText();
        QString	  convertedFileName = destFile;
        convertedFileName = convertedFileName.replace(extension,destFileFormat);

        setStatus("Source="+sourceFolder);
        setStatus("Dest="+destFolder);
        setStatus("Dest Dir=" + destFileDir);
        setStatus("Source Dir=" + sourceFileDir);
        setStatus("Source File=" + sourceFileName);
        setStatus("Destination File=" + destFileName);


	if (destFile == file) {	//path is identical..
		setStatus("Dest and Source Path are identical.. copy failed");
	}


	//create destination path
	QDir destPath(destFileDir);
	if (! destPath.exists()) {
		bool ok = destPath.mkpath(destFileDir);
		if( !ok ){
			setStatus("Failed to make path=" + destFolder);
			return;
		}
	}

	//copy file if it doesn't exists, size changed, or modification time changed
        bool makeFileCopy = guiForm->makeCopyCheckBox->isChecked();
        if (makeFileCopy) {
            if(! destFileInfo.exists() ||
               destFileInfo.lastModified() != sourceFileInfo.lastModified() ||
               destFileInfo.size()         != sourceFileInfo.size() )
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
                markFileConverted(file);

                uploadConvertedFileToGcs(convertedFileName);
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
            qint64 fsize = fi.size();

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
	 QSize size = settings->value("size", QSize(400, 400)).toSize();
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

	 if( ! settings->contains("convertCommand") )
			 settings->setValue("convertCommand",QString("mzWiff.exe --mzXML \"%1\" \"%2\" "));

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
   bool ok = query.exec("create table if not exists datafiles(filename varchar(255), fileConverted int, fileAnalyzed int, fileSize int, modTime timestamp );");
   if(!ok) setStatus(query.lastError().text());
   query.clear();
 }


 void MainWindow::insertFileInfo(QString filename) {
	QSqlQuery query(DB);
	QFileInfo fi(filename);
        QString absfilepath=fi.absoluteFilePath();
        query.prepare("insert into datafiles(filename,fileConverted,fileAnalyzed,fileSize,modTime) values(?,0,0,?,?)");
        query.addBindValue(absfilepath);
	query.addBindValue(fi.size());
	query.addBindValue(fi.lastModified().toString("yyyy-MM-dd hh:mm:ss"));
        if(!query.exec()) setStatus("insertFileInfo: " + query.lastError().text());
	query.clear();
}

 void MainWindow::markFileConverted(QString filename) {
         QSqlQuery query(DB);
         QFileInfo fi(filename);
         QString absfilepath=fi.absoluteFilePath();

         query.prepare(tr("update datafiles set fileConverted=1, fileSize=%1 where filename=\"%2\"")
                       .arg(fi.size())
                       .arg(absfilepath));
         // .arg(fi.lastModified().toString("yyyy-MM-dd hh:mm:ss")


         if(!query.exec()) setStatus("SQL ERROR: " + query.lastError().text());
         query.clear();

         //remoteLogMessage("fileconverted", absfilepath,fi.size(), "File Converted");
 }


 void MainWindow::getDBFileList() {
     QSqlQuery query(DB);
     query.prepare("select *, strftime('%s','now') - strftime('%s',modTime) from datafiles where filename like \"%" + extension + "%\"");
     if(!query.exec()) { setStatus("Error: showDataFilesTable:" + query.lastError().text()); return; }
     while (query.next()) {
         QString filename = query.value(0).toString();
         int filesize = query.value(3).toInt();
         QString modTime  = query.value(4).toString();
         dbFiles[filename]=filesize;
     }
 }

 void MainWindow::showDataFilesTable() {
	 QSqlQuery query(DB);
         query.prepare("select *, strftime('%s','now') - strftime('%s',modTime) from datafiles where filename like \"%" + extension + "%\"");
         //query.addBindValue(extension);
         if(!query.exec()) { setStatus("Error: showDataFilesTable:" + query.lastError().text()); return; }

	 guiForm->treeWidget->clear();
	 while (query.next()) {
             QString filename = query.value(0).toString();
             int filesize = query.value(3).toInt();
             QString modTime  = query.value(4).toString();
             int age = query.value(5).toInt();

             bool fileChanged=false;
             if (fileList.contains(filename) && fileList[filename] != filesize) {
                    fileChanged=true;

             }


             if (age > maxDayDiff*24*60*60) continue;
                bool converted   = query.value(1).toBool();

                 QTreeWidgetItem *item = new QTreeWidgetItem(guiForm->treeWidget);
                 item->setText(0, query.value(4).toString());
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

     setStatus(infotype + "| " + filename + " | " + QString::number(fileSize) + " | " + msgText);

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
