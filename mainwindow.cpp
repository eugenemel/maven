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

  setWindowTitle(PROGRAMNAME + " " + QString::number(MAVEN_VERSION));
  qDebug() << "APP=" <<  QApplication::applicationName() << "VER=" <<  QApplication::applicationVersion();

  readSettings();

  QList<QString> dirs;
  QString methodsFolder =      settings->value("methodsFolder").value<QString>();
  dirs    << methodsFolder
            << "."
            << "./methods"
            << QApplication::applicationDirPath()  + "/methods"
            << QApplication::applicationDirPath() + "/../Resources/methods";

    QString defaultModelFile =   settings->value("clsfModelFilename").value<QString>();
    foreach (QString d, dirs) {
        qDebug() << "Checking dir: " + d;
        QFile test(d+ "/" + defaultModelFile);
        if (test.exists()) { methodsFolder=d; settings->setValue("methodsFolder", methodsFolder); break;}
    }

    qDebug() << "METHODS FOLDER=" << methodsFolder;

    //CONNECT TO COMPOUND DATABASE
    QString writeLocation = QStandardPaths::standardLocations(QStandardPaths::DataLocation).first();
    if(!QFile::exists(writeLocation))  { QDir dir; dir.mkdir(writeLocation); }
    qDebug() << "WRITE FOLDER=" <<  writeLocation;
    DB.connect(writeLocation + "/ligand.db");

    DB.loadCompoundsSQL("KNOWNS");
    //DB.loadCompoundsSQL("ALL");

    //QString commonFragments =   methodsFolder + "/" + "FRAGMENTS.csv";
    //if(QFile::exists(commonFragments)) DB.fragmentsDB = DB.loadAdducts(commonFragments.toStdString());

    QString commonAdducts =     methodsFolder + "/" + "ADDUCTS.csv";
    if(QFile::exists(commonAdducts))   DB.adductsDB = DB.loadAdducts(commonAdducts.toStdString());

    clsf = new ClassifierNeuralNet();    //clsf = new ClassifierNaiveBayes();
    QString clsfModelFilename = methodsFolder +  "/"  +   defaultModelFile;
    if(QFile::exists(clsfModelFilename)) {
        clsf->loadModel(clsfModelFilename.toStdString());
    } else {
        qDebug() << "ERROR: Can't find default.model"  << methodsFolder;
    }



    //progress Bar on the bottom of the page
    statusText  = new QLabel(this);
    statusText->setOpenExternalLinks(true);
    statusBar()->addPermanentWidget(statusText,1);

    progressBar =  new QProgressBar(this);
    progressBar->hide();
    statusBar()->addPermanentWidget(progressBar);

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

    spectraWidget = new SpectraWidget(this);
    isotopeWidget =  new IsotopeWidget(this);
    massCalcWidget =  new MassCalcWidget(this);
    covariantsPanel= new TreeDockWidget(this,"Covariants",3);

    fragmentationEventsWidget	= new TreeDockWidget(this,"MS2 List", 7);
    fragmentationEventsWidget->setupScanListHeader();

    srmDockWidget 	= new TreeDockWidget(this,"SRM List", 1);
    ligandWidget = new LigandWidget(this);
    heatmap	 = 	  new HeatMap(this);
    galleryWidget = new GalleryWidget(this);
    bookmarkedPeaks = addPeaksTable("Bookmarks");
    spectraDockWidget =  createDockWidget("Spectra",spectraWidget);
    heatMapDockWidget =  createDockWidget("HeatMap",heatmap);
    galleryDockWidget =  createDockWidget("Gallery",galleryWidget);
    scatterDockWidget =  new ScatterPlot(this);
    projectDockWidget =  new ProjectDockWidget(this);
    rconsoleDockWidget =  new RconsoleWidget(this);
    fragmenationSpectraWidget = new SpectraWidget(this);
    fragmenationSpectraDockWidget =  createDockWidget("Fragmentation Spectra",fragmenationSpectraWidget);
    ligandWidget->setVisible(false);
    covariantsPanel->setVisible(false);
    isotopeWidget->setVisible(false);
    massCalcWidget->setVisible(false);
    fragmentationEventsWidget->setVisible(false);
    bookmarkedPeaks->setVisible(false);
    spectraDockWidget->setVisible(false);
    scatterDockWidget->setVisible(false);
    heatMapDockWidget->setVisible(false);
    galleryDockWidget->setVisible(false);
    projectDockWidget->setVisible(false);
    rconsoleDockWidget->setVisible(false);
    fragmenationSpectraDockWidget->setVisible(false);    //treemap->setVisible(false);
    //peaksPanel->setVisible(false);
    //treeMapDockWidget =  createDockWidget("TreeMap",treemap);



    //
    //DIALOGS
    //
    peakDetectionDialog = new PeakDetectionDialog(this);
    peakDetectionDialog->setMainWindow(this);
    peakDetectionDialog->setSettings(settings);

    //alignment dialog
    alignmentDialog	 =  new AlignmentDialog(this);
    connect(alignmentDialog->alignButton,SIGNAL(clicked()),SLOT(Align()));
    connect(alignmentDialog->UndoAlignment,SIGNAL(clicked()),SLOT(UndoAlignment()));

    //calibration dialog
    calibrateDialog	 =  new CalibrateDialog(this);

    //Compound Library  Manager
    libraryDialog = new LibraryMangerDialog(this);
    libraryDialog->setMainWindow(this);


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
    addDockWidget(Qt::BottomDockWidgetArea,fragmentationEventsWidget,Qt::Horizontal);
    addDockWidget(Qt::BottomDockWidgetArea,scatterDockWidget,Qt::Horizontal);
    addDockWidget(Qt::BottomDockWidgetArea,bookmarkedPeaks,Qt::Horizontal);
    addDockWidget(Qt::BottomDockWidgetArea,galleryDockWidget,Qt::Horizontal);
    addDockWidget(Qt::BottomDockWidgetArea,srmDockWidget,Qt::Horizontal);
    addDockWidget(Qt::BottomDockWidgetArea,rconsoleDockWidget,Qt::Horizontal);
    addDockWidget(Qt::BottomDockWidgetArea,fragmenationSpectraDockWidget,Qt::Horizontal);

    //addDockWidget(Qt::BottomDockWidgetArea,peaksPanel,Qt::Horizontal);
    //addDockWidget(Qt::BottomDockWidgetArea,treeMapDockWidget,Qt::Horizontal);
    //addDockWidget(Qt::BottomDockWidgetArea,heatMapDockWidget,Qt::Horizontal);

    tabifyDockWidget(ligandWidget,projectDockWidget);

    tabifyDockWidget(spectraDockWidget,massCalcWidget);
    tabifyDockWidget(spectraDockWidget,isotopeWidget);
    tabifyDockWidget(spectraDockWidget,massCalcWidget);
    tabifyDockWidget(spectraDockWidget,fragmentationEventsWidget);
    tabifyDockWidget(spectraDockWidget,covariantsPanel);
    tabifyDockWidget(spectraDockWidget,galleryDockWidget);
    tabifyDockWidget(spectraDockWidget,rconsoleDockWidget);

    setContextMenuPolicy(Qt::NoContextMenu);

    if (settings->contains("windowState")) {
        restoreState(settings->value("windowState").toByteArray());
    }

    if ( settings->contains("geometry")) {
    	restoreGeometry(settings->value("geometry").toByteArray());
    }



    projectDockWidget->show();
    scatterDockWidget->hide();
    fragmentationEventsWidget->hide();



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
    ligandWidget->setDatabase("KNOWNS");
    //if(settings->contains("lastDatabaseFile")) ligandWidget->setDatabase("lastDatabaseFile");

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

        setUrl("http://genomics-pubs.princeton.edu/mzroll/index.php?show=bugs",
                "Woops.. did the program crash last time? Would you like to report a bug?");
    } 

    settings->setValue("closeEvent", 0 );
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
    return NULL;
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

