#include "ligandwidget.h"
#include "numeric_treewidgetitem.h"

using namespace std;

class LigandWidgetTreeBuilder;

LigandWidget::LigandWidget(MainWindow* mw) {
  _mw = mw;
 
  setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  setFloating(false);
  setObjectName("Compounds");

  ligandWidgetTreeBuilder = nullptr;

  treeWidget=new QTreeWidget(this);
  treeWidget->setObjectName(QString::fromUtf8("treeWidget"));
  treeWidget->setSortingEnabled(false);

  setupHeader();

  treeWidget->setRootIsDecorated(false);
  treeWidget->setUniformRowHeights(true);
  treeWidget->setHeaderHidden(false);
  treeWidget->setObjectName("CompoundTable");
  treeWidget->setDragDropMode(QAbstractItemView::DragOnly);

  connect(treeWidget,SIGNAL(itemSelectionChanged()), SLOT(showLigand()));

  filteringProgressBarLbl = new QLabel(INITIAL_PRG_MSG);

  filteringProgressBar = new QProgressBar();
  filteringProgressBar->setRange(0, 100);
  filteringProgressBar->setValue(0);

  QToolBar *toolBar = new QToolBar(this);
  toolBar->setFloatable(false);
  toolBar->setMovable(false);

  QToolBar *toolBar2 = new QToolBar(this);
  toolBar2->setFloatable(false);
  toolBar2->setMovable(false);

  databaseSelect = new QComboBox(toolBar);
  databaseSelect->setObjectName(QString::fromUtf8("databaseSelect"));
  databaseSelect->setDuplicatesEnabled(false);
  connect(databaseSelect, SIGNAL(currentIndexChanged(QString)), this, SLOT(setDatabase(QString)));
  //databaseSelect->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::MinimumExpanding);

  galleryButton = new QToolButton(toolBar);
  galleryButton->setIcon(QIcon(rsrcPath + "/gallery.png"));
  galleryButton->setToolTip(tr("Show Compounds in Gallery Widget"));
  connect(galleryButton,SIGNAL(clicked()),SLOT(showGallery()));

  connect(this, SIGNAL(compoundFocused(Compound*)), mw, SLOT(setCompoundFocus(Compound*)));
  connect(this, SIGNAL(urlChanged(QString)), mw, SLOT(setUrl(QString)));

  filterEditor = new QLineEdit(toolBar);
  filterEditor->setMinimumWidth(250);
  filterEditor->setPlaceholderText("Compound Name Filter");

  //Issue 250: TODO: button sizing may not look great on other systems
  toolBar->setLayout(new QFormLayout());
  btnSubmit = new QPushButton("go");
  btnSubmit->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

  QFontMetrics fm(btnSubmit->font());

    //Issue 286: horizontalAdvance() was not introduced until Qt 5.11
  int txtSize = 0;

#if QT_VERSION_MAJOR >= 5 && QT_VERSION_MINOR >= 11
  txtSize = fm.horizontalAdvance(btnSubmit->text());
#else
  txtSize = fm.width(btnSubmit->text());
#endif

  //Issue 376: Fix dangling pointer issue
  connect(_mw->libraryDialog, SIGNAL(unloadLibrarySignal(QString)), this, SLOT(unloadLibrary(QString)));

  int btnTextPadding = 25;
  btnSubmit->setMaximumWidth(txtSize+btnTextPadding);

  connect(filterEditor, SIGNAL(returnPressed()), this, SLOT(rebuildCompoundTree()));
  connect(btnSubmit, SIGNAL(clicked()), this, SLOT(rebuildCompoundTree()));

  //toolBar->addWidget(new QLabel("Compounds: "));
  toolBar->addWidget(filterEditor);
  toolBar->addWidget(btnSubmit);
  toolBar->addWidget(databaseSelect);
  toolBar->addWidget(galleryButton);

  toolBar2->addWidget(filteringProgressBarLbl);
  toolBar2->addWidget(filteringProgressBar);

  QMainWindow *titleBarWidget = new QMainWindow();
  titleBarWidget->addToolBar(toolBar);
  titleBarWidget->addToolBarBreak();
  titleBarWidget->addToolBar(toolBar2);

  setWidget(treeWidget);
  setTitleBarWidget(titleBarWidget);
  setWindowTitle("Compounds");
}

QString LigandWidget::getDatabaseName() {
	return databaseSelect->currentText();
}

