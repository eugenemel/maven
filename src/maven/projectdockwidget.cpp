#include "projectdockwidget.h"
#include "projectDB.h"

ProjectDockWidget::ProjectDockWidget(QMainWindow *parent):
    QDockWidget("Samples", parent,Qt::Widget)
{

    _mainwindow = (MainWindow*) parent;

    setFloating(false);
    setWindowTitle("Samples");
    setObjectName("Samples");

    QFont font;
    font.setFamily("Helvetica");
    font.setPointSize(10);

    lastUsedSampleColor = QColor(Qt::green);
    lastOpennedProject = QString();
    currentProject = nullptr;


    _treeWidget=new QTreeWidget(this);
    _treeWidget->setColumnCount(4);
    _treeWidget->setObjectName("Samples");
    _treeWidget->setHeaderHidden(true);
    connect(_treeWidget,SIGNAL(itemSelectionChanged()), SLOT(showInfo()));

    QToolBar *toolBar = new QToolBar(this);
    toolBar->setFloatable(false);
    toolBar->setMovable(false);

    QToolButton* uploadSampleOrgButton = new QToolButton(toolBar);
    uploadSampleOrgButton->setIcon(QIcon(rsrcPath + "/metadata_upload.png"));
    uploadSampleOrgButton->setToolTip("Import Sample Organization Information");
    connect(uploadSampleOrgButton,SIGNAL(clicked()), SLOT(importSampleMetadata()));

    QToolButton* downloadSampleOrgButton = new QToolButton(toolBar);
    downloadSampleOrgButton->setIcon(QIcon(rsrcPath + "/metadata_download.png"));
    downloadSampleOrgButton->setToolTip("Export Sample Organization Information");
    connect(downloadSampleOrgButton,SIGNAL(clicked()), SLOT(exportSampleMetadata()));

    QToolButton* colorButton = new QToolButton(toolBar);
    colorButton->setIcon(QIcon(rsrcPath + "/colorfill.png"));
    colorButton->setToolTip("Change Sample Color");
    connect(colorButton,SIGNAL(clicked()), SLOT(changeColors()));

    QLineEdit*  filterEditor = new QLineEdit(toolBar);
    filterEditor->setMinimumWidth(10);
    filterEditor->setPlaceholderText("Sample name filter");
    connect(filterEditor, SIGNAL(textEdited(QString)), this, SLOT(filterTreeItems(QString)));

    toolBar->addWidget(filterEditor);
    toolBar->addWidget(colorButton);
    toolBar->addWidget(downloadSampleOrgButton);
    toolBar->addWidget(uploadSampleOrgButton);

    setTitleBarWidget(toolBar);
    setWidget(_treeWidget);

    fileLoader = new mzFileIO(this);

    connect(fileLoader,SIGNAL(finished()),this, SLOT(warnUserEmptySampleFiles()));

    connect(fileLoader,SIGNAL(finished()),this, SLOT(getSampleInfoSQLITE()));
    connect(fileLoader,SIGNAL(finished()),this, SLOT(updateSampleList()));

}

void ProjectDockWidget::warnUserEmptySampleFiles() {

    qDebug() << "ProjectDockWidget::warnUserEmptySampleFiles()";

    if (fileLoader->getEmptyFiles().size() > 0){
        QString msg = QString();

        int counter = 0;
        msg.append("mzFileIO::loadSamples(): The samples\n\n");
        for (auto file : fileLoader->getEmptyFiles()) {
            msg.append(file);
            msg.append("\n\n");
            if (counter >= 5) {
                msg.append("... and others...");
                break;
            }
            counter++;
        }
        msg.append("\n\ndid not contain any scans.");
        msg.append("\n\nPlease consider examining and reloading these file(s).");

        QMessageBox messageBox;

        messageBox.critical(nullptr, "Empty samples files",  msg);
    }

}

void ProjectDockWidget::changeSampleColor(QTreeWidgetItem* item, int col) {
    if (!item) item = _treeWidget->currentItem();
    if (!item) return;

    if (col != 0) return;
    QVariant v = item->data(0,Qt::UserRole);

    mzSample*  sample =  v.value<mzSample*>();
    if (!sample) return;

     QColor color = QColor::fromRgbF(
            sample->color[0],
            sample->color[1],
            sample->color[2],
            sample->color[3] );

      lastUsedSampleColor = QColorDialog::getColor(color,this,"Select Sample Color",QColorDialog::ShowAlphaChannel);
      setSampleColor(item,lastUsedSampleColor);

      _treeWidget->update();
      _mainwindow->getEicWidget()->replot();
      currentProject = nullptr;

}


void ProjectDockWidget::changeSampleSet(QTreeWidgetItem* item, int col) {

    if (item && item->type() == SampleType) {
        QVariant v = item->data(0,Qt::UserRole);
        mzSample*  sample =  v.value<mzSample*>();

        if ( col == 1 ) {
            QString setName = item->text(1).simplified();
            sample->setSetName(setName.toStdString());
        } else if ( col == 0) {
            QString sampleName = item->text(0).simplified();
            if (sampleName.length() > 0) {
                sample->setSampleName(sampleName.toStdString());
            }
        }
   // cerr <<"changeSampleSet: " << sample->sampleName << "  " << sample->getSetName() << endl;
    }
}


void ProjectDockWidget::changeNormalizationConstant(QTreeWidgetItem* item, int col) {
    if (item && item->type() == SampleType && col == 2 ) {
        QVariant v = item->data(0,Qt::UserRole);
        mzSample*  sample =  v.value<mzSample*>();
        if ( sample == NULL) return;

        bool ok=false;
        float x = item->text(2).toFloat(&ok);
        if (ok) sample->setNormalizationConstant(x);
        cerr <<"changeSampleSet: " << sample->sampleName << "  " << sample->getNormalizationConstant() << endl;
    }
}


