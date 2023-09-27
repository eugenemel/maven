#include "mainwindow.h"

QDataStream &operator<<( QDataStream &out, const mzSample* ) { return out; }
QDataStream &operator>>( QDataStream &in, mzSample* ) { return in; }
QDataStream &operator<<( QDataStream &out, const Compound* ) { return out; }
QDataStream &operator>>( QDataStream &in, Compound* ) { return in; }
QDataStream &operator<<( QDataStream &out, const PeakGroup* ) { return out; }
QDataStream &operator>>( QDataStream &in, PeakGroup* ) { return in; }
QDataStream &operator<<( QDataStream &out, const Scan* ) { return out; }
QDataStream &operator>>( QDataStream &in, Scan* ) { return in; }
QDataStream &operator<<( QDataStream &out, const mzSlice* ) { return out; }
QDataStream &operator>>( QDataStream &in, mzSlice* ) { return in; }
QDataStream &operator<<( QDataStream &out, const mzSlice& ) { return out; }
QDataStream &operator>>( QDataStream &in, mzSlice& ) { return in; }

using namespace mzUtils;

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent) {

 qRegisterMetaType<mzSample*>("mzSample*");
 qRegisterMetaTypeStreamOperators<mzSample*>("mzSample*");

 qRegisterMetaType<Compound*>("Compound*");
 qRegisterMetaTypeStreamOperators<Compound*>("Compound*");

 qRegisterMetaType<Adduct*>("Adduct*");
 //qRegisterMetaTypeStreamOperators<Adduct*>("Adduct*");

 qRegisterMetaType<Scan*>("Scan*");
 qRegisterMetaTypeStreamOperators<Scan*>("Scan*");

 qRegisterMetaType<PeakGroup*>("PeakGroup*");
 qRegisterMetaTypeStreamOperators<PeakGroup*>("PeakGroup*");

 qRegisterMetaType<mzSlice*>("mzSlice*");
 qRegisterMetaTypeStreamOperators<mzSlice*>("mzSlice*");

 qRegisterMetaType<mzSlice>("mzSlice");
 qRegisterMetaTypeStreamOperators<mzSlice>("mzSlice");

 qRegisterMetaType<QTextCursor>("QTextCursor");

  setWindowTitle(PROGRAMNAME + " " + QString(MAVEN_VERSION));
  qDebug() << "APP=" <<  QApplication::applicationName() << "VER=" <<  QApplication::applicationVersion();

  readSettings();

  QList<QString> dirs;
  QString methodsFolder =      settings->value("methodsFolder").value<QString>();
  dirs    << methodsFolder
            << "."
            << "./methods"
            << QApplication::applicationDirPath()  + "/methods"
            << QApplication::applicationDirPath() + "/../Resources/methods";

    QString defaultModelFile("default.model");
    defaultModelFile.replace(QRegExp(".*/"),"");  //clear path name..
    qDebug() << "Searching for method folder:";
    foreach (QString d, dirs) {
        qDebug() << " Checking dir: " + d;
        QFile test(d+ "/" + defaultModelFile);
        if (test.exists()) { methodsFolder=d; settings->setValue("methodsFolder", methodsFolder); break;}
    }

    qDebug() << "METHODS FOLDER=" << methodsFolder;

    vector<Compound*> emptyDb(0);

    //CONNECT TO COMPOUND DATABASE
    QString writeLocation = QStandardPaths::standardLocations(QStandardPaths::DataLocation).first();
    if(!QFile::exists(writeLocation))  { QDir dir; dir.mkdir(writeLocation); }
    qDebug() << "WRITE FOLDER=" <<  writeLocation;
    DB.connect(writeLocation + "/ligand.db");
    DB.loadCompoundsSQL("KNOWNS", DB.getLigandDB());

    // === Issue 76 ========================================== //

    QString commonAdducts =     methodsFolder + "/" + "ADDUCTS.csv";
    if(QFile::exists(commonAdducts))  {
        DB.availableAdducts = DB.loadAdducts(commonAdducts.toStdString());
    } else {
        DB.availableAdducts = DB.defaultAdducts();
    }

    //compare with settings to determine which adducts can be used
    vector<Adduct*> enabledAdducts;
    if (settings->contains("enabledAdducts")) {
        QString enabledAdductsList = settings->value("enabledAdducts").toString();
        QStringList enabledAdductNames = enabledAdductsList.split(";", QString::SkipEmptyParts);

        for (Adduct* adduct : DB.availableAdducts) {
            if (enabledAdductNames.contains(QString(adduct->name.c_str()))) {
                enabledAdducts.push_back(adduct);
            }
        }
    } else {

        //Issue 230: default to only M+H and M-H adducts
        vector<Adduct*> enabledAdductsCandidate{};
        for (auto adduct : DB.availableAdducts) {
            if (adduct->name == "[M+H]+" || adduct->name == "[M-H]-") {
                enabledAdductsCandidate.push_back(adduct);
                if (enabledAdductsCandidate.size() == 2) break;
            }
        }

        if (enabledAdductsCandidate.size() == 2) {
            enabledAdducts = enabledAdductsCandidate;
            settings->setValue("enabledAdducts", "[M+H]+;[M-H]-;");
        } else {
            enabledAdducts = DB.availableAdducts;
        }
    }

    DB.adductsDB = enabledAdducts;
    // === Issue 76 ========================================== //

    // Issue 127: Extra peak group tags
    QString tagsFile = methodsFolder + "/TAGS.csv";
    qDebug() << "Checking for tags in file:" << tagsFile;

    if (QFile::exists(tagsFile)) {
        DB.loadPeakGroupTags(tagsFile.toStdString());
    }

    clsf = new ClassifierNeuralNet();

    //Issue 430: Use previously saved file, if it exists
    bool isFoundClsfModel = false;
    QString clsfModelFilename;
    if (settings->contains("clsfModelFilename")) {
        clsfModelFilename = settings->value("clsfModelFilename").value<QString>();
        if (QFile::exists(clsfModelFilename)){
            clsf->loadModel(clsfModelFilename.toStdString());
            isFoundClsfModel = clsf->hasModel();
        }
    }

    if (!isFoundClsfModel) {
        clsfModelFilename = methodsFolder +  "/default.model";
        if(QFile::exists(clsfModelFilename)) {
            clsf->loadModel(clsfModelFilename.toStdString());
            isFoundClsfModel = clsf->hasModel();
        }
    }

    if (!isFoundClsfModel) {
        clsf->loadDefaultModel();
         qDebug() << "ERROR: Can't find default.model in method folder="  << methodsFolder;
         qDebug() << "       Using built-in neural network model";
    }

    //Compound Library  Manager
    libraryDialog = new LibraryMangerDialog(this);
    libraryDialog->setMainWindow(this);

    //progress Bar on the bottom of the page
    statusText  = new QLabel(this);
    statusText->setOpenExternalLinks(true);
    statusBar()->addPermanentWidget(statusText,1);

    progressBar =  new QProgressBar(this);
    progressBar->hide();
    statusBar()->addPermanentWidget(progressBar);

    //Issue 251
    percentageText = new QLabel(this);
    percentageText->hide();
    statusBar()->addPermanentWidget(percentageText);

    QToolButton *btnBugs = new QToolButton(this);
    btnBugs->setIcon(QIcon(rsrcPath + "/bug.png"));
    btnBugs->setToolTip(tr("Bug!"));
    connect(btnBugs,SIGNAL(clicked()),SLOT(reportBugs()));
    statusBar()->addPermanentWidget(btnBugs,0);

    setWindowIcon(QIcon(":/images/icon.png"));

    //dock widgets
    setDockOptions(QMainWindow::AllowNestedDocks | QMainWindow::VerticalTabs | QMainWindow::AnimatedDocks );

    //set main dock widget
    eicWidget = new EicWidget(this);
    setCentralWidget(eicWidgetController());

    spectraWidget = new SpectraWidget(this, 1);
    isotopeWidget =  new IsotopeWidget(this);
    massCalcWidget =  new MassCalcWidget(this);
    covariantsPanel= new TreeDockWidget(this,"Covariants",3);

    ms1ScansListWidget = new TreeDockWidget(this, "MS1 List", 7);
    ms1ScansListWidget->addMs1TitleBar();
    ms1ScansListWidget->setupMs1ScanHeader();
    ms1ScansListWidget->setExclusiveItemType(ScanType);
    ms1ScansListWidget->treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

    ms2ScansListWidget	= new TreeDockWidget(this,"MS2 List", 7);
    ms2ScansListWidget->setupScanListHeader();
    ms2ScansListWidget->setExclusiveItemType(ScanType); //Issue 189
    ms2ScansListWidget->treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

    ms3ScansListWidget = new TreeDockWidget(this, "MS3 List", 7);
    ms3ScansListWidget->addMs3TitleBar();
    ms3ScansListWidget->setupMs3ScanHeader();
    ms3ScansListWidget->setExclusiveItemType(ScanType);
    ms3ScansListWidget->treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

    ms2ConsensusScansListWidget	= new TreeDockWidget(this,"Consensus MS2 List", 3);
    ms2ConsensusScansListWidget->setupConsensusScanListHeader();
    ms2ConsensusScansListWidget->setExclusiveItemType(ScanVectorType);
    ms2ConsensusScansListWidget->treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

    //Issue 347
    srmTransitionDockWidget = new TreeDockWidget(this, "SRM Transition List", 3);
    srmTransitionDockWidget->setupSRMTransitionListHeader();
    srmTransitionDockWidget->setExclusiveItemType(SRMTransitionType);

    //Issue 259: Currently only support single scan selection
//    ms3ScansListWidget->treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

    srmDockWidget 	= new TreeDockWidget(this,"SRM List", 1); //Issue 189
    ligandWidget = new LigandWidget(this);
    heatmap	 = 	  new HeatMap(this);
    galleryWidget = new GalleryWidget(this);
    barPlotWidget = new SampleBarPlotWidget(this);
    isotopeLegendWidget = new IsotopeLegendWidget(this);

    bookmarkedPeaks = addPeaksTable("Bookmarks",
                                    QString("This is a reserved table containing manually curated \"bookmarked\" peak groups."),
                                    QString("This is a reserved table containing manually curated \"bookmarked\" peak groups.")
                                    );

    spectraDockWidget =  createDockWidget("Spectra",spectraWidget);
    heatMapDockWidget =  createDockWidget("HeatMap",heatmap);
    galleryDockWidget =  createDockWidget("Gallery",galleryWidget);
    barPlotDockWidget = createDockWidget("EIC Legend",barPlotWidget);
    isotopeLegendDockWidget = createDockWidget("Isotope Bar Plot", isotopeLegendWidget);
    scatterDockWidget =  new ScatterPlot(this);
    projectDockWidget =  new ProjectDockWidget(this);
    rconsoleDockWidget =  new RconsoleWidget(this);

    fragmentationSpectraWidget = new SpectraWidget(this, 2);
    fragmentationSpectraDockWidget =  createDockWidget("Fragmentation Spectra",fragmentationSpectraWidget);

    ms3SpectraWidget = new SpectraWidget(this, 3);
    ms3SpectraDockWidget = createDockWidget("MS3 Spectra", ms3SpectraWidget);

    ligandWidget->setVisible(false);
    covariantsPanel->setVisible(false);
    isotopeWidget->setVisible(false);
    massCalcWidget->setVisible(false);
    ms2ScansListWidget->setVisible(false);
    ms1ScansListWidget->setVisible(false);
    ms3ScansListWidget->setVisible(false);
    ms2ConsensusScansListWidget->setVisible(false);
    bookmarkedPeaks->setVisible(false);
    spectraDockWidget->setVisible(true);
    scatterDockWidget->setVisible(false);
    heatMapDockWidget->setVisible(false);
    galleryDockWidget->setVisible(false);
    projectDockWidget->setVisible(false);
    rconsoleDockWidget->setVisible(false);
    fragmentationSpectraDockWidget->setVisible(true);    //treemap->setVisible(false);
    ms3SpectraDockWidget->setVisible(false);
    srmDockWidget->setVisible(false);

    //peaksPanel->setVisible(false);
    //treeMapDockWidget =  createDockWidget("TreeMap",treemap);

    addDockWidget(Qt::RightDockWidgetArea, barPlotDockWidget, Qt::Horizontal);
    addDockWidget(Qt::RightDockWidgetArea, isotopeLegendDockWidget, Qt::Horizontal);

    //
    //DIALOGS
    //

    selectAdductsDialog = new SelectAdductsDialog(this, this, settings);

    //peak detection dialog
    peakDetectionDialog = new PeakDetectionDialog(this);
    peakDetectionDialog->setMainWindow(this);
    peakDetectionDialog->setSettings(settings);

    //direct infusion dialog
    directInfusionDialog = new DirectInfusionDialog(this);
    directInfusionDialog->setMainWindow(this);
    directInfusionDialog->setSettings(settings);

    //alignment dialog
    alignmentDialog	 =  new AlignmentDialog(this);
    connect(alignmentDialog->alignButton,SIGNAL(clicked()),SLOT(Align()));
    connect(alignmentDialog->UndoAlignment,SIGNAL(clicked()),SLOT(UndoAlignment()));

    //calibration dialog
    calibrateDialog	 =  new CalibrateDialog(this);

    //Set RumsDB dialog
    setRumsDBDialog  = nullptr; //Issue #121 Intialize to nullptr to avoid Segmentation Fault

    //rconsole dialog
    //rconsoleDialog	 =  new RConsoleDialog(this);

    //settings dialog
    settingsForm = new SettingsForm(settings,this);

    spectraMatchingForm =  new SpectraMatching(this);

    connect(scatterDockWidget, SIGNAL(groupSelected(PeakGroup*)), SLOT(setPeakGroup(PeakGroup*)));

    addDockWidget(Qt::LeftDockWidgetArea,ligandWidget,Qt::Vertical);
    addDockWidget(Qt::LeftDockWidgetArea,projectDockWidget,Qt::Vertical);

    ligandWidget->setAllowedAreas(Qt::LeftDockWidgetArea);
    projectDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea);


    addDockWidget(Qt::BottomDockWidgetArea,spectraDockWidget,Qt::Horizontal);
    addDockWidget(Qt::BottomDockWidgetArea,covariantsPanel,Qt::Horizontal);
    addDockWidget(Qt::BottomDockWidgetArea,scatterDockWidget,Qt::Horizontal);
    addDockWidget(Qt::BottomDockWidgetArea,bookmarkedPeaks,Qt::Horizontal);
    addDockWidget(Qt::BottomDockWidgetArea,galleryDockWidget,Qt::Horizontal);
    addDockWidget(Qt::BottomDockWidgetArea,srmDockWidget,Qt::Horizontal);
   // addDockWidget(Qt::BottomDockWidgetArea,rconsoleDockWidget,Qt::Horizontal);
    addDockWidget(Qt::BottomDockWidgetArea,fragmentationSpectraDockWidget,Qt::Horizontal);
    addDockWidget(Qt::BottomDockWidgetArea,ms3SpectraDockWidget,Qt::Horizontal);

    addDockWidget(Qt::BottomDockWidgetArea,ms1ScansListWidget, Qt::Horizontal);
    addDockWidget(Qt::BottomDockWidgetArea,ms2ScansListWidget, Qt::Horizontal);
    addDockWidget(Qt::BottomDockWidgetArea,ms3ScansListWidget, Qt::Horizontal);
    addDockWidget(Qt::BottomDockWidgetArea,ms2ConsensusScansListWidget, Qt::Horizontal);
    addDockWidget(Qt::BottomDockWidgetArea,srmTransitionDockWidget, Qt::Horizontal);

    //addDockWidget(Qt::BottomDockWidgetArea,peaksPanel,Qt::Horizontal);
    //addDockWidget(Qt::BottomDockWidgetArea,treeMapDockWidget,Qt::Horizontal);
    //addDockWidget(Qt::BottomDockWidgetArea,heatMapDockWidget,Qt::Horizontal);

    tabifyDockWidget(ligandWidget, projectDockWidget);

    tabifyDockWidget(spectraDockWidget,massCalcWidget);
    tabifyDockWidget(spectraDockWidget,isotopeWidget);
    tabifyDockWidget(spectraDockWidget,covariantsPanel);
    tabifyDockWidget(spectraDockWidget,galleryDockWidget);
    //tabifyDockWidget(spectraDockWidget,rconsoleDockWidget);

    tabifyDockWidget(ms1ScansListWidget, ms2ScansListWidget);
    tabifyDockWidget(ms1ScansListWidget, ms3ScansListWidget);
    tabifyDockWidget(ms1ScansListWidget, ms2ConsensusScansListWidget);

    tabifyDockWidget(srmDockWidget, srmTransitionDockWidget);

    setContextMenuPolicy(Qt::NoContextMenu);

    if (settings->contains("windowState")) {
        restoreState(settings->value("windowState").toByteArray());
    }

    if ( settings->contains("geometry")) {
    	restoreGeometry(settings->value("geometry").toByteArray());
    }

    projectDockWidget->show();
    scatterDockWidget->hide();
    rconsoleDockWidget->hide();

    ms1ScansListWidget->hide();
    ms2ScansListWidget->hide();
    ms3ScansListWidget->hide();
    ms2ConsensusScansListWidget->hide();

    srmTransitionDockWidget->hide();
    srmDockWidget->hide();

    setUserPPM(5);
    if ( settings->contains("ppmWindowBox")) {
    	setUserPPM(settings->value("ppmWindowBox").toDouble());
    }

    QRectF view = settings->value("mzslice").value<QRectF>();
    if (view.width() > 0 && view.height() > 0 ) {
        eicWidget->setMzSlice(mzSlice(view.x(),view.y(),view.width(),view.height()));
    } else {
        eicWidget->setMzSlice(mzSlice(0,0,0,100));
    }

    createMenus();
    createToolBars();

    ligandWidget->updateDatabaseList();
    massCalcWidget->updateDatabaseList();

    //Issue 246: setting a db as a part of start up can be very slow

    //Issue 246: This doesn't make much sense any more
    //ligandWidget->setDatabase("KNOWNS");