void LigandWidget::updateDatabaseList() {
	databaseSelect->disconnect(SIGNAL(currentIndexChanged(QString)));
    databaseSelect->clear();

    QStringList dbnames = DB.getDatabaseNames();

    databaseSelect->addItem(SELECT_DB);

    for(QString db: dbnames ) {
        databaseSelect->addItem(db);
    }
	connect(databaseSelect, SIGNAL(currentIndexChanged(QString)), this, SLOT(setDatabase(QString)));
}

void LigandWidget::unloadLibrary(QString db){
    if (db == "ALL" || databaseSelect->currentText() == db) {
        for (int i = 0; i < databaseSelect->count(); i++) {
            if (databaseSelect->itemText(i) == SELECT_DB) {
                databaseSelect->setCurrentIndex(i);
                break;
            }
        }
        setDatabase(SELECT_DB);
        filterEditor->setText("");
        repaint();
    }
}

QTreeWidgetItem* LigandWidget::addItem(QTreeWidgetItem* parentItem, string key , float value) {
	QTreeWidgetItem *item = new QTreeWidgetItem(parentItem);
	item->setText(0, QString(key.c_str()));
	item->setText(1, QString::number(value,'f',3));
	return item;
}

QTreeWidgetItem* LigandWidget::addItem(QTreeWidgetItem* parentItem, string key , string value) {
	QTreeWidgetItem *item = new QTreeWidgetItem(parentItem);
	item->setText(0, QString(key.c_str()));
	item->setText(1, QString(value.c_str()));
	return item;
}

void LigandWidget::setDatabase(QString dbname) {

    if (SELECT_DB == dbname) {
        visibleCompounds.clear();
        treeWidget->clear();
        filteringProgressBarLbl->setText(INITIAL_PRG_MSG);
        return;
    }

   int index = databaseSelect->findText(dbname,Qt::MatchExactly);

   if (index != -1 ) {
       //Issue 477: setCurrentIndex() will call LigandWidget::setDatabase() again,
       //here we want to change the value of the index without triggering any downstream events
       databaseSelect->blockSignals(true);
       databaseSelect->setCurrentIndex(index);
       databaseSelect->blockSignals(false);
   }

    _mw->getSettings()->setValue("lastCompoundDatabase", getDatabaseName());

    emit(databaseChanged(getDatabaseName()));

    DB.loadCompoundsSQL(dbname,DB.getLigandDB());

    rebuildCompoundTree();

    emit(_mw->libraryDialog->loadLibrarySignal(getDatabaseName()));
}

void LigandWidget::databaseChanged(int index) {
    QString dbname = databaseSelect->currentText();
    setDatabase(dbname);
    setDatabaseAltered(dbname, alteredDatabases[dbname]);
}

void LigandWidget::setDatabaseAltered(QString name, bool altered) {
    alteredDatabases[name]=altered;
    QString dbname = databaseSelect->currentText();
}

void LigandWidget::setCompoundFocus(Compound* c) {
    if (!c) return;
    if (SELECT_DB == getDatabaseName()) return;
	QString dbname(c->db.c_str());
	int index = databaseSelect->findText(dbname,Qt::MatchExactly);
        if (index != -1 ) databaseSelect->setCurrentIndex(index);

    filterString = QString(c->name.c_str());
    rebuildCompoundTree();
}

void LigandWidget::updateProgressGUI(int progress, QString label) {
    filteringProgressBar->setValue(progress);
    filteringProgressBarLbl->setText(label);
}

void LigandWidget::addCompound(Compound* compound) {
    visibleCompounds.push_back(compound);
}

void LigandWidget::toggleEnabling(bool isEnabled){
    databaseSelect->setEnabled(isEnabled);
    filterEditor->setEnabled(isEnabled);
    _mw->btnLibrary->setEnabled(isEnabled);
    btnSubmit->setEnabled(isEnabled);
}

void LigandWidget::rebuildCompoundTree() {

    if (SELECT_DB == getDatabaseName()) return;

    int maxSteps = DB.getLoadedDatabaseCount(getDatabaseName());

    qDebug() << "LigandWidget::rebuildCompoundTree(): Currently loaded database has a total of" << maxSteps << "compounds.";

    filterString = filterEditor->text();
    filteringProgressBar->setRange(0, maxSteps);

    treeWidget->clear();
    visibleCompounds.clear();

    if (ligandWidgetTreeBuilder) {

        if (ligandWidgetTreeBuilder->isRunning()){
            ligandWidgetTreeBuilder->quit();
        }

        delete(ligandWidgetTreeBuilder);
    }

    ligandWidgetTreeBuilder = new LigandWidgetTreeBuilder(this);

    connect(ligandWidgetTreeBuilder, SIGNAL(updateProgress(int, QString)), this, SLOT(updateProgressGUI(int, QString)));
    connect(ligandWidgetTreeBuilder, SIGNAL(sendCompoundToTree(Compound*)), this, SLOT(addCompound(Compound*)));
    connect(ligandWidgetTreeBuilder, SIGNAL(toggleEnabling(bool)), this, SLOT(toggleEnabling(bool)));
    connect(ligandWidgetTreeBuilder, SIGNAL(completed()), this, SLOT(showTable()));

    ligandWidgetTreeBuilder->start();
}


