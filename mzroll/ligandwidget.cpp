#include "ligandwidget.h"
#include "numeric_treewidgetitem.h"

using namespace std;

LigandWidget::LigandWidget(MainWindow* mw) {
  _mw = mw;
 
  setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  setFloating(false);
  setObjectName("Compounds");

  treeWidget=new QTreeWidget(this);
  treeWidget->setObjectName(QString::fromUtf8("treeWidget"));
  treeWidget->setSortingEnabled(false);
  treeWidget->setColumnCount(3);
  treeWidget->setRootIsDecorated(false);
  treeWidget->setUniformRowHeights(true);
  treeWidget->setHeaderHidden(false);
  treeWidget->setObjectName("CompoundTable");
  treeWidget->setDragDropMode(QAbstractItemView::DragOnly);

  connect(treeWidget,SIGNAL(itemSelectionChanged()), SLOT(showLigand()));

  QToolBar *toolBar = new QToolBar(this);
  toolBar->setFloatable(false);
  toolBar->setMovable(false);

  databaseSelect = new QComboBox(toolBar);
  databaseSelect->setObjectName(QString::fromUtf8("databaseSelect"));
  databaseSelect->setDuplicatesEnabled(false);
  //databaseSelect->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::MinimumExpanding);

  galleryButton = new QToolButton(toolBar);
  galleryButton->setIcon(QIcon(rsrcPath + "/gallery.png"));
  galleryButton->setToolTip(tr("Show Compounds in Gallery Widget"));
  connect(galleryButton,SIGNAL(clicked()),SLOT(showGallery()));

  connect(this, SIGNAL(compoundFocused(Compound*)), mw, SLOT(setCompoundFocus(Compound*)));
  connect(this, SIGNAL(urlChanged(QString)), mw, SLOT(setUrl(QString)));

  QLineEdit*  filterEditor = new QLineEdit(toolBar);
  filterEditor->setMinimumWidth(10);
  filterEditor->setPlaceholderText("Compound Name Filter");
  connect(filterEditor, SIGNAL(textEdited(QString)), this, SLOT(showMatches(QString)));

  //toolBar->addWidget(new QLabel("Compounds: "));
  toolBar->addWidget(filterEditor);
  toolBar->addWidget(databaseSelect);
  toolBar->addWidget(galleryButton);


  setWidget(treeWidget);
  setTitleBarWidget(toolBar);
  setWindowTitle("Compounds");

}

QString LigandWidget::getDatabaseName() {
	return databaseSelect->currentText();
}

void LigandWidget::updateDatabaseList() {
	databaseSelect->disconnect(SIGNAL(currentIndexChanged(QString)));
    databaseSelect->clear();

    QStringList dbnames = DB.getDatabaseNames();
    for(QString db: dbnames ) {
        databaseSelect->addItem(db);
    }
	connect(databaseSelect, SIGNAL(currentIndexChanged(QString)), this, SLOT(setDatabase(QString)));
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
   int index = databaseSelect->findText(dbname,Qt::MatchExactly);
   if (index != -1 ) databaseSelect->setCurrentIndex(index);

    _mw->getSettings()->setValue("lastCompoundDatabase", getDatabaseName());
    emit databaseChanged(getDatabaseName());
    DB.loadCompoundsSQL(dbname);
    showTable();

    if(!filterString.isEmpty()) showMatches(filterString);
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
	if (c==NULL) return;
	QString dbname(c->db.c_str());
	int index = databaseSelect->findText(dbname,Qt::MatchExactly);
        if (index != -1 ) databaseSelect->setCurrentIndex(index);

    QString filterString(c->name.c_str());
    showTable();
}


void LigandWidget::setFilterString(QString s) { 
	if(s != filterString) { 
        showMatches(s);
	} 
}