void ProjectDockWidget::updateSampleList() {

    _mainwindow->setupSampleColors();
    vector<mzSample*>samples = _mainwindow->getSamples();
    std::sort(samples.begin(), samples.end(), mzSample::compSampleOrder);
    if ( samples.size() > 0 ) setInfo(samples);

    if ( _mainwindow->getEicWidget() ) {
        _mainwindow->getEicWidget()->replotForced();
    }
}


void ProjectDockWidget::selectSample(QTreeWidgetItem* item, int col) {
    if (item && item->type() == SampleType ) {
        QVariant v = item->data(0,Qt::UserRole);
        mzSample*  sample =  v.value<mzSample*>();
        if (sample && sample->scans.size() > 0 ) {

            _mainwindow->spectraWidget->setScan(sample,-1);
            _mainwindow->getEicWidget()->replot();

        }
    }
}

void ProjectDockWidget::showInfo() {
    QTreeWidgetItem* item = _treeWidget->currentItem();
    if(item and item->type() == SampleType) showSample(item,0);
}


void ProjectDockWidget::changeSampleOrder() {

     QTreeWidgetItemIterator it(_treeWidget);
     int sampleOrder=0;
     bool changed = false;

     while (*it) {
         if ((*it)->type() == SampleType) {
            QVariant v =(*it)->data(0,Qt::UserRole);
            mzSample*  sample =  v.value<mzSample*>();
            if (sample) {
                if ( sample->getSampleOrder() != sampleOrder )  changed=true;
                sample->setSampleOrder(sampleOrder);
                sampleOrder++;
            }
         }
         ++it;
    }

    if (changed) {
        _mainwindow->getEicWidget()->replot();
    }
}


void ProjectDockWidget::filterTreeItems(QString filterString) {
    QRegExp regexp(filterString,Qt::CaseInsensitive,QRegExp::RegExp);

    QTreeWidgetItemIterator it(_treeWidget);
    while (*it) {
        QTreeWidgetItem* item = (*it);
        ++it; //next item
        if (item->type() == SampleType) {
            if (filterString.isEmpty()) {
                item->setHidden(false);
            } else if (item->text(0).contains(regexp) || item->text(1).contains(regexp) || item->text(2).contains(regexp)) {
                item->setHidden(false);
            } else {
                item->setHidden(true);
            }
        }
    }

}

void ProjectDockWidget::changeColors() {

      //get selected items
      QList<QTreeWidgetItem*>selected = _treeWidget->selectedItems();
      if(selected.size() == 0) return;

      //ask for color
      lastUsedSampleColor = QColorDialog::getColor(lastUsedSampleColor,this,"Select Sample Color",QColorDialog::ShowAlphaChannel);

      //change colors of selected items
      foreach (QTreeWidgetItem* item, selected) {
          if (item->type() == SampleType) setSampleColor(item,lastUsedSampleColor);
      }

      _treeWidget->update();
      _mainwindow->getEicWidget()->replot();
}

QIcon ProjectDockWidget::getSampleIcon(mzSample* sample) {
    QColor color = getSampleColor(sample);
    QPixmap pixmap = QPixmap(20,20); pixmap.fill(color);
    return QIcon(pixmap);
}

QColor ProjectDockWidget::getSampleColor(mzSample* sample) {
    if(!sample) return Qt::black;
    return QColor::fromRgbF( sample->color[0], sample->color[1], sample->color[2], sample->color[3] );
}

void ProjectDockWidget::setSampleColor(QTreeWidgetItem* item, QColor color) {
    if (item == NULL) return;
    if (!color.isValid()) return;



    QVariant v = item->data(0,Qt::UserRole);
    mzSample*  sample =  v.value<mzSample*>();
    if ( sample == NULL) return;

    sample->color[0] = color.redF();
    sample->color[1] = color.greenF();
    sample->color[2] = color.blueF();
    sample->color[3] = color.alphaF();

    color.setAlphaF(0.7);
    QPixmap pixmap = QPixmap(20,20); pixmap.fill(color);
    QIcon coloricon = QIcon(pixmap);
    item->setIcon(0,coloricon);
    item->setBackgroundColor(0,color);
    item->setBackgroundColor(1,color);
}