void LigandWidget::updateCurrentItemData() {
    QTreeWidgetItem* item = treeWidget->selectedItems().first();
    if (!item) return;
    QVariant v = item->data(0,Qt::UserRole);
    Compound*  c =  v.value<Compound*>();
    if(!c) return;

    QString mass = QString::number(c->getExactMass());
    QString rt = QString::number(c->expectedRt);
    item->setText(1,mass);
    item->setText(2,rt);

    if (c->hasGroup() ) {
        item->setIcon(0,QIcon(":/images/link.png"));
    } else {
        item->setIcon(0,QIcon());
    }
}


void LigandWidget::setupHeader() {

    QStringList header;
    header << "Name"
           << "Adduct"
           << "Exact Mass"
           << "Precursor m/z"
           << "Product m/z"
           << "RT"
           << "Formula"
           << "SMILES"
           << "Category";

    treeWidget->setColumnCount(header.size());
    treeWidget->setHeaderLabels(header);
}

void LigandWidget::showTable() { 

    setupHeader();
    treeWidget->setSortingEnabled(false);

    string dbname = databaseSelect->currentText().toStdString();
    qDebug() << "ligandwidget::showTable():" << dbname.c_str() << "# compounds=" << visibleCompounds.size();

    for (auto compound : visibleCompounds) {

        NumericTreeWidgetItem *parent  = new NumericTreeWidgetItem(treeWidget,CompoundType);

        QString name(compound->name.c_str() );

        if (compound->hasGroup() ) parent->setIcon(0,QIcon(":/images/link.png"));
        parent->setText(0, name); //Issue 246: capitalizing names is annoying for lipid libraries, do not mutate name

        parent->setText(1, compound->adductString.c_str());
        parent->setText(2, QString::number(compound->getExactMass(), 'f', 4));
        if(compound->precursorMz > 0.0f) parent->setText(3, QString::number(compound->precursorMz, 'f', 4));
        if (compound->productMz > 0.0f) parent->setText(4,QString::number(compound->productMz, 'f', 4));
        if(compound->expectedRt > 0) parent->setText(5,QString::number(compound->expectedRt));
        if (compound->formula.length()) parent->setText(6,compound->formula.c_str());
        if (compound->smileString.length()) parent->setText(7,compound->smileString.c_str());

        if(compound->category.size() > 0) {
            QStringList catList;
            for(string c : compound->category) catList << c.c_str();
            parent->setText(8, catList.join(";"));
        }

        parent->setData(0, Qt::UserRole, QVariant::fromValue(compound));
        parent->setFlags(Qt::ItemIsSelectable|Qt::ItemIsDragEnabled|Qt::ItemIsEnabled);

    }
    treeWidget->setSortingEnabled(true);

    repaint();
    treeWidget->repaint();
}

void LigandWidget::saveCompoundList(){

    QSettings *settings = _mw->getSettings();
    string dbname = databaseSelect->currentText().toStdString();
    QString dbfilename = databaseSelect->currentText() + ".tab";
    QString dataDir = settings->value("dataDir").value<QString>();
    QString methodsFolder =     dataDir +  "/"  + settings->value("methodsFolder").value<QString>();

    QString fileName = QFileDialog::getSaveFileName(
                this, "Export Compounds to Filename", methodsFolder, "TAB (*.tab)");

   if (fileName.isEmpty()) return;

   QString SEP="\t";
   if (fileName.endsWith(".csv",Qt::CaseInsensitive)) {
       SEP=",";
   } else if (!fileName.endsWith(".tab",Qt::CaseInsensitive)) {
       fileName = fileName + tr(".tab");
       SEP="\t";
   }

    QFile data(fileName);
    if (data.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream out(&data);

        //header
        out << "polarity" << SEP;
        out << "compound" << SEP;
        out << "precursorMz" << SEP;
        out << "collisionEnergy" << SEP;
        out << "productMz" << SEP;
        out << "expectedRt" << SEP;
        out << "id" << SEP;
        out << "formula" << SEP;
        out << "srmId" << SEP;
        out << "category" << endl;

        for(unsigned int i=0;  i < DB.compoundsDB.size(); i++ ) {
            Compound* compound = DB.compoundsDB[i];
            if(compound->db != dbname ) continue;

            QString charpolarity;
            if (compound->charge > 0) charpolarity = "+";
            if (compound->charge < 0) charpolarity = "-";

            QStringList category;

            for(int i=0; i < compound->category.size(); i++) {
                category << QString(compound->category[i].c_str());
            }

            out << charpolarity << SEP;
            out << QString(compound->name.c_str()) << SEP;
            out << compound->precursorMz  << SEP;
            out << compound->collisionEnergy << SEP;
            out << compound->productMz    << SEP;
            out << compound->expectedRt   << SEP;
            out << compound->id.c_str() <<  SEP;
            out << compound->formula.c_str() << SEP;
            out << compound->srmId.c_str() << SEP;
            out << category.join(";") << SEP;
            out << "\n";
            // out << QString(compound->category)
        }
        setDatabaseAltered(databaseSelect->currentText(),false);
    }
}