TableDockWidget* MainWindow::addPeaksTable(QString title) {
    QPointer<TableDockWidget> panel	 = new TableDockWidget(this,title,0);
    addDockWidget(Qt::BottomDockWidgetArea,panel,Qt::Horizontal);
    groupTables.push_back(panel);

    /*
    if (sideBar) {
        QToolButton *btnTable = new QToolButton(sideBar);
        btnTable->setIcon(QIcon(rsrcPath + "/featuredetect.png"));
        btnTable->setChecked( panel->isVisible() );
        btnTable->setCheckable(true);
        btnTable->setToolTip(title);
        connect(btnTable,SIGNAL(clicked(bool)),panel, SLOT(setVisible(bool)));
        connect(panel,SIGNAL(visibilityChanged(bool)),btnTable,SLOT(setChecked(bool)));
        sideBar->addWidget(btnTable);
    }
   */

    return panel;
}


void MainWindow::setUserPPM( double x) { 
    _ppmWindow=x;
}

void MainWindow::setIonizationMode( int x ) {
    qDebug() << "setIonizationMode: " << x;
    _ionizationMode=x;
     massCalcWidget->setCharge(_ionizationMode);
     isotopeWidget->setCharge(_ionizationMode);

     if(x>0) adductType->setCurrentText("[M+H]+");
     else if(x<0) adductType->setCurrentText("[M-H]-");
     else adductType->setCurrentText("[M]");
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


void MainWindow::bookmarkSelectedPeakGroup() {
   bookmarkPeakGroup(eicWidget->getSelectedGroup());
}



void MainWindow::bookmarkPeakGroup(PeakGroup* group) {
    qDebug() << "MainWindow::bookmarkPeakGroup(group)";
    if ( bookmarkedPeaks->isVisible() == false ) {
        bookmarkedPeaks->setVisible(true);
    }
    bookmarkedPeaks->addPeakGroup(group,true);
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

void MainWindow::setCompoundFocus(Compound*c) {
    if (c == NULL ) return;

    /*
    int charge = 0;
    if (samples.size() > 0  && samples[0]->getPolarity() > 0) charge = 1;
    if (getIonizationMode()) charge=getIonizationMode(); //user specified ionization mode
    */

    Adduct* adduct = getUserAdduct();
    cerr << "setCompoundFocus:" << c->name.c_str() << " " << adduct->name << " " << c->expectedRt << endl;
    searchText->setText(c->name.c_str());

    float mz = adduct->computeAdductMass(c->mass);
    cerr << "setCompoundFocus:" << c->name.c_str() << " " << adduct->name << " " << c->expectedRt << " mass=" << c->mass << " mz=" << mz << endl;
    if (eicWidget->isVisible() && samples.size() > 0 )  eicWidget->setCompound(c,adduct);
    if (isotopeWidget && isotopeWidget->isVisible() ) isotopeWidget->setCompound(c);
    if (massCalcWidget && massCalcWidget->isVisible() )  massCalcWidget->setMass(mz);

    //show fragmentation
    if (fragmenationSpectraWidget->isVisible())  {
        PeakGroup* group = eicWidget->getSelectedGroup();
        fragmenationSpectraWidget->overlayPeakGroup(group);
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
    QRegExp formula("C[1-9].*(H[1-9]+|O[1-9]+|N[1-9]+)",Qt::CaseInsensitive,QRegExp::RegExp);
    if (needle.contains(formula) ){ setFormulaFocus(needle);}
}

void MainWindow::setMzValue() { 
    bool isDouble =false;
    QString value = searchText->text();
    float mz = value.toDouble(&isDouble);
    if (isDouble) {
        if(eicWidget->isVisible() ) eicWidget->setMzSlice(mz);
        if(massCalcWidget->isVisible() ) massCalcWidget->setMass(mz);
        if(fragmentationEventsWidget->isVisible()   ) showFragmentationScans(mz);
    }
    suggestPopup->addToHistory(QString::number(mz,'f',5));
}

void MainWindow::setMzValue(float mz) { 
    searchText->setText(QString::number(mz,'f',8));
    if (eicWidget->isVisible() ) eicWidget->setMzSlice(mz);
    if (massCalcWidget->isVisible() ) massCalcWidget->setMass(mz);
    if (fragmentationEventsWidget->isVisible()   ) showFragmentationScans(mz);
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
            if (filename.endsWith("mzrollDB"))  projectDockWidget->loadProjectSQLITE(filename);
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
    if (progressBar->isVisible() == false && progress != totalSteps ) { progressBar->show();}
    progressBar->setRange(0,totalSteps);
    progressBar->setValue(progress);
    if (progress == totalSteps ) { progressBar->hide();}
}

void MainWindow::readSettings() {
    settings = new QSettings("mzRoll", "Application Settings");

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
        settings->setValue("grouping_maxRtWindow", 0.5 );

    if( ! settings->contains("grouping_maxMzWindow") )
        settings->setValue("grouping_maxMzWindow", 100);

    if( ! settings->contains("eic_smoothingAlgorithm") )
        settings->setValue("eic_smoothingAlgorithm", 0);

    if( ! settings->contains("eic_smoothingWindow") )
        settings->setValue("eic_smoothingWindow", 5);

    if( ! settings->contains("eic_ppmWindow") )
        settings->setValue("eic_ppmWindow",100);

    if( ! settings->contains("ppmWindowBox") )
        settings->setValue("ppmWindowBox",5);

    if( ! settings->contains("mzslice") )
        settings->setValue("mzslice",QRectF(100.0,100.01,0,30));

    if( ! settings->contains("ionizationMode") )
        settings->setValue("ionizationMode",-1);

    if( ! settings->contains("C13Labeled") )
        settings->setValue("C13Labeled",2);

    if( ! settings->contains("N15Labeled") )
        settings->setValue("N15Labeled",2);

    if( ! settings->contains("isotopeC13Correction") )
        settings->setValue("isotopeC13Correction",2);

    if( ! settings->contains("maxNaturalAbundanceErr") )
        settings->setValue("maxNaturalAbundanceErr",100);

    if( ! settings->contains("maxIsotopeScanDiff") )
        settings->setValue("maxIsotopeScanDiff",10);

    if( ! settings->contains("minIsotopicCorrelation") )
        settings->setValue("minIsotopicCorrelation",0.1);

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
    settings->setValue("closeEvent", 1 );
    projectDockWidget->saveProject();
    writeSettings();
    event->accept();

}

void MainWindow::createMenus() {
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

    QAction* hideWidgets = new QAction(tr("Hide Widgets"), this);
    hideWidgets->setShortcut(tr("F11"));
    connect(hideWidgets, SIGNAL(triggered()), SLOT(hideDockWidgets()));
    widgetsMenu->addAction(hideWidgets);

    QAction* aj = widgetsMenu->addAction("Fragmentation Events List");
    aj->setCheckable(true);  aj->setChecked(false);
    connect(aj,SIGNAL(toggled(bool)),fragmentationEventsWidget,SLOT(setVisible(bool)));

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

    QToolButton *btnOpen = new QToolButton(toolBar);
    btnOpen->setText("Open");
    btnOpen->setIcon(QIcon(rsrcPath + "/fileopen.png"));
    btnOpen->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnOpen->setToolTip(tr("Read in Mass Spec. Files"));

    QToolButton *btnLibrary = new QToolButton(toolBar);
    btnLibrary->setText("Library");
    btnLibrary->setIcon(QIcon(rsrcPath + "/librarymanager.png"));
    btnLibrary->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnLibrary->setToolTip(tr("Library Manager"));

    QToolButton *btnAlign = new QToolButton(toolBar);
    btnAlign->setText("Align");
    btnAlign->setIcon(QIcon(rsrcPath + "/textcenter.png"));
    btnAlign->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnAlign->setToolTip(tr("Align Samples"));

    QToolButton *btnCalibrate = new QToolButton(toolBar);
    btnCalibrate->setText("Calibrate");
    btnCalibrate->setIcon(QIcon(rsrcPath + "/positive_ion.png"));
    btnCalibrate->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnCalibrate->setToolTip(tr("Calibrate"));

    QToolButton *btnDbSearch = new QToolButton(toolBar);
    btnDbSearch->setText("Databases");
    btnDbSearch->setIcon(QIcon(rsrcPath + "/dbsearch.png"));
    btnDbSearch->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnDbSearch->setToolTip(tr("Database Search"));

    QToolButton *btnFeatureDetect = new QToolButton(toolBar);
    btnFeatureDetect->setText("Peaks");
    btnFeatureDetect->setIcon(QIcon(rsrcPath + "/featuredetect.png"));
    btnFeatureDetect->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnFeatureDetect->setToolTip(tr("Feature Detection"));

    QToolButton *btnSpectraMatching = new QToolButton(toolBar);
    btnSpectraMatching->setText("Match");
    btnSpectraMatching->setIcon(QIcon(rsrcPath + "/spectra_search.png"));
    btnSpectraMatching->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnSpectraMatching->setToolTip(tr("Seach Spectra for Fragmentation Patterns"));

    QToolButton *btnSettings = new QToolButton(toolBar);
    btnSettings->setText("Options");
    btnSettings->setIcon(QIcon(rsrcPath + "/settings.png"));
    btnSettings->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnSettings->setToolTip(tr("Change Global Options"));


    connect(btnOpen, SIGNAL(clicked()), SLOT(open()));
    connect(btnLibrary, SIGNAL(clicked()), libraryDialog, SLOT(show()));
    connect(btnAlign,SIGNAL(clicked()), alignmentDialog, SLOT(show()));
    connect(btnCalibrate,SIGNAL(clicked()), calibrateDialog, SLOT(show()));
    connect(btnDbSearch,SIGNAL(clicked()), SLOT(compoundDatabaseSearch()));
    connect(btnFeatureDetect,SIGNAL(clicked()), SLOT(showMassSlices()));
    connect(btnSettings,SIGNAL(clicked()),settingsForm,SLOT(show()));
    connect(btnSpectraMatching,SIGNAL(clicked()),spectraMatchingForm,SLOT(show()));


    toolBar->addWidget(btnOpen);
    toolBar->addWidget(btnLibrary);
    toolBar->addWidget(btnAlign);
    toolBar->addWidget(btnCalibrate);
    toolBar->addWidget(btnDbSearch);
    toolBar->addWidget(btnFeatureDetect);
    toolBar->addWidget(btnSpectraMatching);
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
    quantType->addItem("AreaTop");
    quantType->addItem("Area");
    quantType->addItem("Height");
    quantType->addItem("Retention Time");
    quantType->addItem("Quality");
    quantType->setToolTip("Peak Quntitation Type");
    connect(quantType,SIGNAL(activated(int)),eicWidget,SLOT(replot()));

    adductType = new QComboBox(hBox);
    for(Adduct* a: DB.adductsDB) {
        adductType->addItem(a->name.c_str(),QVariant::fromValue(a));
    }
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


    QToolButton* btnSamples = addDockWidgetButton(sideBar,projectDockWidget,QIcon(rsrcPath + "/samples.png"), "Show Samples Widget (F2)");
    QToolButton* btnLigands = addDockWidgetButton(sideBar,ligandWidget,QIcon(rsrcPath + "/molecule.png"), "Show Compound Widget (F3)");
    QToolButton* btnSpectra = addDockWidgetButton(sideBar,spectraDockWidget,QIcon(rsrcPath + "/spectra.png"), "Show Spectra Widget (F4)");
    QToolButton* btnIsotopes = addDockWidgetButton(sideBar,isotopeWidget,QIcon(rsrcPath + "/isotope.png"), "Show Isotopes Widget (F5)");
    QToolButton* btnFindCompound = addDockWidgetButton(sideBar,massCalcWidget,QIcon(rsrcPath + "/findcompound.png"), "Show Match Compound Widget (F6)");
    QToolButton* btnCovariants = addDockWidgetButton(sideBar,covariantsPanel,QIcon(rsrcPath + "/covariants.png"), "Find Covariants Widget (F7)");
    QToolButton* btnBookmarks = addDockWidgetButton(sideBar,bookmarkedPeaks,QIcon(rsrcPath + "/showbookmarks.png"), "Show Bookmarks (F10)");
    QToolButton* btnGallery = addDockWidgetButton(sideBar,galleryDockWidget,QIcon(rsrcPath + "/gallery.png"), "Show Gallery Widget");
    QToolButton* btnScatter = addDockWidgetButton(sideBar,scatterDockWidget,QIcon(rsrcPath + "/scatterplot.png"), "Show Scatter Plot Widget");
    QToolButton* btnSRM = addDockWidgetButton(sideBar,srmDockWidget,QIcon(rsrcPath + "/qqq.png"), "Show SRM List (F12)");
    QToolButton* btnRconsole = addDockWidgetButton(sideBar,rconsoleDockWidget,QIcon(rsrcPath + "/R.png"), "Show R Console");



    //btnSamples->setShortcut(Qt::Key_F2);
    btnLigands->setShortcut(Qt::Key_F3);
    btnSpectra->setShortcut(Qt::Key_F4);
    btnIsotopes->setShortcut(Qt::Key_F5);
    btnFindCompound->setShortcut(Qt::Key_F6);
    btnCovariants->setShortcut(Qt::Key_F7);
    btnBookmarks->setShortcut(Qt::Key_F10);
    btnSRM->setShortcut(Qt::Key_F12);


    connect(btnSRM,SIGNAL(clicked(bool)),SLOT(showSRMList()));

    sideBar->setOrientation(Qt::Vertical);
    sideBar->setMovable(false);

    sideBar->addWidget(btnSamples);
    sideBar->addWidget(btnLigands);
    sideBar->addWidget(btnSpectra);
    sideBar->addWidget(btnIsotopes);
    sideBar->addWidget(btnFindCompound);
    sideBar->addWidget(btnCovariants);
    sideBar->addWidget(btnSRM);
    sideBar->addWidget(btnGallery);
    sideBar->addWidget(btnScatter);
    sideBar->addWidget(btnRconsole);
    sideBar->addSeparator();
    sideBar->addWidget(btnBookmarks);
    // sideBar->addWidget(btnHeatmap);

    addToolBar(Qt::TopToolBarArea,toolBar);
    addToolBar(Qt::RightToolBarArea,sideBar);
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

void MainWindow::compoundDatabaseSearch() {
    peakDetectionDialog->setFeatureDetection(PeakDetectionDialog::CompoundDB);
    peakDetectionDialog->show();
}

void MainWindow::showSRMList() { 
    vector<mzSlice*>slices = getSrmSlices();
    if (slices.size() ==  0 ) return;
    srmDockWidget->setInfo(slices);
    delete_all(slices);

    //peakDetectionDialog->setFeatureDetection(PeakDetectionDialog::QQQ);
    //peakDetectionDialog->show();
}


void MainWindow::setPeakGroup(PeakGroup* group) {
    if ( group == NULL ) return;

    qDebug() << "MainWindow::setPeakGroup(PeakGroup)" << group;
    searchText->setText(QString::number(group->meanMz,'f',8));

    if ( eicWidget && eicWidget->isVisible() ) {
        eicWidget->setPeakGroup(group);
    }

    if ( isotopeWidget && isotopeWidget->isVisible() && group->compound != NULL ) {
        isotopeWidget->setCompound(group->compound);
    }

    if ( group->compound != NULL) {
        setUrl(group->compound);
    }

    if(fragmenationSpectraWidget->isVisible()) {
        fragmenationSpectraWidget->overlayPeakGroup(group);
    }

    if (massCalcWidget->isVisible()) { massCalcWidget->setPeakGroup(group); }

    /*
    if ( scatterDockWidget->isVisible() ) {
        ((ScatterPlot*)scatterDockWidget)->showSimilar(group);
    }
    */

    if (group->peaks.size() > 0) showPeakInfo(&(group->peaks[0]));
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

vector<mzSlice*> MainWindow::getSrmSlices() {
    QSet<QString>srms;
    //+118.001@cid34.00 [57.500-58.500]
    //+ c ESI SRM ms2 102.000@cid19.00 [57.500-58.500]
    //-87.000 [42.500-43.500]
    //- c ESI SRM ms2 159.000 [113.500-114.500]

    QRegExp rx1a("[+/-](\\d+\\.\\d+)");
    QRegExp rx1b("ms2\\s*(\\d+\\.\\d+)");
    QRegExp rx2("(\\d+\\.\\d+)-(\\d+\\.\\d+)");
    int countMatches=0;

    double amuQ1 = getSettings()->value("amuQ1").toDouble();
    double amuQ3 = getSettings()->value("amuQ3").toDouble();

    vector<mzSlice*>slices;
    for(int i=0; i < samples.size(); i++ ) {
    	mzSample* sample = samples[i];
        for( int j=0; j < sample->scans.size(); j++ ) {
            Scan* scan = sample->getScan(j);
            if (!scan) continue;

            QString filterLine(scan->filterLine.c_str());
            if (filterLine.isEmpty()) continue;

            if (srms.contains(filterLine))  continue;
            srms.insert(filterLine);


            mzSlice* s = new mzSlice(0,0,0,0);
            s->srmId = scan->filterLine.c_str();
            slices.push_back(s);

            //match compounds
            Compound* compound = NULL;
            float precursorMz = scan->precursorMz;
            float productMz   = scan->productMz;
            int   polarity= scan->getPolarity();
            if (polarity==0) filterLine[0] == '+' ? polarity=1 : polarity =-1;
            if (getIonizationMode()) polarity=getIonizationMode(); //user specified ionization mode

            if ( precursorMz == 0 ) {
                if( rx1a.indexIn(filterLine) != -1 ) {
                    precursorMz = rx1a.capturedTexts()[1].toDouble();
                } else if ( rx1b.indexIn(filterLine) != -1 ) {
                    precursorMz = rx1b.capturedTexts()[1].toDouble();
                }
            }

            if (productMz == 0) {
                if ( rx2.indexIn(filterLine) != -1 ) {
                    float lb = rx2.capturedTexts()[1].toDouble();
                    float ub = rx2.capturedTexts()[2].toDouble();
                    productMz = lb+(ub-lb)/2;
                }
            }

            if (precursorMz != 0 && productMz != 0 ) {
                compound = DB.findSpeciesByPrecursor(precursorMz,productMz,polarity,amuQ1,amuQ3);
            }

            /*
            if(!compound) {
            qDebug() <<  "Matching failed: precursorMz=" << precursorMz
                         << " productMz=" << productMz
                         << " polarity=" << polarity;
            }
            */

            if (compound) {
                compound->srmId=filterLine.toStdString();
                s->compound=compound;
                s->rt = compound->expectedRt;
                countMatches++;
            }
        }
        //qDebug() << "SRM mapping: " << countMatches << " compounds mapped out of " << srms.size();
    }
    return slices;
}


void MainWindow::showPeakInfo(Peak* _peak) {
    if (_peak == NULL) return;

    mzSample* sample = _peak->getSample();
    if (sample == NULL) return;

    Scan* scan = sample->getScan(_peak->scan);
    if (scan == NULL) return;

    int ionizationMode = scan->getPolarity();
    if (getIonizationMode()) ionizationMode=getIonizationMode(); //user specified ionization mode

    if (spectraDockWidget->isVisible() && scan) {
        spectraWidget->setScan(_peak);
    }

    if( massCalcWidget->isVisible() ) {
        massCalcWidget->setMass(_peak->peakMz);
        massCalcWidget->setCharge(ionizationMode);
    }

   if ( isotopeWidget->isVisible() ) {
        isotopeWidget->setIonizationMode(ionizationMode);
        isotopeWidget->setCharge(ionizationMode);
        isotopeWidget->setPeak(_peak);
    }

   if (fragmenationSpectraWidget->isVisible()) {
        vector<Scan*>ms2s = _peak->getFragmenationEvents(getUserPPM());
        if (ms2s.size()) fragmenationSpectraWidget->setScan(ms2s[0]);
    }

    if( fragmentationEventsWidget->isVisible() ) {
        showFragmentationScans(_peak->peakMz);
    }
}

void MainWindow::spectaFocused(Peak* _peak) {
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

    if (!fragmentationEventsWidget || fragmentationEventsWidget->isVisible() == false ) return;
    float ppm = getUserPPM();

    if (samples.size() <= 0 ) return;
    fragmentationEventsWidget->clearTree();
    for ( unsigned int i=0; i < samples.size(); i++ ) {
        for (unsigned int j=0; j < samples[i]->scans.size(); j++ ) {
            Scan* s = samples[i]->scans[j];
            if ( s->mslevel > 1 && ppmDist(s->precursorMz,pmz) < ppm ) {
                fragmentationEventsWidget->addScanItem(s);
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

    QToolButton *btnPrint = new QToolButton(toolBar);
    btnPrint->setIcon(QIcon(rsrcPath + "/fileprint.png"));
    btnPrint->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnPrint->setToolTip(tr("Print EIC (Ctr+P)"));
    btnPrint->setShortcut(tr("Ctrl+P"));

    QToolButton *btnPDF = new QToolButton(toolBar);
    btnPDF->setIcon(QIcon(rsrcPath + "/exportpdf.png"));
    btnPDF->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnPDF->setToolTip(tr("Save EIC Image to PDF file"));

    QToolButton *btnPNG = new QToolButton(toolBar);
    btnPNG->setIcon(QIcon(rsrcPath + "/copyPNG.png"));
    btnPNG->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnPNG->setToolTip(tr("Copy EIC Image to Clipboard"));

    QToolButton *btnLast = new QToolButton(toolBar);
    btnLast->setIcon(QIcon(rsrcPath + "/last.png"));
    btnLast->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnLast->setToolTip(tr("History Back (Ctrl+Left)"));
    btnLast->setShortcut(tr("Ctrl+Left"));

    QToolButton *btnNext = new QToolButton(toolBar);
    btnNext->setIcon(QIcon(rsrcPath + "/next.png"));
    btnNext->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnNext->setToolTip(tr("History Forward (Ctrl+Right)"));
    btnNext->setShortcut(tr("Ctrl+Right"));

    QToolButton *btnAutoZoom= new QToolButton(toolBar);
    btnAutoZoom->setCheckable(true);
    btnAutoZoom->setChecked(true);
    btnAutoZoom->setIcon(QIcon(rsrcPath + "/autofocus.png"));
    btnAutoZoom->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnAutoZoom->setToolTip(tr("Auto Zoom. Always center chromatogram on expected retention time!"));

    QToolButton *btnGallary= new QToolButton(toolBar);
    btnGallary->setIcon(QIcon(rsrcPath + "/gallery.png"));
    btnGallary->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnGallary->setToolTip(tr("Show In Gallary"));

    QToolButton *btnMarkGood = new QToolButton(toolBar);
    btnMarkGood->setIcon(QIcon(rsrcPath + "/markgood.png"));
    btnMarkGood->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnMarkGood->setToolTip(tr("Bookmark as Good Group (G)"));

    QToolButton *btnMarkBad = new QToolButton(toolBar);
    btnMarkBad->setIcon(QIcon(rsrcPath + "/markbad.png"));
    btnMarkBad->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnMarkBad->setToolTip(tr("Bookmark as Bad Group (B)"));

    QToolButton *btnCopyCSV = new QToolButton(toolBar);
    btnCopyCSV->setIcon(QIcon(rsrcPath + "/copyCSV.png"));
    btnCopyCSV->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnCopyCSV->setToolTip(tr("Copy Group Information to Clipboard (Ctrl+C)"));
    btnCopyCSV->setShortcut(tr("Ctrl+C"));

    QToolButton *btnBookmark = new QToolButton(toolBar);
    btnBookmark->setIcon(QIcon(rsrcPath + "/bookmark.png"));
    btnBookmark->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnBookmark->setToolTip(tr("Bookmark Group (Ctrl+D)"));
    btnBookmark->setShortcut(tr("Ctrl+D"));


    QToolButton *btnIntegrateArea = new QToolButton(toolBar);
    btnIntegrateArea->setIcon(QIcon(rsrcPath + "/integrateArea.png"));
    btnIntegrateArea->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnIntegrateArea->setToolTip(tr("Manual Integration (Shift+MouseDrag)"));

    QToolButton *btnAverageSpectra = new QToolButton(toolBar);
    btnAverageSpectra->setIcon(QIcon(rsrcPath + "/averageSpectra.png"));
    btnAverageSpectra->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnAverageSpectra->setToolTip(tr("Average Specta (Ctrl+MouseDrag)"));


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
    toolBar->addWidget(btnCopyCSV);
    toolBar->addWidget(btnMarkGood);
    toolBar->addWidget(btnMarkBad);
    toolBar->addWidget(btnGallary);
    toolBar->addSeparator();
    toolBar->addWidget(btnIntegrateArea);
    toolBar->addWidget(btnAverageSpectra);

    toolBar->addWidget(btnLast);
    toolBar->addWidget(btnNext);
    toolBar->addSeparator();

    toolBar->addWidget(btnPDF);
    toolBar->addWidget(btnPNG);
    toolBar->addWidget(btnPrint);
    toolBar->addSeparator();

    toolBar->addWidget(btnAutoZoom);

//    toolBar->addWidget(smoothingWindowBox);

    connect(btnLast,SIGNAL(clicked()), SLOT(historyLast()));
    connect(btnNext,SIGNAL(clicked()), SLOT(historyNext()));
    connect(btnPrint, SIGNAL(clicked()), SLOT(print()));
    connect(btnZoom, SIGNAL(clicked()), eicWidget, SLOT(resetZoom()));
    connect(btnPDF, SIGNAL(clicked()), SLOT(exportPDF()));
    connect(btnPNG, SIGNAL(clicked()), SLOT(exportSVG()));
    connect(btnAutoZoom,SIGNAL(toggled(bool)), eicWidget,SLOT(autoZoom(bool)));
    connect(btnBookmark,SIGNAL(clicked()),  this,  SLOT(bookmarkSelectedPeakGroup()));
    connect(btnCopyCSV,SIGNAL(clicked()),  eicWidget, SLOT(copyToClipboard()));
    connect(btnMarkGood,SIGNAL(clicked()), eicWidget, SLOT(markGroupGood()));
    connect(btnMarkBad,SIGNAL(clicked()),  eicWidget, SLOT(markGroupBad()));
    connect(btnGallary,SIGNAL(clicked()),  eicWidget, SLOT(setGallaryToEics()));
    connect(btnIntegrateArea,SIGNAL(clicked()),  eicWidget, SLOT(startAreaIntegration()));
    connect(btnAverageSpectra,SIGNAL(clicked()),  eicWidget, SLOT(startSpectralAveraging()));

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
    if(!covariantsPanel->isVisible() or ! galleryDockWidget->isVisible()) return;

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
        vector<MassCalculator::Match> matches =  DB.findMathchingCompounds(links[i].mz2, ppm, ionizationMode);
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
        if (type  == "AreaTop") return PeakGroup::AreaTop;
        else if (type  == "Area")    return PeakGroup::Area;
        else if (type  == "Height")  return PeakGroup::Height;
        else if (type  == "Retention Time")  return PeakGroup::RetentionTime;
        else if (type  == "Quality")  return PeakGroup::Quality;
        else if (type  == "S/N Ratio")  return PeakGroup::SNRatio;
    }
    return PeakGroup::AreaTop;
}

void MainWindow::markGroup(PeakGroup* group, char label) {
    if(!group) return;

    group->setLabel(label);
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
    getEicWidget()->recompute();
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
    } else if (filename.contains("mzData",Qt::CaseInsensitive)) {
      return 1;
    } else if (filename.contains("mzML",Qt::CaseInsensitive)) {
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
        cerr << "changeUserAdduct::" << adduct->name << endl;
        massCalcWidget->ionization->setValue(_ionizationMode);

        mzSlice& currentSlice = eicWidget->getMzSlice();
        if(currentSlice.compound) {
            setCompoundFocus(currentSlice.compound);
        }
    }
}

Adduct* MainWindow::getUserAdduct() {
    QVariant v = adductType->currentData();
    Adduct*  adduct =  v.value<Adduct*>();
    if(adduct) {
        cerr << "getUserAdduct::" << adduct->name << endl;
        return adduct;
    } else if (_ionizationMode > 0 ) {
        return MassCalculator::PlusHAdduct;
    } else if (_ionizationMode < 0 ) {
        return MassCalculator::MinusHAdduct;
    } else {
        return MassCalculator::ZeroMassAdduct;
    }
}