//    if(settings->contains("lastCompoundDatabase")){
//        ligandWidget->setDatabase(settings->value("lastCompoundDatabase").toString());
//    }

    setIonizationMode(0);
    if ( settings->contains("ionizationMode")) {
        setIonizationMode(settings->value("ionizationMode").toInt());
    }

    setAcceptDrops(true);

    showNormal();	//return from full screen on startup

    //remove close button from dockwidget
    QList<QDockWidget *> dockWidgets = this->findChildren<QDockWidget *>();
    for (int i = 0; i < dockWidgets.size(); i++) {
        dockWidgets[i]->setFeatures( dockWidgets[i]->features() ^ QDockWidget::DockWidgetClosable );
    }


    //check if program exited correctly last time
    if (settings->contains("closeEvent") and settings->value("closeEvent").toInt() == 0) {

        setUrl("https://github.com/eugenemel/maven/issues",
                "Whoops... did the program crash last time? Would you like to report a bug?");
    }

    settings->setValue("closeEvent", 0 );

    connect(libraryDialog, SIGNAL(loadLibrarySignal(QString)), this, SLOT(updateAdductsInGUI()));
}

QDockWidget* MainWindow::createDockWidget(QString title, QWidget* w) {
    QDockWidget* dock =  new QDockWidget(title, this, Qt::Widget);
    dock->setAllowedAreas(Qt::AllDockWidgetAreas);
    dock->setFloating(false);
    dock->setVisible(false);
    dock->setObjectName(title);
    dock->setWidget(w);
    return dock;

}

void MainWindow::reportBugs() {
    //QUrl link("http://genomics-pubs.princeton.edu/mzroll/index.php?show=bugs");
    QUrl link("https://github.com/eugenemel/maven/issues");
    QDesktopServices::openUrl(link);

}

void MainWindow::openReference() {
    QDesktopServices::openUrl(QUrl("https://www.mdpi.com/2218-1989/12/8/684/htm"));
}

void MainWindow::openTutorialVideo() {
    QDesktopServices::openUrl(QUrl("https://www.youtube.com/watch?v=QUSX0GJ6Gsk"));
}

void MainWindow::setUrl(QString url,QString link) {

    if(url.isEmpty()) return;
    if(link.isEmpty()) link="Link";
    setStatusText(tr("<a href=\"%1\">%2</a>").arg(url, link));
}


void MainWindow::setUrl(Compound* c) {
    if(c==NULL) return;
    QString hmdbURL="http://www.hmdb.ca/metabolites/";
    QString keggURL= "http://www.genome.jp/dbget-bin/www_bget?";
    QString pubChemCidUrl= "https://pubchem.ncbi.nlm.nih.gov/compound/";
    QString pubChemURL= "http://www.ncbi.nlm.nih.gov/sites/entrez?db=pccompound&term=";
    QString lipidMapsURL= "http://www.lipidmaps.org/data/LMSDRecord.php?LMID=";

   QString id = c->id.c_str();
   QString hmdbId("HMDB");
   QString lipidmapsId("LM");
   QRegExp keggId("^C\\d+$");
   QRegExp pubmedId("^\\d+$");

    QString url;
    if ( id.contains(hmdbId,Qt::CaseInsensitive) ) {
        url = hmdbURL + id;
    } else if ( id.startsWith(lipidmapsId,Qt::CaseInsensitive) ) {
       url = lipidMapsURL + id;
    } else if ( id.contains(keggId) ) {
        url = keggURL+tr("%1").arg(id);
    } else if ( id.contains(pubmedId) ) {
        url = pubChemCidUrl + id;
    } else {
        url = pubChemURL+tr("%1").arg(c->name.c_str());
    }

    QString link(c->id.c_str());
    setUrl(url,link);
}

TableDockWidget* MainWindow::findPeakTable(QString title) {
    for(TableDockWidget* x: groupTables) {
        if(x->windowTitle() == title) return x;
    }
    return nullptr;
}

QString MainWindow::getUniquePeakTableTitle(QString title) {

    if (title.isNull() || title.isEmpty()) {
        title = QString("Peak Group Table");
    }

    int counter = 2;
    while (findPeakTable(title)){

        if (counter != 2) {
            QStringList list = title.split(QRegExp("\\s+"), QString::SkipEmptyParts);
            title = QString("");
            for (unsigned int i = 0; i < list.length()-1; i++) {
                title.append(list.at(i));
                title.append(" ");
            }
        } else {
            title.append(" ");
        }

        title.append(QString::number(counter));
        counter++;
    }

    return title;
}

void MainWindow::deleteAllPeakTables() {
    for(TableDockWidget* t: groupTables) {
        t->deleteAll();
        deletePeakTable(t);
    }
}

void MainWindow::deletePeakTable(TableDockWidget *x) {
        if(x == bookmarkedPeaks) return;
        x->setVisible(false);
        groupTables.removeOne(x);
}

TableDockWidget* MainWindow::addPeaksTable(QString title, QString encodedTableInfo, QString displayTableInfo) {
    QPointer<TableDockWidget> panel	 = new TableDockWidget(this, title, 0, encodedTableInfo, displayTableInfo);
    panel->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::BottomDockWidgetArea,panel,Qt::Horizontal);
    groupTables.push_back(panel);

    if (sideBar) {
        /*
        QToolButton *btnTable = new QToolButton(sideBar);
        btnTable->setIcon(QIcon(rsrcPath + "/featuredetect.png"));
        btnTable->setChecked( panel->isVisible() );
        btnTable->setCheckable(true);
        btnTable->setToolTip(title);
        connect(btnTable,SIGNAL(clicked(bool)),panel, SLOT(setVisible(bool)));
        connect(panel,SIGNAL(visibilityChanged(bool)),btnTable,SLOT(setChecked(bool)));
        sideBar->addWidget(btnTable);
        */
    }

    return panel;
}


void MainWindow::setUserPPM( double x) {
    _ppmWindow=x;
}

void MainWindow::setIonizationMode( int x ) {
    qDebug() << "setIonizationMode: " << x;
    _ionizationMode=x;
     massCalcWidget->setCharge(_ionizationMode);

     //Issue 376: Update to respect loaded adducts.

     for (int i = 0; i < adductType->count(); i++) {

         QVariant v = adductType->itemData(i);
         Adduct*  itemAdduct =  v.value<Adduct*>();

         if ((itemAdduct->charge > 0 && itemAdduct->name == "[M+H]+") ||
                 (itemAdduct->charge < 0 && itemAdduct->name == "[M-H]-")) {
             this->adductType->setCurrentIndex(i);
             break;
         }
     }

     //Issue 376: adducts combo box in isotopeWidget should sync with main window adducts combo box
     if (isotopeWidget) {
         for (int i = 0; i < isotopeWidget->adductComboBox->count(); i++) {

             QVariant v = isotopeWidget->adductComboBox->itemData(i);
             Adduct*  itemAdduct =  v.value<Adduct*>();
             if ((itemAdduct->charge > 0 && itemAdduct->name == "[M+H]+") ||
                     (itemAdduct->charge < 0 && itemAdduct->name == "[M-H]-")) {
                 isotopeWidget->adductComboBox->setCurrentIndex(i);
                 break;
             }
         }
     }
}

vector<mzSample*> MainWindow::getVisibleSamples() {

    vector<mzSample*>vsamples;
    for(int i=0; i < samples.size(); i++ ) {
        if (samples[i] && samples[i]->isSelected ) {
            vsamples.push_back(samples[i]);
        }
    }
    return vsamples;
}

float MainWindow::getVisibleSamplesMaxRt(){

    float maxRt = 0.0f;

    for(unsigned int i=0; i < samples.size(); i++ ) {
        if (samples[i] && samples[i]->isSelected ) {
            if (samples[i]->maxRt > maxRt) {
                maxRt = samples[i]->maxRt;
            }
        }
    }

    return maxRt;
}

void MainWindow::bookmarkSelectedPeakGroup() {
   bookmarkPeakGroup(eicWidget->getSelectedGroup());
}



void MainWindow::bookmarkPeakGroup(PeakGroup* group) {
    qDebug() << "MainWindow::bookmarkPeakGroup(group)";
    if ( bookmarkedPeaks->isVisible() == false ) {
        bookmarkedPeaks->setVisible(true);
    }

    bool isWarnMzRt = settings->value("chkBkmkWarnMzRt", false).toBool();
    bool isWarnMz = settings->value("chkBkmkWarnMz", false).toBool();
    float mzTol = settings->value("spnBkmkmzTo", 10.0f).toFloat();
    float rtTol = settings->value("spnBkmkRtTol", 0.50f).toFloat();

    bool isAddBookmark = false;
    if (isWarnMzRt || isWarnMz) {

        QString warnType = isWarnMzRt ? "m/z and RT" : "m/z";

        QString msg = QString("The following bookmarked peak groups have similar ");
        msg.append(warnType);
        msg.append(" values:");
        msg.append("\n\n");

        bool isFoundSimilarBookmark = false;
        for (auto pg : bookmarkedPeaks->getAllGroups()){
            if (pg->deletedFlag) continue;
            if (mzUtils::ppmDist(group->meanMz, pg->meanMz) <= mzTol
                    && (abs(group->meanRt - pg->meanRt) <= rtTol || isWarnMz)) {
                msg.append(bookmarkedPeaks->groupTagString(pg));
                msg.append("\n");
                isFoundSimilarBookmark = true;
            }
        }

        msg.append("\n");
        msg.append("Are you sure you would like to add this bookmark?");

        if (isFoundSimilarBookmark) {
            QMessageBox::StandardButton reply = QMessageBox::question(
                        this,
                        "Similar Bookmark Exists",
                        msg,
                        QMessageBox::Yes|QMessageBox::No
                        );

            isAddBookmark = reply == QMessageBox::Yes;
        } else {
            isAddBookmark = true;
        }


    } else {
        isAddBookmark = true;
    }

    //Issue 280: Always make a copy of the peak group.
    if (isAddBookmark) {

        PeakGroup *groupCopy = new PeakGroup(*group);

        //TODO: separate children peak from isotopic peaks

        //Issue 408: Always add children peaks
//        bool isAddChildren = settings->value("chkIncludeChildren", false).toBool();
//        if (!isAddChildren) {
//            groupCopy->children.clear();
//        } else

        if (groupCopy->children.empty()){

            IsotopeParameters isotopeParameters = groupCopy->isotopeParameters;
            if (isotopeParameters.isotopeParametersType == IsotopeParametersType::INVALID) {
                isotopeParameters = getIsotopeParameters();
            }

            //Issue 615
            groupCopy->pullIsotopes(isotopeParameters, true);

            isotopeParameters.isotopeParametersType = IsotopeParametersType::SAVED;
            groupCopy->isotopeParameters = isotopeParameters;
        }

        groupCopy->searchTableName = "Bookmarks";

        //Issue 617: Retrieve data from EIC widget, copy into PeakGroup
        if (getEicWidget()->getMzSlice().isSrmTransitionSlice() && !groupCopy->compound) {

            string srmName = getEicWidget()->getMzSlice().srmTransition->name;

            if (srmName != "") {
                Compound *compound = new Compound(getEicWidget()->getMzSlice().srmTransition->name,
                                                  getEicWidget()->getMzSlice().srmTransition->name,
                                                  "",
                                                  0,
                                                  0);
                compound->db = "Unmatched SRM Transition";
                compound->category.push_back(srmName);
                compound->srmTransition = getEicWidget()->getMzSlice().srmTransition;
                compound->srmId = srmName;
                compound->precursorMz = compound->srmTransition->precursorMz;
                compound->productMz = compound->srmTransition->productMz;
                compound->cid = -1;

                groupCopy->compound = compound;
                groupCopy->displayName = srmName;
            }

            groupCopy->setType(PeakGroup::SRMTransitionType);

        }

        //Issue 277: tabledockwidget retrieves data from reference, copies it, and returns reference to copied data
        PeakGroup *groupCopy2 = bookmarkedPeaks->addPeakGroup(groupCopy, true, true);

        qDebug() << "MainWindow::bookmarkPeakGroup() group=" << groupCopy2;
        qDebug() << "MainWindow::bookmarkPeakGroup() group data:";
        qDebug() << "MainWindow::bookmarkPeakGroup() group->meanMz=" << groupCopy2->meanMz;
        qDebug() << "MainWindow::bookmarkPeakGroup() group->meanRt=" << groupCopy2->meanRt;
        if(groupCopy2->compound) qDebug() << "MainWindow::bookmarkPeakGroup() group->compound=" << groupCopy2->compound->name.c_str();
        if(!groupCopy2->compound) qDebug() << "MainWindow::bookmarkPeakGroup() group->compound= nullptr";
        if (groupCopy2->adduct){
            qDebug() << "MainWindow::bookmarkPeakGroup() group->adduct=" << groupCopy2->adduct->name.c_str();
        } else {
            qDebug() << "MainWindow::bookmarkPeakGroup() group->adduct= nullptr";
        }

        //Issue 279: re-filter tree after adding bookmark
        bookmarkedPeaks->filterTree();
        bookmarkedPeaks->selectGroup(groupCopy2);
    }
}