void LigandWidget::showGallery() {

	vector<Compound*>matches;
    QTreeWidgetItemIterator itr(treeWidget);
    int count=0;
    while (*itr) {
        QTreeWidgetItem* item =(*itr);
        QVariant v = item->data(0,Qt::UserRole);
        Compound*  compound =  v.value<Compound*>();
        matches.push_back(compound);
        if (count++ > 200) break;
        ++itr;
    }

    //qDebug() << "  showGallery()" << matches.size();
	if (matches.size() > 0) {
		_mw->galleryWidget->clear();
		_mw->galleryWidget->addEicPlots(matches);
		_mw->galleryDockWidget->show();
        _mw->galleryDockWidget->raise();

	}
}

void LigandWidget::showNext() {
	QTreeWidgetItem * item 	=treeWidget->currentItem();
	if (item && treeWidget->itemBelow(item) ) {
			treeWidget->setCurrentItem(treeWidget->itemBelow(item));
	}
}

void LigandWidget::showLast() {
	QTreeWidgetItem * item 	=treeWidget->currentItem();
	if (item && treeWidget->itemAbove(item) ) {
			treeWidget->setCurrentItem(treeWidget->itemAbove(item));
	}
}

void LigandWidget::showLigand() {
	if (!_mw) return;

    //Issue 376
    _mw->unsetLastSelectedPeakGroup();

    qDebug() << "LigandWidget::showLigand()";
    foreach(QTreeWidgetItem* item, treeWidget->selectedItems() ) {
            QVariant v = item->data(0,Qt::UserRole);
            Compound*  c =  v.value<Compound*>();
            if (c) {
                //Issue 488: handled in MainWindow::setCompoundFocus() call

//                if (!c->adductString.empty()) {
//                    Adduct *adduct = DB.findAdductByName(c->adductString);
//                    if (adduct) {
//                        _mw->setAdductFocus(adduct);
//                    }

//                }
                _mw->setCompoundFocus(c);
            }
    }
}

void LigandWidgetTreeBuilder::run(void) {

    int matchCount = 0;
    int progressCount = 0;

    QRegExp regexp(ligandWidget->filterString, Qt::CaseInsensitive, QRegExp::FixedString);
    if(! regexp.isValid()) return;

    string dbname = ligandWidget->databaseSelect->currentText().toStdString();

    emit(toggleEnabling(false));

    for(unsigned int i=0;  i < DB.compoundsDB.size(); i++ ) {

        Compound* compound = DB.compoundsDB[i];

        if(compound->db != dbname ) continue;

        progressCount++;

        QString categoryString;
        for (string cat : compound->category) {
         categoryString.append(cat.c_str());
        }

        if (   ligandWidget->filterString.isEmpty() ||                      // unfiltered tree
               QString(compound->name.c_str()).contains(regexp) ||          // name
               QString(compound->adductString.c_str()).contains(regexp) ||  // adduct string
               QString(compound->formula.c_str()).contains(regexp) ||       // formula
               categoryString.contains(regexp)                              // category
               ){
           matchCount++;

           //Issue 446: debugging
           //qDebug() << "LigandWidgetTreeBuilder::run() i=" << i << ": " << compound->name.c_str();

           emit(sendCompoundToTree(compound));
       }

        QString resultsString("");
        if (!ligandWidget->filterString.isEmpty()){
            resultsString.append("Found ");
            resultsString.append(QString::number(matchCount));
            resultsString.append(" compounds...");
        }
        emit(updateProgress(progressCount, resultsString));
    }

    QString resultsString("Showing ");
    resultsString.append(QString::number(matchCount));
    resultsString.append(" compounds.");

    emit(updateProgress(0, resultsString));
    emit(toggleEnabling(true));
    emit(completed());

    quit();
}