void ProjectDockWidget::setInfo(vector<mzSample*>&samples) {

    if ( _treeWidget->topLevelItemCount() == 0 )  {
        _treeWidget->setAcceptDrops(true);
        _treeWidget->setMouseTracking(true);
        connect(_treeWidget,SIGNAL(itemClicked(QTreeWidgetItem*, int)), SLOT(selectSample(QTreeWidgetItem*, int)));
        connect(_treeWidget,SIGNAL(itemPressed(QTreeWidgetItem*, int)), SLOT(changeSampleOrder()));
        connect(_treeWidget,SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), SLOT(changeSampleColor(QTreeWidgetItem*,int)));
        connect(_treeWidget,SIGNAL(itemEntered(QTreeWidgetItem*, int)), SLOT(showSampleInfo(QTreeWidgetItem*,int)));
    }

    disconnect(_treeWidget,SIGNAL(itemChanged(QTreeWidgetItem*, int)),0,0);

    parentMap.clear();
    _treeWidget->clear();

    _treeWidget->setDragDropMode(QAbstractItemView::InternalMove);
    QStringList header; header << "Sample" << "Set" << "Scalling";
    _treeWidget->setHeaderLabels( header );
    _treeWidget->header()->setStretchLastSection(true);
    _treeWidget->setHeaderHidden(false);
    //_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    _treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    _treeWidget->setRootIsDecorated(true);
    _treeWidget->expandToDepth(10);

    for(int i=0; i < samples.size(); i++ ) {
        mzSample* sample = samples[i];
        if (!sample) continue;

        QTreeWidgetItem* parent = getParentFolder(QString(sample->fileName.c_str()));
        //QTreeWidgetItem* parent=NULL;
        QTreeWidgetItem *item=NULL;

        if (parent) {
            item = new QTreeWidgetItem(parent,SampleType);
        } else {
            item = new QTreeWidgetItem(_treeWidget,SampleType);
        }

        QColor color = QColor::fromRgbF( sample->color[0], sample->color[1], sample->color[2], sample->color[3] );

        /*QPushButton* colorButton = new QPushButton(this);
        QString qss = QString("*{ background-color: rgb(%1,%2,%3) }").arg(color.red()).arg(color.green()).arg(color.blue());
        connect(colorButton,SIGNAL(pressed()), SLOT(changeSampleColor(QTreeWidgetItem*,int)));
        colorButton->setStyleSheet(qss);
	*/

        QPixmap pixmap = QPixmap(20,20); pixmap.fill(color); QIcon coloricon = QIcon(pixmap);

        item->setBackgroundColor(0,color);
        item->setIcon(0,coloricon);
        item->setText(0,QString(sample->sampleName.c_str()));
        item->setData(0,Qt::UserRole,QVariant::fromValue(samples[i]));
        item->setText(1,QString(sample->getSetName().c_str()));
        item->setText(2,QString::number(sample->getNormalizationConstant(),'f',2));
       // _treeWidget->setItemWidget(item,3,colorButton);

        item->setFlags(Qt::ItemIsEditable|Qt::ItemIsSelectable|Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
        sample->isSelected  ? item->setCheckState(0,Qt::Checked) : item->setCheckState(0,Qt::Unchecked);
        item->setExpanded(true);
    }

    _treeWidget->resizeColumnToContents(0);
    connect(_treeWidget,SIGNAL(itemChanged(QTreeWidgetItem*, int)), SLOT(changeSampleSet(QTreeWidgetItem*,int)));
    connect(_treeWidget,SIGNAL(itemChanged(QTreeWidgetItem*, int)), SLOT(changeNormalizationConstant(QTreeWidgetItem*,int)));
    connect(_treeWidget,SIGNAL(itemChanged(QTreeWidgetItem*, int)), SLOT(showSample(QTreeWidgetItem*,int)));

}

void ProjectDockWidget::showSample(QTreeWidgetItem* item, int col) {
    if (!item) return;
    bool checked = (item->checkState(0) != Qt::Unchecked );

    if (item->type() == SampleType ) {
        QVariant v = item->data(0,Qt::UserRole);
        mzSample*  sample =  v.value<mzSample*>();
        if (sample)  {
            bool changed=false;
            sample->isSelected != checked ? changed=true : changed=false;
            sample->isSelected=checked;

            if(changed) {
                cerr << "ProjectDockWidget::showSample() changed! " << checked << endl;
                _mainwindow->getEicWidget()->replotForced();
            }
        }
    }
}
QTreeWidgetItem* ProjectDockWidget::getParentFolder(QString fileName) {
        //get parent name of the directory containg this sample
        QTreeWidgetItem* parent=NULL;
        if (!QFile::exists(fileName)) return NULL;

        QFileInfo fileinfo(fileName);
        QString path = fileinfo.absoluteFilePath();
        QStringList pathlist = path.split("/");
        if (pathlist.size() > 2 ) {
            QString parentFolder = pathlist[ pathlist.size()-2 ];
            if(parentMap.contains(parentFolder)) {
                parent = parentMap[parentFolder];
            } else {
                parent = new QTreeWidgetItem(_treeWidget);
                parent->setText(0,parentFolder);
                parent->setExpanded(true);
                parentMap[parentFolder]=parent;
            }
        }
        return parent;
}

void ProjectDockWidget::showSampleInfo(QTreeWidgetItem* item, int col) {

    if (item && item->type() == SampleType ) {
        QVariant v = item->data(0,Qt::UserRole);
        mzSample*  sample =  v.value<mzSample*>();

        QString ionizationMode = "Unknown";
        sample->getPolarity() < 0 ? ionizationMode="Negative" :  ionizationMode="Positive";

        if (sample)  {
            this->setToolTip(tr("m/z Range: %1-%2<br> rt Range: %3-%4<br> Scan#: %5 <br> MRMs #: %6<br> Ionization: %7<br> Filename: %8").
                   arg(sample->minMz).
                   arg(sample->maxMz).
                   arg(sample->minRt).
                   arg(sample->maxRt).
                   arg(sample->scanCount()).
                   arg(sample->srmScans.size()).
                   arg(ionizationMode).
                   arg(sample->fileName.c_str()));
        }
    }

}

void ProjectDockWidget::dropEvent(QDropEvent* e) {
    cerr << "ProjectDockWidget::dropEvent() " << endl;
    QTreeWidgetItem *item = _treeWidget->currentItem();
    if (item && item->type() == SampleType ) changeSampleOrder();
}

void ProjectDockWidget::saveProjectAs() {

    QSettings* settings = _mainwindow->getSettings();
    QString dir = ".";
    if ( settings->contains("lastDir") ) {
        QString ldir = settings->value("lastDir").value<QString>();
        QDir test(ldir);
        if (test.exists()) dir = ldir;
    }
     QString fileName = QFileDialog::getSaveFileName( this,"Save Project As (.mzrollDB)", dir, "mzRoll Project(*.mzrollDB)");

     if(!fileName.isEmpty()) {
        if(!fileName.endsWith(".mzrollDB",Qt::CaseInsensitive)) fileName = fileName + ".mzrollDB";
        saveProjectSQLITE(fileName);
        lastOpennedProject = fileName;
     }
}