void MainWindow::setFormulaFocus(QString formula) {
    Adduct* adduct = getUserAdduct();
    MassCalculator mcalc;
    //double parentMass = mcalc.computeNeutralMass(formula.toStdString());
    //double pmz = adduct->computeAdductMass(parentMass);

    if ( eicWidget->isVisible() ) {
        Compound *unk = new Compound(formula.toStdString(),formula.toStdString(),formula.toStdString(),0);
        eicWidget->setCompound(unk,adduct);
        //eicWidget->setMzSlice(pmz);
    }
    isotopeWidget->setFormula(formula);
}

void MainWindow::setPeptideFocus(QString peptideSequence){
    qDebug() << "MainWindow::setPeptideFocus() sequence=" << peptideSequence;

    double monoisotopicMass = 0.0;

    bool isFoundFirstDot = false;
    bool isFoundSecondDot = false;

    for (int i = 0; i < peptideSequence.size(); i++){

        QChar c = peptideSequence[i];

        if (c == '[') {

            int numDeuteria = 0;
            if (i < peptideSequence.size()-2 && peptideSequence[i+1] == 'd') {
                int startPos = i+2;
                int endPos = -1;

                for (int pos = startPos; pos < peptideSequence.size(); pos++) {
                    if (peptideSequence[pos] == ']') {
                        endPos = pos;
                        break;
                    }
                }

                if (endPos > startPos) {
                    try {
                        numDeuteria = stoi(peptideSequence.toStdString().substr(
                                               static_cast<unsigned long>(startPos), static_cast<unsigned long>(endPos-startPos)));
                    } catch (const::std::exception e) {
                        qDebug() << "Could not parse number of deuteria from peptide sequence:" << peptideSequence;
                    }
                }

            }
            monoisotopicMass += numDeuteria * D2O_MASS_SHIFT;

            continue;
        }

        if (isFoundSecondDot) continue;

        if (isFoundFirstDot) {
            if (c == '.') { //second dot
                isFoundSecondDot = true;
                continue;
            } else {
                if (!Peptide::AAMonoisotopicMassTable) {
                    Peptide::defaultTables();
                }
                monoisotopicMass += Peptide::getAAMonoisotopicMass(c.toLatin1());
            }
        } else if (c == '.') {
            isFoundFirstDot = true;
        }
    }

    //This is necessary based on how AAMonoisotopicMassTable is computed
    monoisotopicMass += WATER_MONO;

    QVariant v = adductType->currentData();
    Adduct* currentAdduct = v.value<Adduct*>();

    float mz = currentAdduct->computeAdductMass(static_cast<float>(monoisotopicMass));

    if (eicWidget->isVisible() ){

        mzSlice updatedSlice(mz - mz/1e6f*static_cast<float>(getUserPPM()),
                             mz + mz/1e6f*static_cast<float>(getUserPPM()),
                             getEicWidget()->getMzSlice().rtmin,
                             getEicWidget()->getMzSlice().rtmax);

        eicWidget->setMzSlice(updatedSlice, false);
    }
    if (massCalcWidget->isVisible() ) massCalcWidget->setMass(mz);
    if (ms2ScansListWidget->isVisible()   ) showFragmentationScans(mz);
    if (ms2ConsensusScansListWidget->isVisible()) showConsensusFragmentationScans(mz);
    if (ms1ScansListWidget->isVisible() ) showMs1Scans(mz);

}

void MainWindow::setAdductFocus(Adduct *adduct) {

    if (!adduct) return;

    for (int i = 0; i < adductType->count(); i++) {
        QVariant v = adductType->itemData(i);
        Adduct*  itemAdduct =  v.value<Adduct*>();
        if (itemAdduct && itemAdduct->name == adduct->name) {
            this->adductType->setCurrentIndex(i);
            break;
        }
    }

    //Issue 376: adducts combo box in isotopeWidget should sync with main window adducts combo box
    if (isotopeWidget) {
        for (int i = 0; i < isotopeWidget->adductComboBox->count(); i++) {

            QVariant v = isotopeWidget->adductComboBox->itemData(i);
            Adduct*  itemAdduct =  v.value<Adduct*>();

            if (itemAdduct && itemAdduct->name == adduct->name) {
                isotopeWidget->adductComboBox->setCurrentIndex(i);
                break;
            }
        }
    }
}

void MainWindow::setCompoundFocus(Compound *c, bool isUpdateAdduct) {
    if (!c) return;

    /*
    int charge = 0;
    if (samples.size() > 0  && samples[0]->getPolarity() > 0) charge = 1;
    if (getIonizationMode()) charge=getIonizationMode(); //user specified ionization mode
    */

    Adduct* adduct = getUserAdduct();

    if (isUpdateAdduct && !c->adductString.empty()) {
        Adduct *compoundAdduct = DB.findAdductByName(c->adductString);
        if (compoundAdduct && compoundAdduct != adduct) {
            adduct = compoundAdduct;
            setAdductFocus(adduct);
        }
    }

    searchText->setText(c->name.c_str());

    float mz = adduct->computeAdductMass(c->getExactMass());

    qDebug() << "MainWindow::setCompoundFocus():" << c->name.c_str() << " " << adduct->name.c_str()
             << " " << c->expectedRt << " mass="
             << c->getExactMass() << " mz="
             << mz;

    if (eicWidget->isVisible() && samples.size() > 0 ){

        //Issue 347
        if (c->srmTransition) {

            eicWidget->setSelectedGroup(nullptr);
            eicWidget->setSRMTransition(c->srmTransition);
            eicWidget->resetZoom(true);

        } else {
            eicWidget->setCompound(c,adduct);
        }

    }

    if (isotopeWidget && isotopeWidget->isVisible() ) isotopeWidget->setCompound(c);
    if (massCalcWidget && massCalcWidget->isVisible() ) {
        if (eicWidget->getSelectedGroup()) {
            massCalcWidget->setPeakGroup(eicWidget->getSelectedGroup());
        } else {
            massCalcWidget->setMass(mz);
        }
    }
    //show fragmentation
    if (fragmentationSpectraWidget->isVisible())  {

        PeakGroup* group = eicWidget->getSelectedGroup();

        if (group && group->compound && group->compound == c) {

            fragmentationSpectraWidget->overlayPeakGroup(group);

            //Did not detect any MS2 scans
            if (!fragmentationSpectraWidget->getCurrentScan()) {
                fragmentationSpectraWidget->showMS2CompoundSpectrum(c);
            }

        } else {
            fragmentationSpectraWidget->showMS2CompoundSpectrum(c);
        }
    }

    if (c) setUrl(c);
}

void MainWindow::hideDockWidgets() {
  // setWindowState(windowState() ^ Qt::WindowFullScreen);
    QList<QDockWidget *> dockWidgets = this->findChildren<QDockWidget *>();
    for (int i = 0; i < dockWidgets.size(); i++) {
        dockWidgets[i]->hide();
    }
}

void MainWindow::showDockWidgets() {
    //setWindowState(windowState() ^ Qt::WindowFullScreen);
    QList<QDockWidget *> dockWidgets = this->findChildren<QDockWidget *>();
    for (int i = 0; i < dockWidgets.size(); i++) {
        if (! dockWidgets[i]->isVisible() ) dockWidgets[i]->show();
    }

    QWidget* menu = QMainWindow::menuWidget();
    if(menu) menu->show();
}

void MainWindow::doSearch(QString needle) {

    //TODO: relocate these
    QRegExp formula("(C?/d+)?(H?/d+)?(O?/d+)?(N?/d+)?(P?/d+)?(S?/d+)?",Qt::CaseInsensitive, QRegExp::RegExp);
    QRegExp peptideSequence("\\..*\\.",Qt::CaseInsensitive, QRegExp::RegExp);

    if (!needle.isEmpty()) {
        if (needle.contains(peptideSequence)) {
            setPeptideFocus(needle);
        }else if (needle.contains(formula)) {
            setFormulaFocus(needle);
        }
    }
}

void MainWindow::setMzValue() {
    bool isDouble =false;
    QString value = searchText->text();
    float mz = value.toDouble(&isDouble);
    if (isDouble) {
        if(eicWidget->isVisible() ) eicWidget->setMzSlice(mz);
        if(massCalcWidget->isVisible() ) massCalcWidget->setMass(mz);
        if(ms2ScansListWidget->isVisible()   ) showFragmentationScans(mz);
        if(ms2ConsensusScansListWidget->isVisible()) showConsensusFragmentationScans(mz);
        if(ms1ScansListWidget->isVisible() ) showMs1Scans(mz);
    }
    suggestPopup->addToHistory(QString::number(mz,'f',5));
}

void MainWindow::setMzValue(float mz) {
    searchText->setText(QString::number(mz,'f',8));
    if (eicWidget->isVisible() ) eicWidget->setMzSlice(mz);
    if (massCalcWidget->isVisible() ) massCalcWidget->setMass(mz);
    if (ms2ScansListWidget->isVisible()   ) showFragmentationScans(mz);
    if (ms2ConsensusScansListWidget->isVisible()) showConsensusFragmentationScans(mz);
    if (ms1ScansListWidget->isVisible() ) showMs1Scans(mz);
}

void MainWindow::print(){

    QPrinter printer;
    QPrintDialog dialog(&printer);

    if ( dialog.exec() ) {
        printer.setOrientation(QPrinter::Landscape);
        printer.setCreator("MAVEN Metabolics Analyzer");
        QPainter painter;
        if (! painter.begin(&printer)) { // failed to open file
            qWarning("failed to open file, is it writable?");
            return;
        }
        getEicWidget()->render(&painter);
        painter.end();
    }
}

void MainWindow::open(){

    QString dir = ".";

    if ( settings->contains("lastDir") ) {
        QString ldir = settings->value("lastDir").value<QString>();
        QDir test(ldir);
        if (test.exists()) dir = ldir;
    }

    QStringList filelist = QFileDialog::getOpenFileNames(
            this, "Select projects, peaks, samples to open:",
            dir,
                  tr("All Known Formats(*.mzroll *.mzrollDB *.mzPeaks *.mzXML *.mzxml *.mzdata *.mzData *.mzData.xml *.cdf *.nc *.mzML);;")+
                  tr("mzXML Format(*.mzXML *.mzxml);;")+
                  tr("mzData Format(*.mzdata *.mzData *.mzData.xml);;")+
                  tr("mzML Format(*.mzml *.mzML);;")+
                  tr("NetCDF Format(*.cdf *.nc);;")+
                  tr("Maven Project File (*.mzroll *.mzrollDB);;")+
                  tr("Maven Peaks File (*.mzPeaks);;")+
                  tr("Alignment File (*.rt);;")+
                  tr("All Files(*.*)"));

    if (filelist.size() == 0 ) return;

    QString absoluteFilePath(filelist[0]);
    QFileInfo fileInfo (absoluteFilePath);
    QDir tmp = fileInfo.absoluteDir();
    if ( tmp.exists()) settings->setValue("lastDir",tmp.absolutePath());

    QStringList samples;
    QStringList peaks;
    QStringList projects;
    QStringList alignmentfiles;

    foreach(QString filename, filelist ) {
        QFileInfo fileInfo(filename);
        if (!fileInfo.exists()) continue;

        if (isSampleFileType(filename)) {
            samples << filename;
        } else if (isProjectFileType(filename)) {
            projects << filename;
        } else if (filename.endsWith("mzpeaks",Qt::CaseInsensitive)) {
            peaks << filename;
        } else if (filename.endsWith(".rt",Qt::CaseInsensitive)) {
            alignmentfiles << filename;
        }
    }


    if (projects.size() > 0 ) {
        foreach(QString filename, projects) {
            if (filename.endsWith("mzroll"))    projectDockWidget->loadProjectXML(filename);
            if (filename.endsWith("mzrollDB")) {

                if (setRumsDBDialog){
                    delete(setRumsDBDialog);
                    setRumsDBDialog = nullptr;
                }

                //Case 344: only show dialog for mzkitchen files
                ProjectDB *projectDB = new ProjectDB(filename);
                QStringList tableNames = projectDB->getSearchTableNames();
                bool isMzkitchenFile = tableNames.contains(QString("rumsDB")) || tableNames.contains(QString("clamDB"));
                projectDB->closeDatabaseConnection();
                delete(projectDB);

                if (isMzkitchenFile) {
                    setRumsDBDialog  = new SetRumsDBDialog(this);
                    setRumsDBDialog->exec();

                    if (!setRumsDBDialog->isCancelled()) {
                        qDebug() << "rumsDB spectral library: " << rumsDBDatabaseName;
                        projectDockWidget->loadProjectSQLITE(filename);
                    }
                } else {
                    projectDockWidget->loadProjectSQLITE(filename);
                }

            }
        }
        return;
    }

    if ( samples.size() > 0 ) {
        mzFileIO* fileLoader  = new mzFileIO(this);
        fileLoader->setMainWindow(this);
        fileLoader->loadSamples(samples);
        fileLoader->start();
    }

    if (alignmentfiles.size()>0) {
        doGuidedAligment(alignmentfiles.first());
    }
}

void MainWindow::loadModel(){
    QStringList filelist = QFileDialog::getOpenFileNames( this, "Select Model To Load", ".", "All Files(*.model)");
    if ( filelist.size() > 0 )  clsf->loadModel(filelist[0].toStdString());
}
BackgroundPeakUpdate* MainWindow::newWorkerThread(QString funcName) {
    BackgroundPeakUpdate* workerThread = new BackgroundPeakUpdate(this);
    workerThread->setMainWindow(this);

    connect(workerThread, SIGNAL(updateProgressBar(QString,int,int)), SLOT(setProgressBar(QString, int,int)));
    workerThread->setRunFunction(funcName);
    //threads.push_back(workerThread);
    return workerThread;
}

/*
void MainWindow::terminateTheads() {

	for(int i=0; i < threads.size(); i++ ) {
		if (threads[i] != NULL ) {
			if (  threads[i]->isRunning())  {
				QMessageBox::StandardButton reply;
         		reply = QMessageBox::critical(this, tr(  "QMessageBox::critical()"), "Do you wish to stop currently running backround job?", QMessageBox::Yes | QMessageBox::No);
        		if (reply == QMessageBox::Yes) threads[i]->terminate();
			}
			if (! threads[i]->isRunning()) { delete(threads[i]); threads[i]=NULL; }
		}
	}
}


*/


void MainWindow::exportPDF(){

    const QString fileName = QFileDialog::getSaveFileName(
            this, "Export File Name", QString(),
            "PDF Documents (*.pdf)");

    if ( !fileName.isEmpty() )
    {
        QPrinter printer;
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOrientation(QPrinter::Landscape);
        printer.setOutputFileName(fileName);

        QPainter painter;
        if (! painter.begin(&printer)) { // failed to open file
            qWarning("failed to open file, is it writable?");
            return;
        }

        getEicWidget()->render(&painter);
        painter.end();
    }
}

void MainWindow::exportSVG(){
    QPixmap image(eicWidget->width()*2,eicWidget->height()*2);
    image.fill(Qt::white);
    //eicWidget->print(&image);
    QPainter painter;
    painter.begin(&image);
    getEicWidget()->render(&painter);
    painter.end();

    QApplication::clipboard()->setPixmap(image);
    statusBar()->showMessage("EIC Image copied to Clipboard");
}

void MainWindow::setStatusText(QString text){
    statusText->setText(text);
    //statusBar()->showMessage(text,500);
}

