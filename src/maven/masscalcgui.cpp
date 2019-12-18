#include "masscalcgui.h"
using namespace std;


MassCalcWidget::MassCalcWidget(MainWindow* mw) {
  setupUi(this);
  _mw = mw;
  _mz = 0;
  setCharge(-1);
  setPPM(5);

  connect(computeButton, SIGNAL(clicked(bool)), SLOT(compute()));
  connect(lineEdit,SIGNAL(returnPressed()),SLOT(compute()));
  connect(ionization,SIGNAL(valueChanged(double)),SLOT(compute()));
  connect(maxppmdiff,SIGNAL(valueChanged(double)),SLOT(compute()));
  connect(treeWidget,SIGNAL(itemSelectionChanged()), SLOT(showInfo()));
  connect(databaseSelect, SIGNAL(currentIndexChanged(QString)), this, SLOT(showTable()));
  connect(scoringSchema, SIGNAL(currentIndexChanged(QString)), this, SLOT(showTable()));
  connect(this,SIGNAL(visibilityChanged(bool)),this,SLOT(updateDatabaseList()));

   scoringSchema->clear();

   scoringSchema->addItem(QString("rumsDB"));

   for(string scoringAlgorithm: FragmentationMatchScore::getScoringAlgorithmNames()) {
        scoringSchema->addItem(scoringAlgorithm.c_str());
   }


}


void MassCalcWidget::setMass(float mz) { 
    if ( abs(mz-_mz)<0.0005) return;

    lineEdit->setText(QString::number(mz,'f',5)); 
    _mz = mz; 
    //delete_all(matches);
	matches.resize(0); //clean up
    getMatches();

    cerr << "[MassCalcWidget::setPeakGroup()] setMass() call" << endl;
    showTable();
}

void MassCalcWidget::setCharge(float charge) { 
    ionization->setValue(charge);
    _charge=charge;
}

void MassCalcWidget::updateDatabaseList() {
    qDebug() << "updateDatabaseList()";
    databaseSelect->clear();
    QStringList dbnames = DB.getLoadedDatabaseNames();
    databaseSelect->addItem("All");
    for(QString db: dbnames ) {
        databaseSelect->addItem(db);
    }
}

void MassCalcWidget::setPPM(float diff) { maxppmdiff->setValue(diff); _ppm=diff; }

void MassCalcWidget::compute() {
    qDebug() << "MassCalcWidget::compute()";

	 bool isDouble =false;
	 _mz = 		lineEdit->text().toDouble(&isDouble);
  	 _charge =  ionization->value();
  	 _ppm = 	maxppmdiff->value();
     
	 if (!isDouble) return;
	 cerr << "massCalcGui:: compute() " << _charge << " " << _mz << endl;

     //delete_all(matches);
	 matches.resize(0); //clean up

    matches = DB.findMatchingCompounds(_mz,_ppm,_charge);

    if (_mz>0 and matches.size()==0) {
        _mw->setStatusText("Searching for formulas..");
        //matches=mcalc.enumerateMasses(_mz,_charge,_ppm);
    }


    _mw->setStatusText(tr("Found %1 compounds").arg(matches.size()));

     showTable(); 
}