void ProjectDockWidget::saveProject() {
    if (currentProject) {
        saveProjectSQLITE(lastOpennedProject);
        return;
    } else {
        saveProjectAs();
    }
}

void ProjectDockWidget::loadProject() {

    closeProject();

    //project is still open leave
    if (currentProject) return;

    //open project
    QSettings* settings = _mainwindow->getSettings();
    QString dir = ".";
    if ( settings->contains("lastDir") ) dir = settings->value("lastDir").value<QString>();

    QString fileName = QFileDialog::getOpenFileName( this, "Select Project To Open", dir, "All Files(*.mzrollDB)");
    if (fileName.isEmpty()) return;

    loadProjectSQLITE(fileName);

}

void ProjectDockWidget::closeProject() {
    if(!currentProject) return;

    QMessageBox::StandardButton reply = QMessageBox::question(this, "Save And Close Project ?", "Do you want to save and close this project?", QMessageBox::Yes | QMessageBox::Cancel);
    if (reply == QMessageBox::Yes) {
        saveProjectSQLITE(lastOpennedProject);
        currentProject->closeDatabaseConnection();
        delete(currentProject);
        currentProject=nullptr;

        _mainwindow->deleteAllPeakTables();
        unloadAllSamples();
    }
}

void ProjectDockWidget::loadProjectSQLITE(QString fileName) {

    if(currentProject)  closeProject();
    if(currentProject) return;

    QSettings* settings = _mainwindow->getSettings();
    QFileInfo fileinfo(fileName);
    QString projectPath = fileinfo.path();
    QString projectName = fileinfo.fileName();
    _mainwindow->setWindowTitle(PROGRAMNAME + "_" + QString(MAVEN_VERSION) + " " + projectName);

    ProjectDB* selectedProject = new ProjectDB(fileName);

    //load compounds stored in the project file
    DB.loadCompoundsSQL("ALL",selectedProject->sqlDB);

    QStringList pathlist;
    pathlist << projectPath
             << "."
             << ".."
             << settings->value("lastDir").value<QString>();

    QSqlQuery query(selectedProject->sqlDB);

    query.exec("select * from samples");

    QStringList filelist;

    while (query.next()) {
        QString filepath   = query.value("filename").toString();
        QString sname   = query.value("name").toString();

        //skip files that have been loaded already
        bool checkLoaded=false;
        foreach(mzSample* loadedFile, _mainwindow->getSamples()) {
            if (loadedFile->sampleName == sname.toStdString()) checkLoaded=true;
            if (QString(loadedFile->fileName.c_str())== filepath) checkLoaded=true;
        }

        if(checkLoaded == true) continue;  // skip files that have been loaded already

        //locate file
        QString locatedpath = ProjectDB::locateSample(filepath, pathlist);
        QFileInfo sampleFile(locatedpath);
        if (sampleFile.exists()) { filelist << locatedpath; continue; }

        int keepLooking = QMessageBox::Open;
        while(keepLooking == QMessageBox::Open) {
            QMessageBox msgBox;
            msgBox.setText("Could not locate sample");
            msgBox.setInformativeText(sname + " was not found. Add new sample folder?");
            msgBox.setStandardButtons(QMessageBox::Open | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Open);
            keepLooking = msgBox.exec();

            if (keepLooking) {
                QString dirName = QFileDialog::getExistingDirectory( this, "Select Folder with Sample Files", projectPath);
                if (not dirName.isEmpty()) {
                    QString filepath =  dirName + QDir::separator() + sname;
                    QFileInfo checkFile(filepath);
                    if (checkFile.exists()) {
                        filelist << filepath;
                        pathlist << dirName;

                        qDebug() << "Found!!!! " << filepath;
                        keepLooking = QMessageBox::Cancel;
                        break;
                    }
                }
            }
        }
    }

    this->lastOpennedProject = fileName;
    currentProject = selectedProject;
    fileLoader->setMainWindow(_mainwindow);
    fileLoader->loadSamples(filelist);

}

void ProjectDockWidget::getSampleInfoSQLITE() {


    QSqlQuery query(currentProject->sqlDB);
    query.exec("select * from samples");
    qDebug() << "getSampleInfoSQLITE()" << query.numRowsAffected();

    while (query.next()) {
             QString fname   = query.value("filename").toString();
             QString sname   = query.value("name").toString();
             QString setname  = query.value("setName").toString();
             int sampleId  =   query.value("sampleID").toInt();

             int sampleOrder  = query.value("sampleOrder").toInt();
             int isSelected   = query.value("isSelected").toInt();
             float color_red    = query.value("color_red").toDouble();
             float color_blue   = query.value("color_blue").toDouble();
             float color_green = query.value("color_green").toDouble();
             float color_alpha  = query.value("color_alpha").toDouble();
             float norml_const   = query.value("norml_const").toDouble(); if(norml_const == 0) norml_const=1.0;

            foreach(mzSample* s, _mainwindow->getSamples()) {
                if(s->sampleName != sname.toStdString()) continue;

                if (!sname.isEmpty() )  		s->sampleName = sname.toStdString();
                if (!setname.isEmpty() )  		s->setSetName(setname.toStdString());

                qDebug() <<  "Loading sampleId" << sampleId << sname;
                s->setSampleId(sampleId);
                s->setSampleOrder(sampleOrder);
                s->isSelected = isSelected;
                s->color[0]   = color_red;
                s->color[1]   = color_green;
                s->color[2]   = color_blue;
                s->color[3]   = color_alpha;
                s->setNormalizationConstant(norml_const);

             }
         }
    currentProject->setSamples(_mainwindow->getSamples());
    currentProject->doAlignment();
    loadAllPeakTables();
}