void MainWindow::setProgressBar(QString text, int progress, int totalSteps){
    setStatusText(text);

    if (progressBar->isVisible() == false && progress != totalSteps ) {
        progressBar->show();
        percentageText->show();
    }

    progressBar->setRange(0,totalSteps);
    progressBar->setValue(progress);

    int pct = static_cast<int>((100.0f * static_cast<float>(progress)/static_cast<float>(totalSteps)));
    percentageText->setText(QString::number(pct)+"%");

    if (progress == totalSteps ) {
        progressBar->hide();
        percentageText->hide();
    }
}

void MainWindow::readSettings() {
    settings = new QSettings("mzRoll", "Application Settings");

    qDebug() << "Settings file is located at:" << settings->fileName();

    QPoint pos = settings->value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings->value("size", QSize(400, 400)).toSize();

    if( ! settings->contains("scriptsFolder") )
        settings->setValue("scriptsFolder", "scripts");

    if( ! settings->contains("workDir") )
        settings->setValue("workDir",QString("."));

    if( ! settings->contains("methodsFolder") )
        settings->setValue("methodsFolder", "methods");

    if( ! settings->contains("ligandDbFilename") )
        settings->setValue("ligandDbFilename",QString("ligand.db"));

    if( ! settings->contains("clsfModelFilename") )
        settings->setValue("clsfModelFilename",QString("default.model"));

    if( ! settings->contains("grouping_maxRtWindow") )
        settings->setValue("grouping_maxRtWindow", 0.25);

    if( ! settings->contains("grouping_maxMzWindow") )
        settings->setValue("grouping_maxMzWindow", 100);

    if( ! settings->contains("eic_smoothingAlgorithm") )
        settings->setValue("eic_smoothingAlgorithm", EIC::SmootherType::GAUSSIAN);

    if( ! settings->contains("eic_smoothingWindow") )
        settings->setValue("eic_smoothingWindow", 5);

    if ( ! settings->contains("baseline_quantile"))
        settings->setValue("baseline_quantile", 80);

    if (! settings->contains("baseline_smoothing"))
        settings->setValue("baseline_smoothing", 5);

    if( ! settings->contains("eic_ppmWindow") )
        settings->setValue("eic_ppmWindow",100);

    if( ! settings->contains("ppmWindowBox") )
        settings->setValue("ppmWindowBox",5);

    if( ! settings->contains("mzslice") )
        settings->setValue("mzslice",QRectF(100.0,100.01,0,30));

    if( ! settings->contains("ionizationMode") )
        settings->setValue("ionizationMode",-1);

    //Issue 372: Do not check any isotopes by default
//    if( ! settings->contains("C13Labeled") )
//        settings->setValue("C13Labeled",2);

//    if( ! settings->contains("N15Labeled") )
//        settings->setValue("N15Labeled",2);

    if( ! settings->contains("isotopeC13Correction") )
        settings->setValue("isotopeC13Correction",2);

    if( ! settings->contains("maxNaturalAbundanceErr") )
        settings->setValue("maxNaturalAbundanceErr",100);

    if( ! settings->contains("maxIsotopeScanDiff") )
        settings->setValue("maxIsotopeScanDiff",5);

    if( ! settings->contains("minIsotopicCorrelation") )
        settings->setValue("minIsotopicCorrelation",0.2);

    //Issue 451
    if (! settings->contains("chkNotifyNewerMaven")) {
        settings->setValue("chkNotifyNewerMaven", Qt::CheckState::Checked);
    }

    if ( settings->contains("lastOpenedProject")) {
        settings->setValue("lastOpenedProject","");
    }


    resize(size);
    move(pos);
}

void MainWindow::writeSettings() {
    settings->setValue("pos", pos());
    settings->setValue("size", size());
    settings->setValue("ppmWindowBox",ppmWindowBox->value());
    settings->setValue("geometry", saveGeometry());

    //Issue 479: Always hide the MS1 spectrum widget, as it can be very slow to render
    spectraDockWidget->setVisible(false);

    settings->setValue("windowState", saveState());
    settings->setValue("ionizationMode", getIonizationMode());


    mzSlice slice = eicWidget->getMzSlice();
    settings->setValue("mzslice",QRectF(slice.mzmin,slice.mzmax,slice.rtmin,slice.rtmax));

    if ( suggestPopup ) {
        QMap<QString,int>history = suggestPopup->getHistory();
        foreach(QString key, history.keys()) {
            if ( history[key] > 1 ) {
                settings->setValue("searchHistory/"+ key, history[key]);
            }
        }
    }

    qDebug() << "Settings saved to " << settings->fileName();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (QMessageBox::Yes == QMessageBox::question(this, "Close Confirmation", "Exit Maven?", QMessageBox::Yes | QMessageBox::No)) {

        if (QMessageBox::Yes == QMessageBox::question(this, "Close Confirmation", "Do you want to save current project?", QMessageBox::Yes | QMessageBox::No)) {
            projectDockWidget->saveProject();
        }

        //Issue 52
        //avoid some strange bugs when shutting down, specifically a call to updateDatabaseList().
        massCalcWidget->blockSignals(true);

        settings->setValue("closeEvent", 1 );
        writeSettings();
        DB.closeAll();
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::createMenus() {

    //Issue 558
    QMenu *aboutMenu = menuBar()->addMenu(tr("&About"));

    QAction* reference = new QAction("Reference", this);
    connect(reference, SIGNAL(triggered()), this, SLOT(openReference()));
    aboutMenu->addAction(reference);

    QAction *tutorialVideo = new QAction("Tutorial Video", this);
    connect(tutorialVideo, SIGNAL(triggered()), this, SLOT(openTutorialVideo()));
    aboutMenu->addAction(tutorialVideo);

    QAction *reportBug = new QAction("Report Bug", this);
    connect(reportBug, SIGNAL(triggered()), this, SLOT(reportBugs()));
    aboutMenu->addAction(reportBug);

    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    QMenu* widgetsMenu =  menuBar()->addMenu(tr("&Widgets"));

    QAction* openAct = new QAction("Load Samples|Projects|Peaks", this);
    openAct->setShortcut(tr("Ctrl+O"));
    openAct->setToolTip(tr("Open an existing file"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));
    fileMenu->addAction(openAct);

    QAction* loadModel = new QAction(tr("Load Classification Model"), this);
    connect(loadModel, SIGNAL(triggered()), SLOT(loadModel()));
    fileMenu->addAction(loadModel);

    QAction* loadCompoundsFile = new QAction(tr("Load Compound List"), this);
    connect(loadCompoundsFile, SIGNAL(triggered()), libraryDialog, SLOT(loadCompoundsFile()));
    fileMenu->addAction(loadCompoundsFile);

    QAction* saveProjectFile = new QAction(tr("Save Project"), this);
    saveProjectFile->setShortcut(tr("Ctrl+S"));
    connect(saveProjectFile, SIGNAL(triggered()), projectDockWidget, SLOT(saveProject()));
    fileMenu->addAction(saveProjectFile);

    QAction* saveProjectFileAs = new QAction(tr("Save Project As"), this);
    saveProjectFileAs->setShortcut(tr("Ctrl+Shift+S"));
    connect(saveProjectFileAs, SIGNAL(triggered()), projectDockWidget, SLOT(saveProjectAs()));
    fileMenu->addAction(saveProjectFileAs);

    QAction* settingsAct = new QAction(tr("Options"), this);
    settingsAct->setToolTip(tr("Set program options"));
    connect(settingsAct, SIGNAL(triggered()), settingsForm, SLOT(show()));
    fileMenu->addAction(settingsAct);

    QAction* exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcut(tr("Ctrl+Q"));
    exitAct->setToolTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));
    fileMenu->addAction(exitAct);

    //Issue 144: Reorganize menu contents
    QAction *actionCalibrate = widgetsMenu->addAction(QIcon(rsrcPath + "/positive_ion.png"), "PPM Calibrate");
    actionCalibrate->setToolTip("Calibrate");
    connect(actionCalibrate,SIGNAL(triggered()), calibrateDialog, SLOT(show()));

    QAction *actionAlign = widgetsMenu->addAction(QIcon(rsrcPath + "/textcenter.png"), "Sample Alignment");
    actionAlign->setToolTip("Align Samples");
    connect(actionAlign, SIGNAL(triggered()), alignmentDialog, SLOT(show()));

    QAction *actionMatch = widgetsMenu->addAction(QIcon(rsrcPath + "/spectra_search.png"), "Nearest Spectra Search");
    actionMatch->setToolTip(tr("Search Spectra for Fragmentation Patterns"));
    connect(actionMatch, SIGNAL(triggered()), spectraMatchingForm, SLOT(show()));

    QAction *actionDatabases = widgetsMenu->addAction(QIcon(rsrcPath + "/dbsearch.png"), "Library Search");
    actionDatabases->setToolTip("Library Search");
    connect(actionDatabases, SIGNAL(triggered()), SLOT(compoundDatabaseSearch()));

//    QAction *actionRconsole = widgetsMenu->addAction(QIcon(rsrcPath + "/R.png"), "R console");
//    actionRconsole->setCheckable(true);
//    actionRconsole->setChecked(false);
//    actionRconsole->setToolTip("R Console");
//    connect(actionRconsole, SIGNAL(triggered(bool)), rconsoleDockWidget, SLOT(setVisible(bool)));

    widgetsMenu->addSeparator();

    QAction *aMs1Events = widgetsMenu->addAction("MS1 Scans List");
    aMs1Events->setCheckable(true);
    aMs1Events->setChecked(false);
    connect(aMs1Events, SIGNAL(toggled(bool)), ms1ScansListWidget, SLOT(setVisible(bool)));

    QAction* aj = widgetsMenu->addAction("MS2 Scans List");
    aj->setCheckable(true);
    aj->setChecked(false);
    connect(aj,SIGNAL(toggled(bool)), ms2ScansListWidget,SLOT(setVisible(bool)));

    QAction *aMs3Events = widgetsMenu->addAction("MS3 Scans List");
    aMs3Events->setCheckable(true);
    aMs3Events->setChecked(false);
    connect(aMs3Events, SIGNAL(toggled(bool)), ms3ScansListWidget, SLOT(setVisible(bool)));

    widgetsMenu->addSeparator();

    QAction *aConsensusMs2Events =widgetsMenu->addAction("Consensus MS2 Scans List");
    aConsensusMs2Events->setCheckable(true);
    aConsensusMs2Events->setChecked(false);
    connect(aConsensusMs2Events, SIGNAL(toggled(bool)), ms2ConsensusScansListWidget,SLOT(setVisible(bool)));

    QAction *SRMListAction = widgetsMenu->addAction("SRM List");
    SRMListAction->setCheckable(true);
    SRMListAction->setChecked(false);

    connect(SRMListAction, SIGNAL(clicked(bool)), SLOT(showSRMListCached()));

    SRMTransitionListAction = widgetsMenu->addAction("SRM Transition List");
    SRMTransitionListAction->setCheckable(true);
    SRMTransitionListAction->setChecked(false);

    connect(SRMTransitionListAction, SIGNAL(clicked(bool)), SLOT(showSRMListCached()));
    connect(SRMTransitionListAction, SIGNAL(toggled(bool)), btnSRM, SLOT(setChecked(bool)));

    connect(SRMListAction, SIGNAL(toggled(bool)), srmDockWidget, SLOT(setVisible(bool)));
    connect(SRMTransitionListAction, SIGNAL(toggled(bool)), srmTransitionDockWidget, SLOT(setVisible(bool)));

    connect(libraryDialog, SIGNAL(loadLibrarySignal(QString)), SLOT(showSRMList()));
    connect(libraryDialog, SIGNAL(afterUnloadedLibrarySignal(QString)), SLOT(showSRMList()));

    widgetsMenu->addSeparator();

    QAction* hideWidgets = new QAction(tr("Hide Widgets"), this);
    hideWidgets->setShortcut(tr("F11"));
    connect(hideWidgets, SIGNAL(triggered()), SLOT(hideDockWidgets()));
    widgetsMenu->addAction(hideWidgets);

    menuBar()->show();
}

QToolButton* MainWindow::addDockWidgetButton( QToolBar* bar, QDockWidget* dockwidget, QIcon icon, QString description) {
    QToolButton *btn = new QToolButton(bar);
    btn->setCheckable(true);
    btn->setIcon(icon);
    btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    btn->setToolTip(description);
    connect(btn,SIGNAL(clicked(bool)),dockwidget, SLOT(setVisible(bool)));
    connect(btn,SIGNAL(clicked(bool)),dockwidget, SLOT(raise()));
    btn->setChecked( dockwidget->isVisible() );
    connect(dockwidget,SIGNAL(visibilityChanged(bool)),btn,SLOT(setChecked(bool)));
    dockwidget->setWindowIcon(icon);
    return btn;
}