void LigandWidget::showMatches(QString needle) { 
    filterString=needle;

    QRegExp regexp(needle,Qt::CaseInsensitive,QRegExp::RegExp);
    if(! regexp.isValid())return;

    QTreeWidgetItemIterator itr(treeWidget);
    while (*itr) {
        QTreeWidgetItem* item =(*itr);
        QVariant v = item->data(0,Qt::UserRole);
        Compound*  compound =  v.value<Compound*>();
        if (compound) {
                item->setHidden(true);
                if (needle.isEmpty()) {
                   item->setHidden(false);
                } else if ( item->text(0).contains(regexp) ){
                   item->setHidden(false);
                } else {
                    QStringList stack;
                    stack << compound->name.c_str()
                          << compound->id.c_str()
                          << compound->formula.c_str();

                    if( compound->category.size()) {
                        for(int i=0; i < compound->category.size(); i++) {
                            stack << compound->category[i].c_str();
                        }
                    }

                    foreach( QString x, stack) {
                        if (x.contains(regexp)) {
                             item->setHidden(false); break;
                        }
                    }
                }
        }
        ++itr;
    }
}


void LigandWidget::updateCurrentItemData() {
    QTreeWidgetItem* item = treeWidget->selectedItems().first();
    if (!item) return;
    QVariant v = item->data(0,Qt::UserRole);
    Compound*  c =  v.value<Compound*>();
    if(!c) return;

    QString mass = QString::number(c->mass);
    QString rt = QString::number(c->expectedRt);
    item->setText(1,mass);
    item->setText(2,rt);

    if (c->hasGroup() ) {
        item->setIcon(0,QIcon(":/images/link.png"));
    } else {
        item->setIcon(0,QIcon());
    }
}


void LigandWidget::showTable() { 
    //	treeWidget->clear();
    treeWidget->clear();
    treeWidget->setColumnCount(4);
    QStringList header; header << "name" << "M" << "rt" << "formula" << "category";
    treeWidget->setHeaderLabels( header );
    treeWidget->setSortingEnabled(false);

    string dbname = databaseSelect->currentText().toStdString();
    cerr << "ligandwidget::showTable() " << dbname << endl;

    for(unsigned int i=0;  i < DB.compoundsDB.size(); i++ ) {
        Compound* compound = DB.compoundsDB[i];
        if(compound->db != dbname ) continue; //skip compounds from other databases
        NumericTreeWidgetItem *parent  = new NumericTreeWidgetItem(treeWidget,CompoundType);

        QString name(compound->name.c_str() );
        parent->setText(0,name.toUpper());  //Feng note: sort names after capitalization
        parent->setText(1,QString::number(compound->mass));
        if(compound->expectedRt > 0) parent->setText(2,QString::number(compound->expectedRt));
        if (compound->formula.length())parent->setText(3,compound->formula.c_str());
        if (compound->smileString.length()) parent->setText(3,compound->smileString.c_str());

        parent->setData(0, Qt::UserRole, QVariant::fromValue(compound));
        parent->setFlags(Qt::ItemIsSelectable|Qt::ItemIsDragEnabled|Qt::ItemIsEnabled);

        if (compound->charge) addItem(parent,"Charge", compound->charge);
        if (compound->precursorMz) addItem(parent,"Precursor Mz", compound->precursorMz);
        if (compound->productMz) addItem(parent,"Product Mz", compound->productMz);
        if (compound->collisionEnergy) addItem(parent,"Collision Energy", compound->collisionEnergy);
        if (!compound->smileString.empty()) addItem(parent,"Smile", compound->smileString);
        if (compound->hasGroup() ) parent->setIcon(0,QIcon(":/images/link.png"));

        if(compound->category.size() > 0) {
            QStringList catList;
            for(string c : compound->category) catList << c.c_str();
            parent->setText(4,catList.join(";"));
        }

        /*
        if (compound->fragment_mzs.size()) {
            QStringList mzList;
            for(unsigned int i=0; i<compound->fragment_mzs.size();i++) {
                mzList << QString::number(compound->fragment_mzs[i],'f',2);
            }
            QTreeWidgetItem* child = addItem(parent,"Fragments",compound->fragment_mzs[0]);
            child->setText(1,mzList.join(";"));
        }
        */

    }
    treeWidget->setSortingEnabled(true);
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

    qDebug() << "LigandWidget::showLigand()";
    foreach(QTreeWidgetItem* item, treeWidget->selectedItems() ) {
            QVariant v = item->data(0,Qt::UserRole);
            Compound*  c =  v.value<Compound*>();
            if (c)  _mw->setCompoundFocus(c);

    }
}