void ProjectDockWidget::loadAllPeakTables() {

    if(!currentProject) return;

    currentProject->setSamples(_mainwindow->getSamples());

    //create tables for search results
    currentProject->loadSearchParams();
    QStringList tableNames = currentProject->getSearchTableNames();
    for(QString searchTableName : tableNames) {

        TableDockWidget* table = _mainwindow->findPeakTable(searchTableName);

        if(!table){

            QString encodedTableInfo("");
            QString displayTableInfo("");

            string searchTableNameString = searchTableName.toLatin1().data();

            if (currentProject->diSearchParameters.find(searchTableNameString) != currentProject->diSearchParameters.end()) {
                shared_ptr<DirectInfusionSearchParameters> searchParams = currentProject->diSearchParameters[searchTableNameString];

                string encodedParams = searchParams->encodeParams();
                string displayParams = encodedParams;
                replace(displayParams.begin(), displayParams.end(), ';', '\n');
                replace(displayParams.begin(), displayParams.end(), '=', ' ');

                encodedTableInfo = QString(encodedParams.c_str());
                displayTableInfo = QString(displayParams.c_str());
            }

            _mainwindow->addPeaksTable(searchTableName, encodedTableInfo,displayTableInfo);
        }
    }

    qDebug() << "Created tables for search results.";

    //clear previous results
    currentProject->clearLoadedPeakGroups();

    qDebug() << "Cleared loaded peak groups.";

    //load all peakgroups
    currentProject->loadPeakGroups("peakgroups", _mainwindow->rumsDBDatabaseName);

    //Issue 73 / mzkitchen 8: load match table
    currentProject->loadMatchTable();

    for(int i=0; i < currentProject->allgroups.size(); i++ ) {
        PeakGroup* g = &(currentProject->allgroups[i]);
        currentProject->allgroups[i].groupStatistics();

        //put them in right place
        if(g->searchTableName.empty()) g->searchTableName="Bookmarks";
        TableDockWidget* table = _mainwindow->findPeakTable(g->searchTableName.c_str());
        if(table) table->addPeakGroup(g,false);
    }

    //updated display widgets
    for(TableDockWidget* t: _mainwindow->getAllPeakTables()) {
        t->showAllGroups();
    }
}

void ProjectDockWidget::saveProjectSQLITE(QString filename) {
    if (filename.isEmpty()) return;
    qDebug() << "saveProjectSQLITE()" << filename << endl;

    std::vector<mzSample*> sampleSet = _mainwindow->getSamples();
    if(sampleSet.size() == 0) return;

    ProjectDB* project = nullptr;

    if (currentProject and currentProject->databaseName() == filename and currentProject->isOpen())  {
        qDebug() << "existing project..";
        project = currentProject;
    } else {
        qDebug() << "new project..";
        project = new ProjectDB(filename);
        project->openDatabaseConnection(filename);

        if (currentProject && currentProject->isOpen()) {
            project->topMatch = currentProject->topMatch;
            project->allMatches = currentProject->allMatches;
        }
    }

    lastOpennedProject=filename;
    currentProject=project;

    if(project->isOpen()) {
        project->deleteAll(); /// this is crazy
        project->setSamples(sampleSet);
        project->saveSamples(sampleSet);
        project->saveAlignment();

        set<Compound*> compoundSet;

        unsigned int groupCount=0;

        map<QString, QString> searchTableData{};

        for(TableDockWidget* peakTable : _mainwindow->getAllPeakTables() ) {

            qDebug() << peakTable->windowTitle() << ": Starting peak group table save...";

            unsigned int onePeakTableCount = 0;

            for(PeakGroup* group : peakTable->getGroups()) {

                unsigned int numGroupsAdded = 1 + group->childCount();

                groupCount = groupCount + numGroupsAdded;

                //Issue 75:
                //Bookmarks take precedence over original table name (if it exists).
                QString searchTableName;
                if (peakTable->windowTitle() == "Bookmarks" || group->searchTableName.empty()) {
                     searchTableName = peakTable->windowTitle();
                } else {
                    searchTableName = QString(group->searchTableName.c_str());
                }

                //qDebug() << "Write PeakGroup to DB: ID=" << group->groupId << ", table=" << searchTableName;

                project->writeGroupSqlite(group, 0, searchTableName);
                onePeakTableCount = onePeakTableCount + numGroupsAdded;

                if(group->compound){
                    compoundSet.insert(group->compound);
                }

            }

            searchTableData.insert(make_pair(peakTable->windowTitle(), peakTable->getEncodedTableInfo()));

            qDebug() << peakTable->windowTitle() << ": Saved " << onePeakTableCount << "groups.";
        }

        project->savePeakGroupsTableData(searchTableData);

        qDebug() << "All tables: Saved" << groupCount << "groups.";

        project->saveCompounds(compoundSet);

        project->saveMatchTable();

        _mainwindow->setStatusText(tr("Saved %1 groups to %2").arg(QString::number(groupCount),filename));
    } else {
       qDebug() << "Can't write to closed project" << filename;
    }
}