void MainWindow::createToolBars() {

    QToolBar *toolBar = new QToolBar(this);
    toolBar->setObjectName("mainToolBar");
    toolBar->setMovable(false);

    QToolButton *btnSave = new QToolButton(toolBar);
    btnSave->setText("Save");
    btnSave->setIcon(QIcon(rsrcPath +"/filesave.png"));
    btnSave->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnSave->setToolTip(tr("Save Project"));

    QToolButton *btnOpen = new QToolButton(toolBar);
    btnOpen->setText("Open");
    btnOpen->setIcon(QIcon(rsrcPath + "/fileopen.png"));
    btnOpen->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnOpen->setToolTip(tr("Read in Mass Spec Files"));

    //Issue 246: made public for enabling logic.
    btnLibrary = new QToolButton(toolBar);
    btnLibrary->setText("Library");
    btnLibrary->setIcon(QIcon(rsrcPath + "/librarymanager.png"));
    btnLibrary->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnLibrary->setToolTip(tr("Library Manager"));

    QToolButton *btnAdducts = new QToolButton(toolBar);
    btnAdducts->setText("Adducts");
    btnAdducts->setIcon(QIcon(rsrcPath + "/adducts.png"));
    btnAdducts->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnAdducts->setToolTip(tr("Select Active Adducts"));

    QToolButton *btnFeatureDetect = new QToolButton(toolBar);
    btnFeatureDetect->setText("Peaks");
    btnFeatureDetect->setIcon(QIcon(rsrcPath + "/featuredetect.png"));
    btnFeatureDetect->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnFeatureDetect->setToolTip(tr("Feature Detection"));

    QToolButton *btnDirectInfusion = new QToolButton(toolBar);
    btnDirectInfusion->setText("Direct Infusion");
    btnDirectInfusion->setIcon(QIcon(rsrcPath +"/averageSpectra.png"));
    btnDirectInfusion->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnDirectInfusion->setToolTip(tr("Process Direct Infusion Samples"));

    QToolButton *btnSettings = new QToolButton(toolBar);
    btnSettings->setText("Options");
    btnSettings->setIcon(QIcon(rsrcPath + "/settings.png"));
    btnSettings->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnSettings->setToolTip(tr("Change Global Options"));

    connect(btnOpen, SIGNAL(clicked()), SLOT(open()));
    connect(btnSave, SIGNAL(clicked()), projectDockWidget, SLOT(saveProject()));
    connect(btnLibrary, SIGNAL(clicked()), libraryDialog, SLOT(show()));
    connect(btnAdducts, SIGNAL(clicked()), SLOT(showSelectAdductsDialog()));
    connect(btnFeatureDetect,SIGNAL(clicked()), SLOT(showMassSlices()));
    connect(btnDirectInfusion, SIGNAL(clicked()), SLOT(showDirectInfusionDialog()));
    connect(btnSettings,SIGNAL(clicked()),settingsForm,SLOT(bringIntoView()));

    toolBar->addWidget(btnOpen);
    toolBar->addWidget(btnSave);
    toolBar->addWidget(btnLibrary);
    toolBar->addWidget(btnAdducts);
    toolBar->addWidget(btnFeatureDetect);
    toolBar->addWidget(btnDirectInfusion);
    toolBar->addWidget(btnSettings);

    QWidget *hBox = new QWidget(toolBar);
    (void)toolBar->addWidget(hBox);

    QHBoxLayout *layout = new QHBoxLayout(hBox);
    layout->setSpacing(0);
    layout->addWidget(new QWidget(hBox), 15); // spacer

    //ppmValue
    ppmWindowBox = new QDoubleSpinBox(hBox);
    ppmWindowBox->setRange(0.00, 100000.0);
    ppmWindowBox->setValue(settings->value("ppmWindowBox").toDouble());
    ppmWindowBox->setSingleStep(0.5);	//ppm step
    ppmWindowBox->setToolTip("PPM (parts per million) Window");
    connect(ppmWindowBox, SIGNAL(valueChanged(double)),this , SLOT(setUserPPM(double)));
    connect(ppmWindowBox, SIGNAL(valueChanged(double)), eicWidget, SLOT(setPPM(double)));

    searchText = new QLineEdit(hBox);
    searchText->setMinimumWidth(400);
    searchText->setToolTip("<b>Text Search</b> <br> Compound Names: <b>ATP</b>,<br> Patterns: <b>[45]-phosphate</b> <br>Formulas: <b> C6H10* </b>");
    searchText->setObjectName(QString::fromUtf8("searchText"));
    searchText->setShortcutEnabled(true);
    connect(searchText,SIGNAL(textEdited(QString)),this,SLOT(doSearch(QString)));
    connect(searchText,SIGNAL(returnPressed()), SLOT(setMzValue()));

    QShortcut* ctrlK = new QShortcut(QKeySequence(tr("Ctrl+K", "Do Search")), this);
    QShortcut* ctrlF = new QShortcut(QKeySequence(tr("Ctrl+F", "Do Search")), this);
    connect(ctrlK,SIGNAL(activated()),searchText,SLOT(selectAll()));
    connect(ctrlK,SIGNAL(activated()),searchText,SLOT(setFocus()));
    connect(ctrlF,SIGNAL(activated()),searchText,SLOT(selectAll()));
    connect(ctrlF,SIGNAL(activated()),searchText,SLOT(setFocus()));


    suggestPopup = new SuggestPopup(searchText);
    connect(suggestPopup,SIGNAL(compoundSelected(Compound*)),this,SLOT(setCompoundFocus(Compound*)));
    connect(ligandWidget,SIGNAL(databaseChanged(QString)),suggestPopup,SLOT(setDatabase(QString)));
    layout->addSpacing(10);

    quantType = new QComboBox(hBox);
    quantType->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    //Issue 285: bookmarkedPeaks is created before quantType is initialized, need to connect bookmarks table manually.
    //All other peak groups tables will be connected in the TableDockWidget() constructor.

    quantType->addItem("AreaTop");
    quantType->addItem("Area");
    quantType->addItem("Height");
    quantType->addItem("Retention Time");
    quantType->addItem("Quality");
    quantType->addItem("Area Uncorrected");
    quantType->addItem("Area Fractional");
    quantType->addItem("S/N Ratio");
    quantType->addItem("Smoothed Height");
    quantType->addItem("Smoothed Area");
    quantType->addItem("Smoothed Area Uncorrected");
    quantType->addItem("Smoothed AreaTop");
    quantType->addItem("Smoothed S/N Ratio");
    quantType->addItem("FWHM Area");
    quantType->addItem("FWHM Smoothed Area");

    quantType->setToolTip("Peak Quantitation Type");
    connect(quantType, SIGNAL(activated(int)), eicWidget, SLOT(replot()));
    connect(quantType, SIGNAL(currentIndexChanged(int)), bookmarkedPeaks, SLOT(refreshPeakGroupQuant()));

    adductType = new QComboBox(hBox);
    adductType->setMinimumSize(250, 0);
    updateAdductsInGUI();
    connect(adductType,SIGNAL(currentIndexChanged(QString)),SLOT(changeUserAdduct()));

    settings->beginGroup("searchHistory");
    QStringList keys = settings->childKeys();
    foreach(QString key, keys) suggestPopup->addToHistory(key, settings->value(key).toInt());
    settings->endGroup();

    layout->addWidget(quantType, 0);
    layout->addWidget(new QLabel("[m/z]", hBox), 0);
    layout->addWidget(searchText, 0);
    layout->addWidget(adductType, 0);
    layout->addWidget(new QLabel("+/-",0,0));
    layout->addWidget(ppmWindowBox, 0);


    sideBar = new QToolBar(this);
    sideBar->setObjectName("sideBar");


    QToolButton* btnSamples = addDockWidgetButton(sideBar,projectDockWidget,QIcon(rsrcPath + "/samples.png"), "Samples (F2)");
    QToolButton* btnLigands = addDockWidgetButton(sideBar,ligandWidget,QIcon(rsrcPath + "/molecule.png"), "Compound Library (F3)");
    QToolButton* btnSpectra = addDockWidgetButton(sideBar,spectraDockWidget,QIcon(rsrcPath + "/spectra.png"), "MS1 Spectra (F4)");
    QToolButton* btnFragSpectra = addDockWidgetButton(sideBar, fragmentationSpectraDockWidget, QIcon(rsrcPath + "/spectra_with_2.png"), "MS2 Spectra");
    QToolButton* btnMs3Spectra = addDockWidgetButton(sideBar, ms3SpectraDockWidget, QIcon(rsrcPath + "/spectra_with_3.png"), "MS3 Spectra");
    QToolButton* btnIsotopes = addDockWidgetButton(sideBar,isotopeWidget,QIcon(rsrcPath + "/isotope.png"), "Isotopes Widget (F5)");
    QToolButton* btnFindCompound = addDockWidgetButton(sideBar,massCalcWidget,QIcon(rsrcPath + "/findcompound.png"), "Compound Search (F6)");
    QToolButton* btnCovariants = addDockWidgetButton(sideBar,covariantsPanel,QIcon(rsrcPath + "/covariants.png"), "Covariants Widget (F7)");
    QToolButton* btnBookmarks = addDockWidgetButton(sideBar,bookmarkedPeaks,QIcon(rsrcPath + "/showbookmarks.png"), "Bookmarks (F10)");
    QToolButton* btnGallery = addDockWidgetButton(sideBar,galleryDockWidget,QIcon(rsrcPath + "/gallery.png"), "Gallery");
    QToolButton* btnScatter = addDockWidgetButton(sideBar,scatterDockWidget,QIcon(rsrcPath + "/scatterplot.png"), "Scatter Plot");

    btnSRM = addDockWidgetButton(sideBar,srmTransitionDockWidget,QIcon(rsrcPath + "/qqq.png"), "SRM List (F12)");

    //QToolButton* btnRconsole = addDockWidgetButton(sideBar,rconsoleDockWidget,QIcon(rsrcPath + "/R.png"), "Show R Console");
    QToolButton* btnBarPlot = addDockWidgetButton(sideBar,barPlotDockWidget, QIcon(rsrcPath + "/bar_plot_samples.png"), "EIC Legend");
    QToolButton* btnIsotopeLegend = addDockWidgetButton(sideBar, isotopeLegendDockWidget, QIcon(rsrcPath +"/isotope_legend.png"), "Isotope Bar Plot");

    btnSamples->setShortcut(Qt::Key_F2);
    btnLigands->setShortcut(Qt::Key_F3);
    btnSpectra->setShortcut(Qt::Key_F4);
    btnIsotopes->setShortcut(Qt::Key_F5);
    btnFindCompound->setShortcut(Qt::Key_F6);
    btnCovariants->setShortcut(Qt::Key_F7);
    btnBookmarks->setShortcut(Qt::Key_F10);
    btnSRM->setShortcut(Qt::Key_F12);

    connect(btnSRM,SIGNAL(clicked(bool)),SLOT(showSRMListCached()));
    connect(btnSRM,SIGNAL(clicked(bool)), SRMTransitionListAction, SLOT(setChecked(bool)));

    sideBar->setOrientation(Qt::Vertical);
    sideBar->setMovable(false);

    sideBar->addWidget(btnSamples);
    sideBar->addWidget(btnBarPlot);
    sideBar->addWidget(btnIsotopeLegend);
    sideBar->addWidget(btnLigands);
    sideBar->addWidget(btnSpectra);
    sideBar->addWidget(btnFragSpectra);
    sideBar->addWidget(btnMs3Spectra);
    sideBar->addWidget(btnIsotopes);
    sideBar->addWidget(btnFindCompound);
    sideBar->addWidget(btnCovariants);
    sideBar->addWidget(btnSRM);
    sideBar->addWidget(btnGallery);
    sideBar->addWidget(btnScatter);
    //sideBar->addWidget(btnRconsole);
    sideBar->addSeparator();
    sideBar->addWidget(btnBookmarks);
    // sideBar->addWidget(btnHeatmap);

    addToolBar(Qt::TopToolBarArea,toolBar);
    addToolBar(Qt::RightToolBarArea,sideBar);
}

//Issue 423: update based on currentProject, or Projects being unloaded.
void MainWindow::updateQuantTypeComboBox() {
    quantType->clear();

    vector<string> quantTypes{"AreaTop",
                              "Area",
                              "Height",
                              "Retention Time",
                              "Quality",
                              "Area Uncorrected",
                              "Area Fractional",
                              "S/N Ratio",
                              "Smoothed Height",
                              "Smoothed Area",
                              "Smoothed Area Uncorrected",
                              "Smoothed AreaTop",
                              "Smoothed S/N Ratio",
                              "FWHM Area",
                              "FWHM Smoothed Area"
                             };

    for (auto quantTypeString : quantTypes) {
        if (projectDockWidget->currentProject) {
            if (projectDockWidget->currentProject->quantTypeMap.find(quantTypeString) != projectDockWidget->currentProject->quantTypeMap.end()) {
                quantType->addItem(projectDockWidget->currentProject->quantTypeMap[quantTypeString].c_str());
            } else {
                quantType->addItem(quantTypeString.c_str());
            }
        } else {
            quantType->addItem(quantTypeString.c_str());
        }
    }
}

void MainWindow::historyLast() {
    if(history.size()==0) return;
    eicWidget->setMzSlice(history.last());
}

void MainWindow::historyNext() {
    if(history.size()==0) return;
    eicWidget->setMzSlice(history.next());
}

void MainWindow::addToHistory(const mzSlice& slice) {
    history.addToHistory(slice);
}

bool MainWindow::addSample(mzSample* sample) {
    if ( sample && sample->scans.size() > 0 ) {
        sample->setSampleOrder(samples.size());
        samples.push_back(sample);
        if(sample->getPolarity()) setIonizationMode(sample->getPolarity());
        return true;
    } else {
        delete(sample);
        return false;
    }
}

void MainWindow::showMassSlices() {
    peakDetectionDialog->setFeatureDetection( PeakDetectionDialog::FullSpectrum );
    peakDetectionDialog->show();
}

void MainWindow::showDirectInfusionDialog() {
    qDebug() << "MainWindow::showDirectInfusionDialog()";
    directInfusionDialog->show();
}

void MainWindow::showSelectAdductsDialog() {
    qDebug() << "MainWindow::showSelectAdductsDialog()";

    if (selectAdductsDialog->isVisible()){
        selectAdductsDialog->show();
        selectAdductsDialog->raise();
        selectAdductsDialog->activateWindow();
    } else {
        selectAdductsDialog->show();
    }
}

void MainWindow::compoundDatabaseSearch() {
    peakDetectionDialog->setFeatureDetection(PeakDetectionDialog::CompoundDB);
    peakDetectionDialog->show();
}

void MainWindow::showSRMListCached() {
    showSRMList(false);
}

void MainWindow::showSRMList(bool recalculateTransitions) {

    //Issue 591: Option to avoid excessive recomputations
    if (recalculateTransitions || srmTransitions.empty()) {
         srmTransitions = getSRMTransitions();

         if (srmTransitions.empty()) return;

         set<string> srmIds = QQQProcessor::getSRMIds(srmTransitions);
         srmDockWidget->setInfo(srmIds);

         srmTransitionDockWidget->setInfo(srmTransitions);
    }

// Issue 564: Trade dangling pointer issues for memory leaks

//    delete_all(slices.first);
//    delete_all(slices.second);

//    slices.first.clear();
//    slices.second.clear();
    //peakDetectionDialog->setFeatureDetection(PeakDetectionDialog::QQQ);
    //peakDetectionDialog->show();
}

void MainWindow::setMs3PeakGroup(PeakGroup* parentGroup, PeakGroup* childGroup) {
    if (!parentGroup) return;
    if (!childGroup) return;

    _lastSelectedPeakGroup = parentGroup;

    qDebug() << "MainWindow::setMs3PeakGroup() parent=" << parentGroup << "child=" << childGroup;

    if (ms3ScansListWidget->isVisible()) {
        ms3ScansListWidget->setInfo(static_cast<double>(parentGroup->meanMz), static_cast<double>(childGroup->meanMz));
    }

    if ( eicWidget && eicWidget->isVisible() ) {
        eicWidget->setPeakGroup(parentGroup);
    }

    if (parentGroup->compound) {
        setUrl(parentGroup->compound);

        if (ms3SpectraWidget->isVisible()) {

            Ms3Compound ms3Compound(parentGroup->compound);

            int ms2MzKey = static_cast<int>(mzUtils::mzToIntKey(static_cast<double>(childGroup->meanMz)));

            //avoid possible rounding problems by identifying index from map based on closest value
            int mzKeyFromMap = -1;
            int smallestDiff = INT_MAX;

            for (auto it = ms3Compound.ms3_fragment_mzs.begin(); it != ms3Compound.ms3_fragment_mzs.end(); ++it) {
                int key = it->first;
                int diff = abs(ms2MzKey-key);
                if (diff < smallestDiff) {
                    smallestDiff = diff;
                    mzKeyFromMap = key;
                }
            }

            vector<float> fragment_mzs = ms3Compound.ms3_fragment_mzs[mzKeyFromMap];
            vector<string> fragment_labels = ms3Compound.ms3_fragment_labels[mzKeyFromMap];
            vector<float> fragment_intensities = ms3Compound.ms3_fragment_intensity[mzKeyFromMap];

            string compoundName = parentGroup->compound->name + " " + childGroup->tagString;

            Compound *cpd = new Compound(compoundName, compoundName,parentGroup->compound->getFormula(), 0, parentGroup->compound->getExactMass());
            cpd->db = parentGroup->compound->db;
            cpd->precursorMz = parentGroup->compound->precursorMz;
            cpd->adductString = parentGroup->compound->adductString;
            cpd->fragment_mzs = fragment_mzs;
            cpd->fragment_intensity = fragment_intensities;
            cpd->fragment_labels = fragment_labels;

            ms3SpectraWidget->overlayCompound(cpd);

            delete(cpd);

        }
    }

}

