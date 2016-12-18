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
    currentProject = 0;


    _treeWidget=new QTreeWidget(this);
    _treeWidget->setColumnCount(4);
    _treeWidget->setObjectName("Samples");
    _treeWidget->setHeaderHidden(true);
    connect(_treeWidget,SIGNAL(itemSelectionChanged()), SLOT(showInfo()));

    QToolBar *toolBar = new QToolBar(this);
    toolBar->setFloatable(false);
    toolBar->setMovable(false);

    QToolButton* loadButton = new QToolButton(toolBar);
    loadButton->setIcon(QIcon(rsrcPath + "/fileopen.png"));
    loadButton->setToolTip("Load Project");
    connect(loadButton,SIGNAL(clicked()), SLOT(loadProject()));

    QToolButton* saveButton = new QToolButton(toolBar);
    saveButton->setIcon(QIcon(rsrcPath + "/filesave.png"));
    saveButton->setToolTip("Save Project");
    connect(saveButton,SIGNAL(clicked()), SLOT(saveProject()));

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
    toolBar->addWidget(loadButton);
    toolBar->addWidget(saveButton);

    setTitleBarWidget(toolBar);
    setWidget(_treeWidget);

    fileLoader = new mzFileIO(this);
    connect(fileLoader,SIGNAL(finished()),this, SLOT(getSampleInfoSQLITE()));
    connect(fileLoader,SIGNAL(finished()),this, SLOT(updateSampleList()));

}

void ProjectDockWidget::changeSampleColor(QTreeWidgetItem* item, int col) {
    if (!item) item = _treeWidget->currentItem();
    if (item == NULL) return;

    if (col != 0) return;
    QVariant v = item->data(0,Qt::UserRole);

    mzSample*  sample =  v.value<mzSample*>();
    if ( sample == NULL) return;

     QColor color = QColor::fromRgbF(
            sample->color[0],
            sample->color[1],
            sample->color[2],
            sample->color[3] );

      lastUsedSampleColor = QColorDialog::getColor(color,this,"Select Sample Color",QColorDialog::ShowAlphaChannel);
      setSampleColor(item,lastUsedSampleColor);

      _treeWidget->update();
      _mainwindow->getEicWidget()->replot();
      currentProject = 0;

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
    if(item != NULL and item->type() == SampleType) showSample(item,0);
}


void ProjectDockWidget::changeSampleOrder() {

     QTreeWidgetItemIterator it(_treeWidget);
     int sampleOrder=0;
     bool changed = false;

     while (*it) {
         if ((*it)->type() == SampleType) {
            QVariant v =(*it)->data(0,Qt::UserRole);
            mzSample*  sample =  v.value<mzSample*>();
            if ( sample != NULL) {
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
    if (item == NULL) return;
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
        currentProject=0;

        _mainwindow->deleteAllPeakTables();
        unloadAllSamples();
    }
}

int ProjectDockWidget::bookmarkPeakGroup(PeakGroup* group) {
    if (!currentProject) saveProject();

    if (currentProject)
        return currentProject->writeGroupSqlite(group,0,"Bookmarks");
    else
        return -1;
}

void ProjectDockWidget::loadProjectSQLITE(QString fileName) {

    if(currentProject)  closeProject();
    if(currentProject) return;

    QSettings* settings = _mainwindow->getSettings();
    QFileInfo fileinfo(fileName);
    QString projectPath = fileinfo.path();
    QString projectName = fileinfo.fileName();
    _mainwindow->setWindowTitle(PROGRAMNAME + "_" + QString::number(MAVEN_VERSION) + " " + projectName);

    ProjectDB* selectedProject = new ProjectDB(fileName);

    QStringList pathlist;
    pathlist << projectPath
             << "."
             << ".."
             << settings->value("lastDir").value<QString>();

    QSqlQuery query(selectedProject->sqlDB);
    query.exec("select * from samples");

    QStringList filelist;

    while (query.next()) {
        QString fname   = query.value("filename").toString();
        QString sname   = query.value("name").toString();

        //skip files that have been loaded already
        bool checkLoaded=false;
        foreach(mzSample* loadedFile, _mainwindow->getSamples()) {
            if (loadedFile->sampleName == sname.toStdString()) checkLoaded=true;
            if (QString(loadedFile->fileName.c_str())== fname) checkLoaded=true;
        }

        if(checkLoaded == true) continue;  // skip files that have been loaded already

        //find location of the file
        QFileInfo sampleFile(fname);
        if (!sampleFile.exists()) {
            foreach(QString path, pathlist) {
                fname= path + QDir::separator() + sampleFile.fileName();
                if (sampleFile.exists())  break;
            }
        }

        if (!fname.isEmpty() ) {
            filelist << fname;
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

                s->setSampleOrder(sampleOrder);
                s->isSelected = isSelected;
                s->color[0]   = color_red;
                s->color[1]   = color_green;
                s->color[2]   = color_blue;
                s->color[3]   = color_alpha;
                s->setNormalizationConstant(norml_const);

             }
         }

    loadAllPeakTables();
}

void ProjectDockWidget::loadAllPeakTables() {

    if(!currentProject) return;

    currentProject->setSamples(_mainwindow->getSamples());

    //create tables for search results
    QStringList tableNames = currentProject->getSearchTableNames();
    for(QString searchTableName : tableNames) {
        TableDockWidget* table = _mainwindow->findPeakTable(searchTableName);
        if(!table) _mainwindow->addPeaksTable(searchTableName);
    }

    //clear previous results
    currentProject->clearLoadedPeakGroups();;

    //load all peakgroups
    currentProject->loadPeakGroups("peakgroups");
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

    ProjectDB* project=0;
    if (currentProject and currentProject->databaseName() == filename and currentProject->isOpen())  {
        qDebug() << "existing project..";
        project = currentProject;
    } else {
        qDebug() << "new project..";
        project = new ProjectDB(filename);
        project->openDatabaseConnection(filename);
    }

    lastOpennedProject=filename;
    currentProject=project;

    if(project->isOpen()) {
        project->deleteAll();;
        project->saveSamples(sampleSet);

        int groupCount=0;
        for(TableDockWidget* peakTable : _mainwindow->getAllPeakTables() ) {
            for(PeakGroup* group : peakTable->getGroups()) {
                groupCount++;
                project->writeGroupSqlite(group,0,peakTable->windowTitle());
            }
        }
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

    menu.exec(event->globalPos());
}

void ProjectDockWidget::keyPressEvent(QKeyEvent *e ) {
    //cerr << "TableDockWidget::keyPressEvent()" << e->key() << endl;
    if (e->key() == Qt::Key_Delete ) {
        unloadSample();
    }

    QDockWidget::keyPressEvent(e);
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