void ProjectDockWidget::loadProjectXML(QString fileName) {

    QSettings* settings = _mainwindow->getSettings();

    QFileInfo fileinfo(fileName);
    QString projectPath = fileinfo.path();
    QString projectName = fileinfo.fileName();

    QFile data(fileName);
    if ( !data.open(QFile::ReadOnly) ) {
        QErrorMessage errDialog(this);
        errDialog.showMessage("File open: " + fileName + " failed");
        return;
    }

    QXmlStreamReader xml(&data);
    mzSample* currentSample=NULL;

    QStringList pathlist;
    pathlist << projectPath
             << "."
             << settings->value("lastDir").value<QString>();

    QString projectDescription;
    QStringRef currentXmlElement;

    int currentSampleCount=0;

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            currentXmlElement = xml.name();


            if (xml.name() == "sample") {
                currentSampleCount++;
                QString fname   = xml.attributes().value("filename").toString();
                QString sname   = xml.attributes().value("name").toString();
                QString setname   = xml.attributes().value("setName").toString();
                QString sampleOrder   = xml.attributes().value("sampleOrder").toString();
                QString isSelected   = xml.attributes().value("isSelected").toString();
                _mainwindow->setStatusText(tr("Loading sample: %1").arg(sname));
                _mainwindow->setProgressBar(tr("Loading Sample Number %1").arg(currentSampleCount),currentSampleCount,currentSampleCount+1);

                bool checkLoaded=false;
                foreach(mzSample* loadedFile, _mainwindow->getSamples()) {
                    if (QString(loadedFile->fileName.c_str())== fname) checkLoaded=true;
                }

                if(checkLoaded == true) continue;  // skip files that have been loaded already


                qDebug() << "Checking:" << fname;
                QFileInfo sampleFile(fname);
                if (!sampleFile.exists()) {
                    foreach(QString path, pathlist) {
                        fname= path + QDir::separator() + sampleFile.fileName();
                        qDebug() << "Checking if exists:" << fname;
                        if (sampleFile.exists())  break;
                    }
                }

                if ( !fname.isEmpty() ) {
                    mzFileIO* fileLoader = new mzFileIO(this);
                    fileLoader->setMainWindow(_mainwindow);
                    mzSample* sample = fileLoader->loadSample(fname);
                    delete(fileLoader);

                    if (sample) {
                        _mainwindow->addSample(sample);
                        currentSample=sample;
                        if (!sname.isEmpty() )  		sample->sampleName = sname.toStdString();
                        if (!setname.isEmpty() )  		sample->setSetName(setname.toStdString());
                        if (!sampleOrder.isEmpty())     sample->setSampleOrder(sampleOrder.toInt());
                        if (!isSelected.isEmpty()) 		sample->isSelected = isSelected.toInt();
                    } else {
                        currentSample=NULL;
                    }
                }
            }

            if (xml.name() == "color" && currentSample) {
                currentSample->color[0]   = xml.attributes().value("red").toString().toDouble();
                currentSample->color[1]   = xml.attributes().value("blue").toString().toDouble();
                currentSample->color[2]   = xml.attributes().value("green").toString().toDouble();
                currentSample->color[3]  = xml.attributes().value("alpha").toString().toDouble();
            }


        }
        if (xml.isCharacters() && currentXmlElement == "projectDescription") {
            projectDescription.append( xml.text() );
        }
    }
    data.close();

    // update other widget
    vector<mzSample*> samples = _mainwindow->getSamples();
    int sampleCount = _mainwindow->sampleCount();
    updateSampleList();
    if(_mainwindow->srmDockWidget->isVisible()) _mainwindow->showSRMList();
    if(_mainwindow->bookmarkedPeaks) _mainwindow->bookmarkedPeaks->loadPeakTableXML(fileName);
    if(_mainwindow->spectraWidget && sampleCount) _mainwindow->spectraWidget->setScan(samples[0]->getScan(0));

    lastOpennedProject = fileName;
}

void ProjectDockWidget::saveProjectXML(QString filename, TableDockWidget* peakTable) {


    if (filename.isEmpty() ) return;
    QFile file(filename);
    if (!file.open(QFile::WriteOnly) ) {
        QErrorMessage errDialog(this);
        errDialog.showMessage("File open " + filename + " failed");
        return;
    }

    QXmlStreamWriter stream(&file);
    stream.setAutoFormatting(true);
    stream.writeStartElement("project");

    stream.writeStartElement("samples");

    vector<mzSample*> samples = _mainwindow->getSamples();
    for(int i=0; i < samples.size(); i++ ) {
        mzSample* sample = samples[i];
        if (sample == NULL ) continue;

        stream.writeStartElement("sample");
        stream.writeAttribute("name",  sample->sampleName.c_str() );
        stream.writeAttribute("filename",  sample->fileName.c_str() );
        stream.writeAttribute("sampleOrder", QString::number(sample->getSampleOrder()));
        stream.writeAttribute("setName", sample->getSetName().c_str());
        stream.writeAttribute("isSelected", QString::number(sample->isSelected==true ? 1 : 0));

        stream.writeStartElement("color");
        stream.writeAttribute("red", QString::number(sample->color[0],'f',2));
        stream.writeAttribute("blue", QString::number(sample->color[1],'f',2));
        stream.writeAttribute("green", QString::number(sample->color[2],'f',2));
        stream.writeAttribute("alpha", QString::number(sample->color[3],'f',2));
        stream.writeEndElement();
        stream.writeEndElement();

    }
    stream.writeEndElement();

    if( peakTable ){
         peakTable->writePeakTableXML(stream);
    } else {
        _mainwindow->bookmarkedPeaks->writePeakTableXML(stream);
    }

    stream.writeEndElement();
    QSettings* settings = _mainwindow->getSettings();
    settings->setValue("lastSavedProject", filename);
    lastSavedProject=filename;
}

void ProjectDockWidget::contextMenuEvent ( QContextMenuEvent * event )
{
    QMenu menu;

    QAction* z0 = menu.addAction("Unload Selected Sample");
    connect(z0, SIGNAL(triggered()), this ,SLOT(unloadSample()));

    menu.addSeparator();

    QAction *z3 = menu.addAction("Toggle Visibility of Selected Samples");
    connect(z3, SIGNAL(triggered()), this, SLOT(toggleSelectedSamples()));

    QAction *z1 = menu.addAction("Make All Samples Visible");
    connect(z1, SIGNAL(triggered()), this, SLOT(allSamplesVisible()));

    QAction *z2 = menu.addAction("Make All Samples Invisible");
    connect(z2, SIGNAL(triggered()), this, SLOT(allSamplesInvisible()));

    menu.exec(event->globalPos());
}