void MainWindow::setPeakGroup(PeakGroup* group) {
    qDebug() << "MainWindow::setPeakGroup(PeakGroup)" << group;

    if (!group) return;

    //Issue 371: More complex case for equality: isotopic quant versions
    //for now, try always updating the peak group to avoid tricky equality cases

//    //Issue 373: Compare values pointed to, not the pointers themselves (implementation-defined behavior)
//    if (_lastSelectedPeakGroup && *_lastSelectedPeakGroup == *group){

//        bool isEqual = true;

//        //Also need to check the compounds and adducts
//        if (_lastSelectedPeakGroup->compound && group->compound && _lastSelectedPeakGroup->compound->id != group->compound->id) {
//            isEqual = false;
//        }

//        if (_lastSelectedPeakGroup->adduct && group->adduct && _lastSelectedPeakGroup->adduct->name != group->adduct->name) {
//            isEqual = false;
//        }

//        if (isEqual) return;
//    }

    _lastSelectedPeakGroup = group;

    updateGUIWithLastSelectedPeakGroup();

}

void MainWindow::updateGUIWithLastSelectedPeakGroup(){
    qDebug() << "MainWindow::updateGUIWithLastSelectedPeakGroup()";

    if (!_lastSelectedPeakGroup) return;

    bool isDisplayConsensusSpectrum = settings->value("chkDisplayConsensusSpectrum", false).toBool();

    rconsoleDockWidget->updateStatus();

    searchText->setText(QString::number(static_cast<double>(_lastSelectedPeakGroup->meanMz),'f',8));

    //no need to compute consensus spectrum if it won't be displayed.
    if (isDisplayConsensusSpectrum) {

        //Issue 550
        qDebug() << "MainWindow::updateGUIWithLastSelectedPeakGroup(): Drawing consensus MS2 spectrum.";

        if (_lastSelectedPeakGroup->fragmentationPattern.nobs() == 0) {
            //if saving a loaded fragmentation file, this should all be stored from the file.

            if (getProjectWidget()->currentProject) {
                if (getProjectWidget()->currentProject->diSearchParameters.find(_lastSelectedPeakGroup->searchTableName) != getProjectWidget()->currentProject->diSearchParameters.end()) {

                    shared_ptr<DirectInfusionSearchParameters> params = getProjectWidget()->currentProject->diSearchParameters[_lastSelectedPeakGroup->searchTableName];

                    _lastSelectedPeakGroup->computeDIFragPattern(params);

                } else if (getProjectWidget()->currentProject->peaksSearchParameters.find(_lastSelectedPeakGroup->searchTableName) != getProjectWidget()->currentProject->peaksSearchParameters.end()) {

                    shared_ptr<PeaksSearchParameters> params = getProjectWidget()->currentProject->peaksSearchParameters[_lastSelectedPeakGroup->searchTableName];

                    _lastSelectedPeakGroup->computePeaksSearchFragPattern(params);
                }
            }
        }

        //last resort
        //Issue 311: if the mz width is greater than or equal to half a Da, assume DI sample (and do not compute frag pattern)
        if (_lastSelectedPeakGroup->fragmentationPattern.nobs() == 0 && (_lastSelectedPeakGroup->maxMz - _lastSelectedPeakGroup->minMz < 0.5f)) {
            _lastSelectedPeakGroup->computeFragPattern(static_cast<float>(massCalcWidget->fragmentPPM->value()));
        }
    }

    if ( eicWidget && eicWidget->isVisible() ) {
        eicWidget->setPeakGroup(_lastSelectedPeakGroup);
    }

    if ( isotopeWidget && isotopeWidget->isVisible() && _lastSelectedPeakGroup->compound ) {
        isotopeWidget->setCompound(_lastSelectedPeakGroup->compound);
    }

    if (_lastSelectedPeakGroup->compound) {
        setUrl(_lastSelectedPeakGroup->compound);
    }

    if(fragmentationSpectraWidget->isVisible()) {
        fragmentationSpectraWidget->overlayPeakGroup(_lastSelectedPeakGroup);
    }

    if (massCalcWidget->isVisible()) {
        massCalcWidget->setPeakGroup(_lastSelectedPeakGroup);
        massCalcWidget->lineEdit->setText(QString::number(static_cast<double>(_lastSelectedPeakGroup->meanMz),'f',5));
    }

    if (_lastSelectedPeakGroup->peaks.empty()) return;

    Peak *_peak = &(_lastSelectedPeakGroup->peaks[0]);

    showPeakInfo(_peak);

}

void MainWindow::Align() {
    if (sampleCount() < 2 ) return;

    BackgroundPeakUpdate* workerThread = newWorkerThread("processMassSlices");
    connect(workerThread, SIGNAL(finished()),eicWidget,SLOT(replotForced()));
    connect(workerThread, SIGNAL(started()),alignmentDialog,SLOT(close()));


    if (settings != NULL ) {
         workerThread->eic_ppmWindow = settings->value("eic_ppmWindow").toDouble();
         workerThread->eic_smoothingAlgorithm = settings->value("eic_smoothingWindow").toInt();
    }

    workerThread->minGoodPeakCount = alignmentDialog->minGoodPeakCount->value();
    workerThread->limitGroupCount =  alignmentDialog->limitGroupCount->value();
    workerThread->minGroupIntensity =  alignmentDialog->minGroupIntensity->value();
    workerThread->eic_smoothingWindow = alignmentDialog->groupingWindow->value();
    workerThread->mustHaveMS2 = alignmentDialog->mustHaveMS2Events->isChecked();

    workerThread->minSignalBaseLineRatio = alignmentDialog->minSN->value();
    workerThread->minNoNoiseObs = alignmentDialog->minPeakWidth->value();
    workerThread->alignSamplesFlag=true;
    workerThread->keepFoundGroups=false;
    workerThread->eicMaxGroups=5;
    workerThread->start();
}

void MainWindow::doGuidedAligment(QString alignmentFile) {
    if (samples.size() <= 1) return;

    for(int i=0; i < samples.size(); i++ ) {
        if (samples[i]) samples[i]->saveOriginalRetentionTimes();
    }

    if (alignmentFile.length()) {
        cerr << "Aligning samples" << "\t" << alignmentFile.toStdString() << endl;
        Aligner aligner;
        aligner.setSamples(samples);
        aligner.loadAlignmentFile(alignmentFile.toStdString().c_str());
        aligner.doSegmentedAligment();
    }

    getEicWidget()->replotForced();
}

void MainWindow::UndoAlignment() {
    for(int i=0; i < samples.size(); i++ ) {
        if (samples[i])
          samples[i]->restoreOriginalRetentionTimes();
    }
    getEicWidget()->replotForced();
}

vector<SRMTransition*> MainWindow::getSRMTransitions() {

    shared_ptr<QQQSearchParameters> params = shared_ptr<QQQSearchParameters>(new QQQSearchParameters());

    //Issue 564: If this becomes a problem, this can always be changed back.
    //The strictness is necessary to deal with crappy transitions

    params->amuQ1 = 0.01f;//getSettings()->value("amuQ1").toFloat();
    params->amuQ3 = 0.01f;//getSettings()->value("amuQ3").toFloat();

    vector<mzSample*> visibleSamples = getSamples();

    vector<SRMTransition*> transitions = QQQProcessor::getSRMTransitions(
                visibleSamples,
                params,
                DB.compoundsDB,
                DB.adductsDB,
                false
                );

    return(transitions);
}


//callers are setPeakGroup() and EICPoint::peakSelected signal
void MainWindow::showPeakInfo(Peak* _peak) {
    qDebug() << "MainWindow::showPeakInfo(peak)";

    if (!_peak) return;

    mzSample* sample = _peak->getSample();
    if (!sample) return;

    Scan* scan = sample->getScan(_peak->scan);
    if (!scan) return;

    int ionizationMode = scan->getPolarity();
    if (getIonizationMode()) ionizationMode=getIonizationMode(); //user specified ionization mode

    if (spectraDockWidget->isVisible() && scan) {
        spectraWidget->setScan(_peak);
    }

    if( ms2ScansListWidget->isVisible() ) {
        float mz = _peak->peakMz;
        if (_peak->mzmax - _peak->mzmin > 0.5f) { //direct infusion peak
            mz = 0.5f* (_peak->mzmin + _peak->mzmax);
        }
        showFragmentationScans(mz);
    }

    if (ms2ConsensusScansListWidget->isVisible()) {
        float mz = _peak->peakMz;
        if (_peak->mzmax - _peak->mzmin > 0.5f) { //direct infusion peak
            mz = 0.5f* (_peak->mzmin + _peak->mzmax);
        }
        showConsensusFragmentationScans(mz);
    }

    if (covariantsPanel->isVisible()) {
        getCovariants(_peak);
    }
}

//caller is EICPoint::scanSelected signal
void MainWindow::showScanInfo(Scan* _scan){
    if (!_scan) return;

    fragmentationSpectraDockWidget->setVisible(true);
    fragmentationSpectraWidget->setScan(_scan);

    if (massCalcWidget->isVisible()) {
        massCalcWidget->setFragmentationScan(_scan);
    }

    //Issue 550: Add focus line based on RT click
    if (_scan->mslevel == 2) {

        ms2ScansListWidget->selectMs2Scans(_scan);

        if (_scan->sample && spectraWidget->isVisible()) {
            int scanHistory=50;
            Scan* lastfullscan = _scan->getLastFullScan(scanHistory);
            if(lastfullscan) {
                spectraWidget->setScan(lastfullscan);
                spectraWidget->zoomRegion(_scan->precursorMz,0.5);
            }
        }
    }
}

void MainWindow::spectraFocused(Peak* _peak) {
    if (_peak == NULL) return;

    mzSample* sample = _peak->getSample();
    if (sample == NULL) return;

    Scan* scan = sample->getScan(_peak->scan);
    if (scan == NULL) return;

    if (spectraDockWidget->isVisible() && scan) {
        spectraWidget->setScan(_peak);
    }

    massCalcWidget->setMass(_peak->peakMz);

}

void MainWindow::setupSampleColors() {
    if (samples.size()==0) return;

    float N = samples.size();

    for( unsigned int i=0; i< samples.size(); i++ ) {
        //skip sample that have been colored
        if ( samples[i]->color[0] + samples[i]->color[1] + samples[i]->color[2] > 0 ) continue;

        //set blank to non transparent red
        if (samples[i]->isBlank ) {
            samples[i]->color[0]=0.9;
            samples[i]->color[1]=0.0;
            samples[i]->color[2]=0.0;
            samples[i]->color[3]=1.0;
            continue;
        }

        float hue = 1-0.6*((float)(i+1)/N);
        QColor c = QColor::fromHsvF(hue,1.0,1.0,1.0);
        //qDebug() << "SAMPLE COLOR=" << c;

        samples[i]->color[0] = c.redF();
        samples[i]->color[1] = c.greenF();
        samples[i]->color[2] = c.blueF();
        samples[i]->color[3] = c.alphaF();
    }
}

QString MainWindow::groupTextExport(PeakGroup* group ) {

    if (group == NULL || group->isEmpty()) return QString();

    QStringList groupInfo;
    QString compoundName;
    float expectedRt=-1;

    if ( group->hasCompoundLink() ) {
        compoundName = "\"" + QString(group->compound->name.c_str()) + "\"";
        expectedRt = group->compound->expectedRt;
    }

    if ( compoundName.isEmpty() && group->srmId.length() ) {
        compoundName = "\"" + QString(group->srmId.c_str()) + "\"";
    }

    //sort peaks
    sort(group->peaks.begin(), group->peaks.end(), Peak::compSampleOrder);

    groupInfo << "sample\tgroupId\tcompoundName\texpectedRt\tpeakMz\tmedianMz\trt\trtmin\trtmax\tquality\tpeakIntensity\tpeakArea\tpeakAreaTop\tpeakAreaCorrected\tnoNoiseObs\tsignalBaseLineRatio\tfromBlankSample";
    for(int j=0; j<group->peaks.size(); j++ ) {
        QStringList peakinfo;
        Peak& peak = group->peaks[j];
        mzSample* s = peak.getSample();
        string sampleName;
        if ( s!=NULL) sampleName = s->sampleName;

        peakinfo << QString(sampleName.c_str())
                << QString::number(group->groupId)
                << compoundName
                << QString::number(expectedRt, 'f',4 )
                << QString::number(peak.peakMz, 'f',4 )
                << QString::number(peak.medianMz, 'f', 4)
                << QString::number(peak.rt, 'f', 4)
                << QString::number(peak.rtmin, 'f', 4)
                << QString::number(peak.rtmax, 'f', 4)
                << QString::number(peak.quality, 'f', 4)
                << QString::number(peak.peakIntensity, 'f', 4)
                << QString::number(peak.peakArea, 'f', 4)
                << QString::number(peak.peakAreaTop, 'f', 4)
                << QString::number(peak.peakAreaCorrected, 'f', 4)
                << QString::number(peak.noNoiseObs, 'f', 4)
                << QString::number(peak.signalBaselineRatio, 'f', 4)
                << QString::number(peak.fromBlankSample, 'f', 4);
        groupInfo << peakinfo.join("\t");
    }
    return groupInfo.join("\n");
}

void MainWindow::findCovariants(Peak* _peak) {
    if ( covariantsPanel->isVisible() ) {
        vector<mzLink> links = _peak->findCovariants();
        covariantsPanel->setInfo(links);
    }
}

void MainWindow::setClipboardToGroup(PeakGroup* group) {
    if (group == NULL || group->isEmpty() ) return;
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText( groupTextExport(group) );
}

void MainWindow::showFragmentationScans(float pmz) {

    if (!ms2ScansListWidget || ms2ScansListWidget->isVisible() == false || samples.empty()) return;

    float ppm = static_cast<float>(getUserPPM());

    ms2ScansListWidget->clearTree();

    //Issue 392: Only display scans from visible samples
    vector<mzSample*> visibleSamples = getVisibleSamples();

    for ( unsigned int i=0; i < visibleSamples.size(); i++ ) {
        for (unsigned int j=0; j < visibleSamples[i]->scans.size(); j++ ) {
            Scan* s = visibleSamples[i]->scans[j];
            if ( s->mslevel == 2 && ppmDist(s->precursorMz,pmz) < ppm ) {
                ms2ScansListWidget->addScanItem(s);
            }
        }
    }
}

void MainWindow::showConsensusFragmentationScans(float pmz) {

    if (!ms2ConsensusScansListWidget || ms2ConsensusScansListWidget->isVisible() == false || samples.empty()) return;

    float ppm = static_cast<float>(getUserPPM());

    ms2ConsensusScansListWidget->clearTree();

    //Issue 392: Only display scans from visible samples
    vector<mzSample*> visibleSamples = getVisibleSamples();

    for ( unsigned int i=0; i < visibleSamples.size(); i++ ) {

        vector<Scan*> scanVector{};

        for (unsigned int j=0; j < visibleSamples[i]->scans.size(); j++ ) {
            Scan* s = visibleSamples[i]->scans[j];
            if (s->mslevel == 2 && ppmDist(s->precursorMz, pmz) < ppm) {
                scanVector.push_back(s);
            }
        }

        if (!scanVector.empty()){
            ms2ConsensusScansListWidget->addMs2ScanVectorItem(scanVector);
        }
    }
}

void MainWindow::showMs1Scans() {
    if (lastMs1ScansMz > 0.0f) {
        showMs1Scans(lastMs1ScansMz);
    }
}