void MassCalcWidget::showTable() {
        
    QTreeWidget *p = treeWidget;
    p->setUpdatesEnabled(false); 
    p->clear(); 
    p->setColumnCount(8);
    p->setHeaderLabels( QStringList() << "Compound" << "Reference Adduct"  << "rtDiff" << "ppmDiff" << "fragScore" << "Observed Adduct" << "Mass" << "DB");
    p->setSortingEnabled(false);
    p->setUpdatesEnabled(false);

    string scoringAlgorithm = scoringSchema->currentText().toStdString();

    for(unsigned int i=0;  i < matches.size(); i++ ) {
        //no duplicates in the list
        Compound* c = matches[i].compoundLink;
        Adduct *  a = matches[i].adductLink;
        QString preMz = QString::number( matches[i].mass , 'f', 4);
        QString ppmDiff = QString::number( matches[i].diff , 'f', 2);
        QString rtDiff = QString::number(matches[i].rtdiff,'f', 1);

        QString matchScore;
        if (scoringAlgorithm == "rumsDB") {
            //retrieve score from database instead of from frag score
            matchScore = "banana";
        } else {
            matchScore = QString::number( matches[i].fragScore.getScoreByName(scoringAlgorithm) , 'f', 3);
        }

        QString observedAdductName = QString("");
        if (a && !a->name.empty()) {
            observedAdductName = QString(a->name.c_str());
        }

        QString db = QString("");
        if (c && !c->db.empty()){
            db = QString(c->db.c_str());
        }

        //filter hits by database
        if (databaseSelect->currentText() != "All" and databaseSelect->currentText() != db) continue;

        NumericTreeWidgetItem* item = new NumericTreeWidgetItem(treeWidget,Qt::UserRole);
        item->setData(0,Qt::UserRole,QVariant(i));

        item->setText(0, c->name.c_str());
        item->setText(1, c->adductString.c_str());
        item->setText(2, rtDiff);
        item->setText(3, ppmDiff);
        item->setText(4, matchScore);
        item->setText(5, observedAdductName);
        item->setText(6, preMz);
        item->setText(7, db);
    }

    p->sortByColumn(4,Qt::DescendingOrder); //decreasing by score
    p->header()->setStretchLastSection(true);
    p->setSortingEnabled(true);
    p->setUpdatesEnabled(true);
    p->update();

}

void MassCalcWidget::setPeakGroup(PeakGroup* grp) {
     qDebug() << "MassCalcWidget::setPeakGroup()";

    if(!grp) return;

    _mz = grp->meanMz;
    matches = DB.findMatchingCompounds(_mz,_ppm,_charge);

    if (grp->fragmentationPattern.nobs() == 0){
        grp->computeFragPattern(fragmentPPM->value());
    }

    if(grp->fragmentationPattern.nobs() == 0) return;

    for(MassCalculator::Match& m: matches ) {
       Compound* cpd = m.compoundLink;
       m.fragScore = cpd->scoreCompoundHit(&grp->fragmentationPattern,fragmentPPM->value(),false);
       if (cpd->expectedRt > 0) m.rtdiff =  grp->meanRt - cpd->expectedRt;

    }

    showTable();
}

void MassCalcWidget::setFragmentationScan(Scan* scan) {
    qDebug() << "MassCalcWidget::setFragmentationScan()" << endl;
    if(!scan) return;

    Fragment f(scan,0,0,1024);

    _mz = scan->precursorMz;
    matches = DB.findMatchingCompounds(_mz,_ppm,_charge);
    lineEdit->setText(QString::number(_mz,'f',5));

    for(MassCalculator::Match& m: matches ) {
       Compound* cpd = m.compoundLink;
       m.fragScore = cpd->scoreCompoundHit(&f,fragmentPPM->value(),false);
    }

    showTable();
}


void MassCalcWidget::getMatches() {
    matches = DB.findMatchingCompounds(_mz,_ppm,_charge);
}

void MassCalcWidget::pubChemLink(QString formula){
	_mw->setStatusText("Searhing pubchem");
	QString requestStr( 
		tr("http://pubchem.ncbi.nlm.nih.gov/search/search.cgi?cmd=search&q_type=mf&q_data=%1&simp_schtp=mf")
    .arg(formula));
    
    _mw->setUrl(requestStr);
}

void MassCalcWidget::keggLink(QString formula){
	_mw->setStatusText("Searhing Kegg");
	QString requestStr( 
		tr("http://www.genome.jp/ligand-bin/txtsearch?column=formula&query=%1&DATABASE=compound").arg(formula));
    _mw->setUrl(requestStr);
}

void MassCalcWidget::showInfo() {
    if (matches.size() == 0) return;

    QList<QTreeWidgetItem*> items = treeWidget->selectedItems();
    if (items.size() == 0) {
        return;
    }

    QTreeWidgetItem* item = items.last();
    if(!item) return;

    QVariant v = item->data(0,Qt::UserRole);
    int mNum = v.toInt();

    MassCalculator::Match m = matches[mNum];
    if (m.compoundLink) {
        _mw->getEicWidget()->setCompound(m.compoundLink,m.adductLink);
        _mw->fragmentationSpectraWidget->overlayCompound(m.compoundLink);
    }
}

MassCalcWidget::~MassCalcWidget() { 
    //delete_all(matches);
    matches.resize(0); 
}