void ProjectDockWidget::allSamplesVisible() {
    qDebug() << "ProjectDockWidget::allSamplesVisible()";

    for (unsigned int i = 0; i < _treeWidget->topLevelItemCount(); i++){
        QTreeWidgetItem* item = _treeWidget->topLevelItem(i);
        toggleSamplesVisibility(item, true);
    }

    _mainwindow->getEicWidget()->replotForced();

    if (_mainwindow->barPlotWidget->isVisible()) {
        _mainwindow->barPlotWidget->refresh();
    }
}

void ProjectDockWidget::allSamplesInvisible() {
    qDebug() << "ProjectDockWidget::allSamplesInvisible()";

    for (unsigned int i = 0; i < _treeWidget->topLevelItemCount(); i++){
        QTreeWidgetItem* item = _treeWidget->topLevelItem(i);
        toggleSamplesVisibility(item, false);
    }

    _mainwindow->getEicWidget()->replotForced();

    if (_mainwindow->barPlotWidget->isVisible()) {
        _mainwindow->barPlotWidget->refresh();
    }
}

void ProjectDockWidget::toggleSelectedSamples() {
    qDebug() << "ProjectDockWidget::toggleSelectedSamples()";

    for (unsigned int i = 0; i < _treeWidget->topLevelItemCount(); i++){

        QTreeWidgetItem* item = _treeWidget->topLevelItem(i);
        toggleSelectedSampleVisibility(item);
    }

    _mainwindow->getEicWidget()->replotForced();

    if (_mainwindow->barPlotWidget->isVisible()) {
        _mainwindow->barPlotWidget->refresh();
    }
}

void ProjectDockWidget::keyPressEvent(QKeyEvent *e ) {
    //cerr << "TableDockWidget::keyPressEvent()" << e->key() << endl;
    if (e->key() == Qt::Key_Delete ) {
        unloadSample();
    }

    QDockWidget::keyPressEvent(e);
}

void ProjectDockWidget::toggleSelectedSampleVisibility(QTreeWidgetItem *item) {

    if (!item) return;

    if(item->type() == SampleType && item->isSelected()) {
        QVariant v = item->data(0,Qt::UserRole);
        mzSample*  sample =  v.value<mzSample*>();
        sample->isSelected = !sample->isSelected;
        item->setCheckState(0, (sample->isSelected ? Qt::Checked : Qt::Unchecked));
    }

    for (int i = 0; i < item->childCount(); ++i){
        toggleSelectedSampleVisibility(item->child(i));
    }
}

void ProjectDockWidget::toggleSamplesVisibility(QTreeWidgetItem *item, bool isVisible) {

    if (!item) return;

    if(item->type() == SampleType){
        QVariant v = item->data(0,Qt::UserRole);
        mzSample*  sample =  v.value<mzSample*>();
        sample->isSelected = isVisible;
        item->setCheckState(0, (isVisible ? Qt::Checked : Qt::Unchecked));
    }

    for (int i = 0; i < item->childCount(); ++i){
        toggleSamplesVisibility(item->child(i), isVisible);
    }
}

void ProjectDockWidget::unloadAllSamples() {

        //delete samples
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Unload Samples", "Do you want unload all samples?", QMessageBox::Yes | QMessageBox::Cancel);
        if (reply != QMessageBox::Yes) return;

        _treeWidget->clear();

        for(mzSample* sample: _mainwindow->samples) {
            sample->isSelected=false;
            delete_all(sample->scans);
        }

        _mainwindow->samples.clear();
        _mainwindow->getEicWidget()->replotForced();
}


void ProjectDockWidget::unloadSample() {
    QTreeWidgetItem *item = _treeWidget->currentItem();
    if (item) {
        QVariant v = item->data(0,Qt::UserRole);
        mzSample*  sample =  v.value<mzSample*>();

        if ( sample == NULL) return;
        item->setHidden(true);
         _treeWidget->removeItemWidget(item,0);


        //mark sample as unselected
        sample->isSelected=false;
        delete_all(sample->scans);

        qDebug() << "Removing Sample " << sample->getSampleName().c_str();
        qDebug() << " Empting scan data #Scans=" << sample->scans.size();

        //remove sample from sample list
        for(unsigned int i=0; i<_mainwindow->samples.size(); i++) {
            if (_mainwindow->samples[i] == sample) {
                _mainwindow->samples.erase( _mainwindow->samples.begin()+i);
                break;
            }
        }
        qDebug() << "Number of Remaining Samples =" << _mainwindow->sampleCount();
        //delete(item);
    }

    _mainwindow->getEicWidget()->replotForced();
}

const string SEP = ",";