void MainWindow::showMs1Scans(float pmz) {

    if (!ms1ScansListWidget || !ms1ScansListWidget->isVisible() || samples.empty()) return;

    lastMs1ScansMz = pmz;

    TreeDockWidgetMs1FilterOptions treeDockWidgetMs1FilterOptions = ms1ScansListWidget->getTreeDockWidgetMs1FilterOptions();

    float ppm = static_cast<float>(getUserPPM());

    float minMz = pmz - pmz * ppm / 1e6f;
    float maxMz = pmz + pmz * ppm / 1e6f;

    ms1ScansListWidget->clearTree();

    //Issue 392: Only display scans from visible samples
    vector<mzSample*> visibleSamples = getVisibleSamples();

    unsigned long scansAdded = 0;
    for ( unsigned int i=0; i < visibleSamples.size(); i++ ) {
        mzSample *sample = visibleSamples[i];

        if (!treeDockWidgetMs1FilterOptions.txtNameFilter.isEmpty()) {
            QString sampleName(sample->sampleName.c_str());
            if (!sampleName.contains(treeDockWidgetMs1FilterOptions.txtNameFilter, Qt::CaseInsensitive)) continue;
        }

        for (unsigned int j=0; j < sample->scans.size(); j++ ) {
            Scan* s = sample->scans[j];
            if (s->mslevel == 1 &&
                    minMz >= s->minMz() && maxMz <= s->maxMz() &&
                    s->rt >= treeDockWidgetMs1FilterOptions.minRt && s->rt <= treeDockWidgetMs1FilterOptions.maxRt) {

                ms1ScansListWidget->addMs1ScanItem(s);
                scansAdded++;
            }
            if (treeDockWidgetMs1FilterOptions.isLimitScans && scansAdded > treeDockWidgetMs1FilterOptions.numScansLimit) {
                return;
            }
        }
    }

}

void MainWindow::showMs3Scans(float preMs1Mz, float preMs2Mz){

    if (!ms3ScansListWidget || !ms3ScansListWidget->isVisible() || samples.empty()) return;

    float ppm = static_cast<float>(getUserPPM()); //TODO: more sophisticated ppm tolerances?

    float minMs1Mz = preMs1Mz - preMs1Mz * ppm / 1e6f;
    float maxMs1Mz = preMs1Mz + preMs1Mz * ppm / 1e6f;

    float minMs2Mz = preMs2Mz - preMs2Mz * ppm / 1e6f;
    float maxMs2Mz = preMs2Mz + preMs2Mz* ppm / 1e6f;

    ms3ScansListWidget->clearTree();

    //Issue 392: Only display scans from visible samples
    vector<mzSample*> visibleSamples = getVisibleSamples();

    for ( unsigned int i=0; i < visibleSamples.size(); i++ ) {
        for (unsigned int j=0; j < visibleSamples[i]->scans.size(); j++ ) {
            Scan* s = visibleSamples[i]->scans[j];
            if (s->mslevel == 3 &&
                    s->ms1PrecursorForMs3 >= minMs1Mz && s->ms1PrecursorForMs3 <= maxMs1Mz &&
                    s->precursorMz >= minMs2Mz && s->precursorMz <= maxMs2Mz) {
                ms3ScansListWidget->addMs3ScanItem(s);
            }
        }
    }
}

QWidget* MainWindow::eicWidgetController() {

    QToolBar *toolBar = new QToolBar(this);
    toolBar->setFloatable(false);
    toolBar->setMovable(false);

    QToolButton *btnZoom = new QToolButton(toolBar);
    btnZoom->setIcon(QIcon(rsrcPath + "/resetzoom.png"));
    btnZoom->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnZoom->setToolTip(tr("Zoom out (0)"));

//    QToolButton *btnPrint = new QToolButton(toolBar);
//    btnPrint->setIcon(QIcon(rsrcPath + "/fileprint.png"));
//    btnPrint->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
//    btnPrint->setToolTip(tr("Print EIC (Ctr+P)"));
//    btnPrint->setShortcut(tr("Ctrl+P"));

    QToolButton *btnPDF = new QToolButton(toolBar);
    btnPDF->setIcon(QIcon(rsrcPath + "/exportpdf.png"));
    btnPDF->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnPDF->setToolTip(tr("Save EIC Image to PDF file"));

    QToolButton *btnPNG = new QToolButton(toolBar);
    btnPNG->setIcon(QIcon(rsrcPath + "/copyPNG.png"));
    btnPNG->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnPNG->setToolTip(tr("Copy EIC Image to Clipboard"));

//    QToolButton *btnLast = new QToolButton(toolBar);
//    btnLast->setIcon(QIcon(rsrcPath + "/last.png"));
//    btnLast->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
//    btnLast->setToolTip(tr("History Back (Ctrl+Left)"));
//    btnLast->setShortcut(tr("Ctrl+Left"));

//    QToolButton *btnNext = new QToolButton(toolBar);
//    btnNext->setIcon(QIcon(rsrcPath + "/next.png"));
//    btnNext->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
//    btnNext->setToolTip(tr("History Forward (Ctrl+Right)"));
//    btnNext->setShortcut(tr("Ctrl+Right"));

//    btnAutoZoom = new QToolButton(toolBar);
//    btnAutoZoom->setCheckable(true);
//    btnAutoZoom->setChecked(true);
//    btnAutoZoom->setIcon(QIcon(rsrcPath + "/autofocus.png"));
//    btnAutoZoom->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
//    btnAutoZoom->setToolTip(tr("Center chromatogram on compound expected RT"));

    QToolButton *btnGallery= new QToolButton(toolBar);
    btnGallery->setIcon(QIcon(rsrcPath + "/gallery.png"));
    btnGallery->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnGallery->setToolTip(tr("Show In Gallery"));

//    QToolButton *btnMarkGood = new QToolButton(toolBar);
//    btnMarkGood->setIcon(QIcon(rsrcPath + "/markgood.png"));
//    btnMarkGood->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
//    btnMarkGood->setToolTip(tr("Bookmark as Good Group (G)"));

//    QToolButton *btnMarkBad = new QToolButton(toolBar);
//    btnMarkBad->setIcon(QIcon(rsrcPath + "/markbad.png"));
//    btnMarkBad->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
//    btnMarkBad->setToolTip(tr("Bookmark as Bad Group (B)"));

//    QToolButton *btnCopyCSV = new QToolButton(toolBar);
//    btnCopyCSV->setIcon(QIcon(rsrcPath + "/copyCSV.png"));
//    btnCopyCSV->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
//    btnCopyCSV->setToolTip(tr("Copy Group Information to Clipboard (Ctrl+C)"));
//    btnCopyCSV->setShortcut(tr("Ctrl+C"));

    QToolButton *btnBookmark = new QToolButton(toolBar);
    btnBookmark->setIcon(QIcon(rsrcPath + "/bookmark.png"));
    btnBookmark->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnBookmark->setToolTip(tr("Bookmark Group (Ctrl+D)"));
    btnBookmark->setShortcut(tr("Ctrl+D"));


    QToolButton *btnIntegrateArea = new QToolButton(toolBar);
    btnIntegrateArea->setIcon(QIcon(rsrcPath + "/integrateArea.png"));
    btnIntegrateArea->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnIntegrateArea->setToolTip(tr("Manual Integration (Shift+MouseDrag)"));

//    QToolButton *btnAverageSpectra = new QToolButton(toolBar);
//    btnAverageSpectra->setIcon(QIcon(rsrcPath + "/averageSpectra.png"));
//    btnAverageSpectra->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
//    btnAverageSpectra->setToolTip(tr("Average Spectra (Ctrl+MouseDrag)"));

    btnGroupPeaks = new QToolButton(toolBar);
    btnGroupPeaks->setCheckable(true);
    btnGroupPeaks->setChecked(true);
    btnGroupPeaks->setIcon(QIcon(rsrcPath + "/gaussian_groups.png"));
    btnGroupPeaks->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnGroupPeaks->setToolTip(tr("Automatic Peak Grouping"));

    btnEICFill = new QToolButton(toolBar);
    btnEICFill->setCheckable(true);
    btnEICFill->setChecked(true);
    btnEICFill->setIcon(QIcon(rsrcPath + "/filled_gaussian.png"));
    btnEICFill->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnEICFill->setToolTip(tr("Show Filled EIC"));

    btnEICDots = new QToolButton(toolBar);
    btnEICDots->setCheckable(true);
    btnEICDots->setChecked(false);
    btnEICDots->setIcon(QIcon(rsrcPath + "/dotted_gaussian.png"));
    btnEICDots->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnEICDots->setToolTip(tr("Show Scans as Dots"));

    btnLockRt = new QToolButton(toolBar);
    btnLockRt->setCheckable(true);
    btnLockRt->setChecked(false);
    btnLockRt->setIcon(QIcon(rsrcPath + "/padlock.png"));
    btnLockRt->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnLockRt->setToolTip("Lock RT range");

    btnBarPlot = new QToolButton(toolBar);
    btnBarPlot->setCheckable(true);
    btnBarPlot->setChecked(false);
    btnBarPlot->setIcon(QIcon(rsrcPath + "/bar_plot_samples.png"));
    btnBarPlot->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnBarPlot->setToolTip(tr("Show Bar Plot"));

    btnIsotopePlot = new QToolButton(toolBar);
    btnIsotopePlot->setCheckable(true);
    btnIsotopePlot->setChecked(false);
    btnIsotopePlot->setIcon(QIcon(rsrcPath + "/isotope.png"));
    btnIsotopePlot->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnIsotopePlot->setToolTip(tr("Show Isotope Plot"));
/*
    QSpinBox* smoothingWindowBox = new QSpinBox(toolBar);
    smoothingWindowBox->setRange(1, 2000);
    smoothingWindowBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    smoothingWindowBox->setValue(settings->value("eic_smoothingWindow").toInt());
    smoothingWindowBox->setToolTip("EIC Smoothing Window");
    connect(smoothingWindowBox, SIGNAL(valueChanged(int)), SLOT(updateEicSmoothingWindow(int)));
*/

    toolBar->addWidget(btnZoom);
    toolBar->addWidget(btnBookmark);
    toolBar->addWidget(btnIntegrateArea);
    toolBar->addWidget(btnGallery);

    toolBar->addSeparator();

    toolBar->addWidget(btnPDF);
    toolBar->addWidget(btnPNG);

    //Issue 222
    toolBar->addSeparator();
    toolBar->addWidget(btnEICFill);
    toolBar->addWidget(btnEICDots);
    toolBar->addWidget(btnGroupPeaks);

    toolBar->addSeparator();
    //toolBar->addWidget(btnAutoZoom);
    toolBar->addWidget(btnLockRt);
    toolBar->addWidget(btnBarPlot);
    toolBar->addWidget(btnIsotopePlot);
    //toolBar->addWidget(btn)

    //Issue 222:
    //disabled buttons

    //toolBar->addWidget(btnAverageSpectra); // Issue 222: Not clear what this does at this point

    //toolBar->addWidget(btnCopyCSV);
    //toolBar->addWidget(btnMarkGood);
    //toolBar->addWidget(btnMarkBad);

    //toolBar->addWidget(btnLast);
    //toolBar->addWidget(btnNext);
   // toolBar->addSeparator();

    //toolBar->addWidget(btnPrint);
    //toolBar->addSeparator();

//    toolBar->addWidget(smoothingWindowBox);

//    connect(btnLast,SIGNAL(clicked()), SLOT(historyLast()));
//    connect(btnNext,SIGNAL(clicked()), SLOT(historyNext()));
//    connect(btnPrint, SIGNAL(clicked()), SLOT(print()));
    connect(btnZoom, SIGNAL(clicked()), eicWidget, SLOT(resetZoom()));
    connect(btnPDF, SIGNAL(clicked()), SLOT(exportPDF()));
    connect(btnPNG, SIGNAL(clicked()), SLOT(exportSVG()));
//    connect(btnAutoZoom,SIGNAL(toggled(bool)), eicWidget,SLOT(autoZoom(bool)));
    connect(btnBookmark,SIGNAL(clicked()),  this,  SLOT(bookmarkSelectedPeakGroup()));
//    connect(btnCopyCSV,SIGNAL(clicked()),  eicWidget, SLOT(copyToClipboard()));
//    connect(btnMarkGood,SIGNAL(clicked()), eicWidget, SLOT(markGroupGood()));
//    connect(btnMarkBad,SIGNAL(clicked()),  eicWidget, SLOT(markGroupBad()));
    connect(btnGallery,SIGNAL(clicked()),  eicWidget, SLOT(setGallaryToEics()));
    connect(btnIntegrateArea,SIGNAL(clicked()),  eicWidget, SLOT(startAreaIntegration()));
//    connect(btnAverageSpectra,SIGNAL(clicked()),  eicWidget, SLOT(startSpectralAveraging()));

    connect(btnEICFill, SIGNAL(toggled(bool)), eicWidget, SLOT(showEICFill(bool)));
    connect(btnEICFill, SIGNAL(toggled(bool)), eicWidget, SLOT(replot()));

    connect(btnEICDots, SIGNAL(toggled(bool)), eicWidget, SLOT(emphasizeEICPoints(bool)));
    connect(btnEICDots, SIGNAL(toggled(bool)), eicWidget, SLOT(replot()));

    connect(btnGroupPeaks, SIGNAL(toggled(bool)), eicWidget, SLOT(automaticPeakGrouping(bool)));
    connect(btnGroupPeaks, SIGNAL(toggled(bool)), eicWidget, SLOT(groupPeaks()));
    connect(btnGroupPeaks, SIGNAL(toggled(bool)), eicWidget, SLOT(replot()));

    connect(btnLockRt, SIGNAL(toggled(bool)), eicWidget, SLOT(preservePreviousRtRange(bool)));
    connect(btnLockRt, SIGNAL(toggled(bool)), eicWidget, SLOT(replot()));

    connect(btnBarPlot, SIGNAL(toggled(bool)), eicWidget, SLOT(showBarPlot(bool)));
    connect(btnBarPlot, SIGNAL(toggled(bool)), eicWidget, SLOT(replot()));

    connect(btnIsotopePlot, SIGNAL(toggled(bool)), eicWidget, SLOT(showIsotopePlot(bool)));
    connect(btnIsotopePlot, SIGNAL(toggled(bool)), eicWidget, SLOT(replot()));

    QWidget *window = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->setContentsMargins(QMargins(0,0,0,0));
    layout->addWidget(toolBar);
    layout->addWidget(eicWidget);

    window->setLayout(layout);
    return window;
}

void MainWindow::getCovariants(Peak* peak ) {
    if(!peak) return;
    if(!covariantsPanel->isVisible()) return;

    Scan* scan = peak->getScan();
    if (!scan) return;

    mzSample* sample = scan->getSample();
    if (!sample) return;

    int ionizationMode = scan->getPolarity();
    if (getIonizationMode()) ionizationMode=getIonizationMode(); //user specified ionization mode

    float ppm = getUserPPM();

    vector<mzLink> links = peak->findCovariants();
    vector<mzLink> linksX = SpectraWidget::findLinks(peak->peakMz, scan, ppm, ionizationMode);
    for(unsigned int i=0; i < linksX.size(); i++ ) links.push_back(linksX[i]);

    //correlations
    float rtmin = peak->rtmin-1;
    float rtmax=  peak->rtmax+1;
    for(int i=0; i < links.size(); i++ ) {
        links[i].correlation = sample->correlation(links[i].mz1, links[i].mz2, 5, rtmin, rtmax);
    }

    //matching compounds
    for(int i=0; i < links.size(); i++ ) {
        vector<MassCalculator::Match> matches =  DB.findMatchingCompounds(links[i].mz2, ppm, ionizationMode);
        for(auto& m: matches) { links[i].note += " |" + m.name; break; }
    }

    vector<mzLink>subset;
    for(int i=0; i < links.size(); i++ ) { if(links[i].correlation > 0.5)  subset.push_back(links[i]); }
    if(subset.size())    covariantsPanel->setInfo(subset);
    if(subset.size() && galleryDockWidget->isVisible()) galleryWidget->addEicPlots(subset);
}