void ProjectDockWidget::importSampleMetadata(){

    if (_mainwindow->sampleCount() == 0) {
        QMessageBox::information(this, "No Samples Loaded", "No samples are currently loaded.\nPlease load samples before importing sample organization information.");
        return;
    }

    QString dir = ".";

    if ( _mainwindow->getSettings()->contains("lastDir") ) {
        QString ldir = _mainwindow->getSettings()->value("lastDir").value<QString>();
        QDir test(ldir);
        if (test.exists()) dir = ldir;
    }

    QString metadataFile = QFileDialog::getOpenFileName(
            this, "Select Sample Organization File:",
            dir,
                  tr("Experiment Metadata File(*.csv);;")+
                  tr("All Files(*.*)"));

    qDebug() << "Sample metadata file:" << metadataFile;

    ifstream sampleMetadata;
    sampleMetadata.open(metadataFile.toStdString().c_str());

    //determine column order from the headers
    map<string, unsigned int> indexOf;

    {
        string line;
        getline(sampleMetadata, line);
        replaceAll(line, "\r", "");
        vector<string> headerValues;
        split(line, ',', headerValues);
        for (unsigned int i = 0; i < headerValues.size(); i++) {
            indexOf[headerValues.at(i)] = i;
        }
    }

    vector<mzSample*>samples = _mainwindow->getSamples();

    string line;
    while (getline(sampleMetadata, line)) {

        replaceAll(line, "\r", "");

        vector<string> values;
        split(line, ',', values);

        if (values.size() != indexOf.size()) {
            qDebug() << "Skipping line \"" << line.c_str() << "\" b/c of mismatch between header count and value count.";
            continue;
        }

        string sampleName;
        string setName;
        int sampOrderNum = -1;
        int isSelectedInt = -1;

        float red = -1.0f;
        float green = -1.0f;
        float blue = -1.0f;
        float alpha = -1.0f;

        if (indexOf.find("sampleName") != indexOf.end()) {
            sampleName = values.at(indexOf["sampleName"]);
        }

        if (indexOf.find("setName") != indexOf.end()) {
            setName = values.at(indexOf["setName"]);
        }

        if (indexOf.find("sampleOrder") != indexOf.end()) {
            try {
                int sampOrderNumCandidate = stoi(values.at(indexOf["sampleOrder"]))-1; //Remove 1 to store as 0-indexed
                if (sampOrderNumCandidate >= 0 && sampOrderNumCandidate < static_cast<int>(samples.size())){
                    sampOrderNum = sampOrderNumCandidate;
                }
            } catch (...) {

            }
        }

        if (indexOf.find("isSelected") != indexOf.end()) {
            try {
                isSelectedInt = stoi(values.at(indexOf["isSelected"]));
            } catch (...) {

            }
        }

        if (indexOf.find("color_red") != indexOf.end()) {
            try {
                float redCandidate = stof(values.at(indexOf["color_red"]));
                if (redCandidate >= 0.0f && redCandidate <= 1.0f) {
                    red = redCandidate;
                }
            } catch (...) {

            }
        }

        if (indexOf.find("color_green") != indexOf.end()) {
            try {
                float greenCandidate = stof(values.at(indexOf["color_green"]));
                if (greenCandidate >= 0.0f && greenCandidate <= 1.0f) {
                    green = greenCandidate;
                }
            } catch (...) {

            }
        }

        if (indexOf.find("color_blue") != indexOf.end()) {
            try {
                float blueCandidate = stof(values.at(indexOf["color_blue"]));
                if (blueCandidate >= 0.0f && blueCandidate <= 1.0f) {
                    blue = blueCandidate;
                }
            } catch (...) {

            }
        }

        if (indexOf.find("color_alpha") != indexOf.end()) {
            try {
                float alphaCandidate = stof(values.at(indexOf["color_alpha"]));
                if (alphaCandidate >= 0.0f && alphaCandidate <= 1.0f) {
                    alpha = alphaCandidate;
                }
            } catch (...) {

            }
        }

        if (!sampleName.empty()) {
            for (auto& sample : samples) {
                if (sample->getSampleName() == sampleName) {

                    if (sampOrderNum != -1) {
                        sample->setSampleOrder(sampOrderNum);
                    }

                    if (!setName.empty()) {
                        sample->setSetName(setName);
                    }

                    if (isSelectedInt != -1) {
                        sample->isSelected = (isSelectedInt == 1 ? true : false);
                    }

                    if (red > -1.0f && blue > -1.0f && green > -1.0f && alpha > -1.0f) {
                        sample->color[0] = red;
                        sample->color[1] = green;
                        sample->color[2] = blue;
                        sample->color[3] = alpha;
                    }

                    break;
                }
            }
        }

    }

    sampleMetadata.close();

    std::sort(samples.begin(), samples.end(), mzSample::compSampleOrder);
    setInfo(samples);

    if ( _mainwindow->getEicWidget() ) {
        _mainwindow->getEicWidget()->replotForced();
    }
}

void ProjectDockWidget::exportSampleMetadata() {

    if (_mainwindow->sampleCount() == 0) {
        QMessageBox::information(this, "No Samples Loaded", "No samples are currently loaded.\nPlease load samples before exporting sample organization information.");
        return;
    }

    const QString fileName = QFileDialog::getSaveFileName(
            this, "Export Sample Organization", QString(),
            "Comma Separated Value file (*.csv)");

    ofstream sampleMetadata;
    sampleMetadata.open(fileName.toStdString().c_str());

    QStringList header;

    header << "sampleName"
           << "setName"
           << "sampleOrder"
           << "isSelected"
           << "color_red"
           << "color_green"
           << "color_blue"
           << "color_alpha"
           << "after_first_underscore";

    for (int i = 0; i < header.size(); i++) {
        if (i > 0) {
            sampleMetadata << SEP;
        }

        sampleMetadata << header[i].toStdString();
    }

    sampleMetadata << "\n";

    vector<mzSample*>samples = _mainwindow->getSamples();
    std::sort(samples.begin(), samples.end(), mzSample::compSampleOrder);

    for (auto& sample : samples) {

        string afterUnderscore = QString(sample->getSampleName().c_str())
                .section('_', 1)
                .toStdString();

        sampleMetadata << sample->getSampleName() << SEP
                       << sample->getSetName() << SEP
                       << (sample->getSampleOrder() + 1) << SEP //Add 1 to display as 1-indexed
                       << (sample->isSelected ? "1" : "0") << SEP
                       << sample->color[0] << SEP
                       << sample->color[1] << SEP
                       << sample->color[2] << SEP
                       << sample->color[3] << SEP
                       << afterUnderscore << "\n";

    }

    sampleMetadata.close();

}