PeakGroup::QType MainWindow::getUserQuantType() {
    if (quantType) {
        QString type = quantType->currentText();

        if (projectDockWidget->currentProject) {
            if (projectDockWidget->currentProject->quantTypeInverseMap.find(type.toStdString()) != projectDockWidget->currentProject->quantTypeInverseMap.end()) {
                type = QString(projectDockWidget->currentProject->quantTypeInverseMap[type.toStdString()].c_str());
            }
        }

        if (type  == "AreaTop") return PeakGroup::AreaTop;
        else if (type  == "Area")    return PeakGroup::Area;
        else if (type  == "Height")  return PeakGroup::Height;
        else if (type  == "Retention Time")  return PeakGroup::RetentionTime;
        else if (type  == "Quality")  return PeakGroup::Quality;
        else if (type == "Area Uncorrected") return PeakGroup::AreaNotCorrected;
        else if (type == "Area Fractional") return PeakGroup::AreaFractional;
        else if (type  == "S/N Ratio")  return PeakGroup::SNRatio;
        else if (type == "Smoothed Height") return PeakGroup::SmoothedHeight;
        else if (type == "Smoothed Area") return PeakGroup::SmoothedArea;
        else if (type == "Smoothed Area Uncorrected") return PeakGroup::SmoothedAreaNotCorrected;
        else if (type == "Smoothed AreaTop") return PeakGroup::SmoothedAreaTop;
        else if (type == "Smoothed S/N Ratio") return PeakGroup::SmoothedSNRatio;
        else if (type == "FWHM Area") return PeakGroup::AreaFWHM;
        else if (type == "FWHM Smoothed Area") return PeakGroup::SmoothedAreaFWHM;
    }
    return PeakGroup::AreaTop;
}

void MainWindow::markGroup(PeakGroup* group, char label) {
    if(!group) return;

    group->addLabel(label);
    bookmarkPeakGroup(group);
    //if (getClassifier()) { getClassifier()->refineModel(group); }
    //getPlotWidget()->scene()->update();
}


MatrixXf MainWindow::getIsotopicMatrix(PeakGroup* group) {

    PeakGroup::QType qtype = getUserQuantType();
    //get visiable samples
    vector <mzSample*> vsamples = getVisibleSamples();
    sort(vsamples.begin(), vsamples.end(), mzSample::compSampleOrder);

    //get isotopic groups
    vector<PeakGroup*>isotopes;
    for(int i=0; i < group->childCount(); i++ ) {
        if (group->children[i].isIsotope() ) {
            PeakGroup* isotope = &(group->children[i]);
            isotopes.push_back(isotope);
        }
    }
    std::sort(isotopes.begin(), isotopes.end(), PeakGroup::compC13);

    MatrixXf MM((int) vsamples.size(),(int) isotopes.size());            //rows=samples, cols=isotopes
    MM.setZero();

    for(int i=0; i < isotopes.size(); i++ ) {
        if (! isotopes[i] ) continue;
        vector<float> values = isotopes[i]->getOrderedIntensityVector(vsamples,qtype); //sort isotopes by sample
        for(int j=0; j < values.size(); j++ ) MM(j,i)=values[j];  //rows=samples, columns=isotopes
    }

    int numberofCarbons=0;
    if (group->compound && !group->compound->formula.empty()) {
        MassCalculator mcalc;
        map<string,int>composition= mcalc.getComposition(group->compound->formula);
        numberofCarbons=composition["C"];
    }

    isotopeC13Correct(MM,numberofCarbons);
    return MM;
}

void MainWindow::isotopeC13Correct(MatrixXf& MM, int numberofCarbons) {
    if (numberofCarbons == 0) return;

    qDebug() << "IsotopePlot::isotopeC13Correct() " << MM.rows() << " " << MM.cols() << " nCarbons=" <<  numberofCarbons << endl;
    if (settings && settings->value("isotopeC13Correction").toBool() == false ) return;

    for(int i=0; i<MM.rows(); i++ ) {		//samples
        float sum=0; vector<double>mv(MM.cols());
        //qDebug() << "Correction for " << i;

        //make a copy
        for(int j=0; j< MM.cols(); j++) { mv[j]=MM(i,j); sum += MM(i,j); }


        //normalize to sum=1 and correct
        if(sum>0) {
            for(int j=0; j< mv.size(); j++) { mv[j] /= sum; } //normalize
            vector<double>cmv = mzUtils::naturalAbundanceCorrection(numberofCarbons,mv);

            for(int j=0; j< mv.size(); j++) {
                MM(i,j) = cmv[j];

                //cerr << mv[j] << " " << cmv[j] << endl;
            }
        }
    }
    //qDebug() << "IsotopePlot::IsotopePlot() done..";

}

void MainWindow::updateEicSmoothingWindow(int value) {
    settings->setValue("eic_smoothingWindow",value);
    getEicWidget()->recompute(!getEicWidget()->isPreservePreviousRtRange());
    getEicWidget()->replot();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    foreach (QUrl url, event->mimeData()->urls() ) {
        if (isSampleFileType(url.toString())) {
            event->acceptProposedAction();
            return;
        } else if ( isProjectFileType(url.toString())) {
            event->acceptProposedAction();
            return;
        }
    }
}

void MainWindow::dropEvent(QDropEvent *event)
 {

    QStringList samples;
    QStringList projects;

    foreach (QUrl url, event->mimeData()->urls() ) {
        QString filename = url.toString();
        filename.replace("file:///","");
        if (isSampleFileType(filename)) {
            samples << filename;
            qDebug() << filename;
        } else if ( isProjectFileType(filename)) {
            projects << filename;
            qDebug() << filename;
        }
    }

    if (projects.size() > 0 ) {

        return;
    }

    if ( samples.size() > 0 ) {
        mzFileIO* fileLoader  = new mzFileIO(this);
        fileLoader->setMainWindow(this);
        fileLoader->loadSamples(samples);
        fileLoader->start();
    }
 }

bool MainWindow::isSampleFileType(QString filename) {
    if (filename.endsWith("mzXML",Qt::CaseInsensitive)) {
       return 1;
    } else if (filename.endsWith("cdf",Qt::CaseInsensitive) || filename.endsWith("nc",Qt::CaseInsensitive) ) {
       return 1;
    } else if (filename.endsWith("mzCSV",Qt::CaseInsensitive)) {
       return 1;
    } else if (filename.endsWith("mzData",Qt::CaseInsensitive)) {
      return 1;
    } else if (filename.endsWith("mzML",Qt::CaseInsensitive)) {
      return 1;
    }
    return 0;
}


bool MainWindow::isProjectFileType(QString filename) {
    if (filename.endsWith("mzrollDB",Qt::CaseInsensitive))  return 1;
    if (filename.endsWith("mzroll",Qt::CaseInsensitive))  return 1;
    return 0;
}

void MainWindow::changeUserAdduct() {
    QVariant v = adductType->currentData();
    Adduct*  adduct =  v.value<Adduct*>();
    if(adduct) {
        _ionizationMode = SIGN(adduct->charge);
        qDebug() << "MainWindow::changeUserAdduct():" << QString(adduct->name.c_str());
        massCalcWidget->ionization->setValue(_ionizationMode);

        mzSlice& currentSlice = eicWidget->getMzSlice();
        if(currentSlice.compound) {
            setCompoundFocus(currentSlice.compound, false);
        } else if (!searchText->text().isEmpty()){
            //Issue 361
            doSearch(searchText->text());
        }
    }
}

Adduct* MainWindow::getUserAdduct() {
    QVariant v = adductType->currentData();
    Adduct*  adduct =  v.value<Adduct*>();
    if(adduct) {
        qDebug() << "MainWindow::getUserAdduct()" << QString(adduct->name.c_str());
        return adduct;
    } else if (_ionizationMode > 0 ) {
        return MassCalculator::PlusHAdduct;
    } else if (_ionizationMode < 0 ) {
        return MassCalculator::MinusHAdduct;
    } else {
        return MassCalculator::ZeroMassAdduct;
    }
}

QColor MainWindow::getBackgroundAdjustedBlack(QWidget *widget){

    //check background color
    QColor backgroundColor = QWidget::palette().color(widget->backgroundRole());

    if (backgroundColor.red() == darkModeBackgroundColor && backgroundColor.green() == darkModeBackgroundColor && backgroundColor.blue() == darkModeBackgroundColor) {
         return QColor(225, 225, 225); //off-white
    } else {
        return Qt::black;
    }
}

void MainWindow::updateAdductsInGUI() {

    adductType->clear();

    if (isotopeWidget) {
        isotopeWidget->adductComboBox->clear();
    }

    QString enabledAdductNames;
    for(Adduct* a: DB.adductsDB) {
        adductType->addItem(a->name.c_str(),QVariant::fromValue(a));

        if (isotopeWidget) {
            isotopeWidget->adductComboBox->addItem(a->name.c_str(),QVariant::fromValue(a));
        }

        enabledAdductNames.append(a->name.c_str());
        enabledAdductNames.append(";");
    }

    settings->setValue("enabledAdducts", enabledAdductNames);

    selectAdductsDialog->updateGUI();

    emit(updatedAvailableAdducts());
}

IsotopeParameters MainWindow::getIsotopeParameters(){

    IsotopeParameters isotopeParameters;

    isotopeParameters.peakPickingAndGroupingParameters = this->getPeakPickingAndGroupingParameters();

    isotopeParameters.ppm = static_cast<float>(_ppmWindow);
    isotopeParameters.clsf = clsf;
    isotopeParameters.adduct = getUserAdduct();

    if (settings) {
        isotopeParameters.maxIsotopeScanDiff  = settings->value("maxIsotopeScanDiff", 10).toDouble();
        isotopeParameters.minIsotopicCorrelation  = settings->value("minIsotopicCorrelation", 0).toDouble();
        isotopeParameters.maxNaturalAbundanceErr  = settings->value("maxNaturalAbundanceErr", 100).toDouble();
        isotopeParameters.isC13Labeled   =  settings->value("C13Labeled", false).toBool();
        isotopeParameters.isN15Labeled   =  settings->value("N15Labeled", false).toBool();
        isotopeParameters.isS34Labeled   =  settings->value("S34Labeled", false).toBool();
        isotopeParameters.isD2Labeled   =   settings->value("D2Labeled", false).toBool();
        isotopeParameters.isIgnoreNaturalAbundance = settings->value("chkIgnoreNaturalAbundance", false).toBool();
        isotopeParameters.isExtractNIsotopes = settings->value("chkExtractNIsotopes", false).toBool();
        isotopeParameters.maxIsotopesToExtract = settings->value("spnMaxIsotopesToExtract", 5).toInt();
        isotopeParameters.eic_smoothingWindow = static_cast<float>(settings->value("eic_smoothingWindow", 1).toDouble());
        isotopeParameters.eic_smoothingAlgorithm = static_cast<EIC::SmootherType>(settings->value("eic_smoothingAlgorithm", 0).toInt());
    } else {
        qDebug() << "MainWindow::getIsotopeParameters(): Could not find saved program settings! using defaults.";
    }

    isotopeParameters.isotopeParametersType = IsotopeParametersType::FROM_GUI;

    return isotopeParameters;
}

void MainWindow::showPeakInMs1Spectrum(Peak* peak) {
    if (!peak) return;
    if (spectraWidget->isVisible()) {
        spectraWidget->setScan(peak, true);
    }
}

void MainWindow::showPeakInCovariantsWidget(Peak *peak) {
    if (!peak) return;
    if (covariantsPanel->isVisible()) {
        getCovariants(peak);
    }
}

shared_ptr<PeakPickingAndGroupingParameters> MainWindow::getPeakPickingAndGroupingParameters(){

     if (!settings) return peakPickingAndGroupingParameters;

     float peakRtBoundsSlopeThreshold = settings->value("peakRtBoundsSlopeThreshold", 0.01f).toFloat();
     if (peakRtBoundsSlopeThreshold < 0) {
         peakRtBoundsSlopeThreshold = -1.0f;
     }

     float mergedSmoothedMaxToBoundsMinRatio = settings->value("mergedSmoothedMaxToBoundsMinRatio", 1.0f).toFloat();
     if (mergedSmoothedMaxToBoundsMinRatio < 0) {
         mergedSmoothedMaxToBoundsMinRatio = -1.0f;
     }

     // START EIC::getPeakPositionsD()
     //peak picking
     peakPickingAndGroupingParameters->peakSmoothingWindow = settings->value("eic_smoothingWindow").toInt();
     //peakPickingAndGroupingParameters->peakRtBoundsMaxIntensityFraction = peakRtBoundsMaxIntensityFraction;
     peakPickingAndGroupingParameters->peakRtBoundsSlopeThreshold = peakRtBoundsSlopeThreshold;
     peakPickingAndGroupingParameters->peakBaselineSmoothingWindow = settings->value("baseline_smoothing").toInt();
     peakPickingAndGroupingParameters->peakBaselineDropTopX = settings->value("baseline_quantile").toInt();
     peakPickingAndGroupingParameters->peakIsComputeBounds = true;
     peakPickingAndGroupingParameters->peakIsReassignPosToUnsmoothedMax = true;

     //eic
     int eicBaselineEstimationType = settings->value("eicBaselineEstimationType", EICBaselineEstimationType::DROP_TOP_X).toInt();
     peakPickingAndGroupingParameters->eicBaselineEstimationType = static_cast<EICBaselineEstimationType>(eicBaselineEstimationType);

//    // END EIC::getPeakPositionsD()

//    // START EIC::groupPeaksE()

//    //merged EIC
      peakPickingAndGroupingParameters->mergedSmoothingWindow = settings->value("eic_smoothingWindow").toInt();
//    peakPickingAndGroupingParameters->mergedPeakRtBoundsMaxIntensityFraction = mergedPeakRtBoundsMaxIntensityFraction;
      peakPickingAndGroupingParameters->mergedPeakRtBoundsSlopeThreshold = peakRtBoundsSlopeThreshold;
      peakPickingAndGroupingParameters->mergedSmoothedMaxToBoundsMinRatio = mergedSmoothedMaxToBoundsMinRatio;
      peakPickingAndGroupingParameters->mergedSmoothedMaxToBoundsIntensityPolicy = SmoothedMaxToBoundsIntensityPolicy::MINIMUM; // loosest - use smaller intensity in denominator
      peakPickingAndGroupingParameters->mergedBaselineSmoothingWindow = settings->value("baseline_smoothing").toInt();
      peakPickingAndGroupingParameters->mergedBaselineDropTopX = settings->value("baseline_quantile").toInt();
      peakPickingAndGroupingParameters->mergedIsComputeBounds = true;

//    //grouping
      peakPickingAndGroupingParameters->groupMaxRtDiff = settings->value("grouping_maxRtWindow").toFloat();
      peakPickingAndGroupingParameters->groupMergeOverlap = settings->value("spnMergeOverlap").toFloat();

//    //post-grouping filters
//    peakPickingAndGroupingParameters->filterMinGoodGroupCount = minGoodGroupCount;
//    peakPickingAndGroupingParameters->filterMinQuality = minQuality;
//    peakPickingAndGroupingParameters->filterMinNoNoiseObs = minNoNoiseObs;
//    peakPickingAndGroupingParameters->filterMinSignalBaselineRatio = minSignalBaseLineRatio;
//    peakPickingAndGroupingParameters->filterMinGroupIntensity = minGroupIntensity;
//    peakPickingAndGroupingParameters->filterMinPrecursorCharge = minPrecursorCharge;

//    // END EIC::groupPeaksE()

      return peakPickingAndGroupingParameters;
}
