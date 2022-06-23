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
  connect(scoringSchema, SIGNAL(currentIndexChanged(QString)), this, SLOT(showTableCheckRumsDB()));
  connect(this,SIGNAL(visibilityChanged(bool)),this,SLOT(updateDatabaseList()));
  connect(chkReferenceObservedAdductMatch, SIGNAL(toggled(bool)), this, SLOT(showTable()));

   scoringSchema->clear();

   for(string scoringAlgorithm: FragmentationMatchScore::getScoringAlgorithmNames()) {
        scoringSchema->addItem(scoringAlgorithm.c_str());
   }

   scoringSchema->addItem(QString("rumsDB/clamDB"));

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

void MassCalcWidget::showTablerumsDBMatches(PeakGroup *grp) {

    QTreeWidget *p = treeWidget;
    p->setUpdatesEnabled(false);
    p->clear();
    p->setColumnCount(4);
    p->setHeaderLabels( QStringList() << "Compound" << "Adduct"  << "score" << "DB");
    p->setSortingEnabled(false);
    p->setUpdatesEnabled(false);

    int peakGroupId = grp->groupId;

    if (_mw->getProjectWidget()->currentProject){
        pair<matchIterator, matchIterator> rumsDBMatches = _mw->getProjectWidget()->currentProject->allMatches.equal_range(peakGroupId);

        for (matchIterator it = rumsDBMatches.first; it != rumsDBMatches.second; ++it) {

            shared_ptr<mzrollDBMatch> matchInfo = it->second;

            string compoundName = matchInfo->compoundName;
            string adductName = matchInfo->adductName;
            int compoundId = matchInfo->ionId;
            float score = matchInfo->score;

            NumericTreeWidgetItem* item = new NumericTreeWidgetItem(treeWidget, Qt::UserRole);
            item->setData(0, Qt::UserRole, QVariant(compoundId));
            item->setData(1, Qt::UserRole, QString(adductName.c_str()));

            item->setText(0, compoundName.c_str());
            item->setText(1, adductName.c_str());
            item->setText(2, QString::number(score, 'f', 3));
            item->setText(3, "rumsDB/clamDB matches");
        }
    }

    p->sortByColumn(2,Qt::DescendingOrder); //decreasing by score
    p->header()->setStretchLastSection(true);
    p->setSortingEnabled(true);
    p->setUpdatesEnabled(true);
    p->update();

    isInRumsDBMode = true;
}

void MassCalcWidget::showTableCheckRumsDB() {

    PeakGroup *grp = nullptr;

    if (scoringSchema->currentText().toStdString() == "rumsDB/clamDB") {

        if (_mw->projectDockWidget->currentProject) {

            TableDockWidget* table = _mw->findPeakTable("rumsDB");

            if (table) {
                grp = table->getSelectedGroup();
            } else {
                TableDockWidget *table2 = _mw->findPeakTable("clamDB");
                if (table2) {
                    grp = table2->getSelectedGroup();
                }
            }
        }
    }

    if (grp) {
        showTablerumsDBMatches(grp);
    } else {
        showTable();
    }
}

void MassCalcWidget::showTable() {

    string scoringAlgorithm = scoringSchema->currentText().toStdString();

    QTreeWidget *p = treeWidget;
    p->setUpdatesEnabled(false); 
    p->clear(); 
    p->setColumnCount(8);
    p->setHeaderLabels( QStringList() << "Compound" << "Reference Adduct"  << "Observed Adduct" << "Theoretical m/z" << "ppmDiff" <<  "rtDiff" << "Score" << "DB");
    p->setSortingEnabled(false);
    p->setUpdatesEnabled(false);

    for(unsigned int i=0;  i < matches.size(); i++ ) {
        //no duplicates in the list
        Compound* c = matches[i].compoundLink;
        Adduct *  a = matches[i].adductLink;
        QString preMz = QString::number( matches[i].mass , 'f', 4);
        QString ppmDiff = QString::number( matches[i].diff , 'f', 2);
        QString rtDiff = QString::number(matches[i].rtdiff,'f', 1);

        QString matchScore;
        if (scoringAlgorithm == "rumsDB/clamDB") {
            matchScore = "N/A";
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

        //Issue 437: filter hits by adduct matching
        bool isAdductMatch = true;
        if (chkReferenceObservedAdductMatch->isChecked()) {

            //Issue 495: Allow leeway in [M] vs [M]+ vs [M]- matches - treat these as synonyms
            bool compoundMZeroAdduct = (c->adductString == "[M]" || c->adductString == "[M]+" || c->adductString == "[M]-");
            bool adductMZeroAdduct = (c->adductString == "[M]" || c->adductString == "[M]+" || c->adductString == "[M]-");

            if (compoundMZeroAdduct && adductMZeroAdduct) {
                isAdductMatch = true;
            } else {
                isAdductMatch = a->name == c->adductString;
            }

        }

        if (!isAdductMatch) continue;

        NumericTreeWidgetItem* item = new NumericTreeWidgetItem(treeWidget,Qt::UserRole);
        item->setData(0, Qt::UserRole,QVariant(i));

        item->setData(0, CompoundType, QVariant::fromValue(c));
        item->setData(0, AdductType, QVariant::fromValue(a));

        item->setText(0, c->name.c_str());
        item->setText(1, c->adductString.c_str());
        item->setText(2, observedAdductName);
        item->setText(3, preMz);
        item->setText(4, ppmDiff);
        item->setText(5, rtDiff);
        item->setText(6, matchScore);
        item->setText(7, db);
    }

    p->sortByColumn(6, Qt::DescendingOrder); //decreasing by score
    p->header()->setStretchLastSection(true);
    p->setSortingEnabled(true);
    p->setUpdatesEnabled(true);
    p->update();

    isInRumsDBMode = false;
}

void MassCalcWidget::setPeakGroup(PeakGroup* grp) {
     qDebug() << "MassCalcWidget::setPeakGroup()";

    if(!grp) return;
    if (grp->meanMz <= 0.0) return; //Issue 495

    _mz = grp->meanMz;

    matches = DB.findMatchingCompounds(_mz,_ppm,_charge);

    //TODO: this information should be saved in the file, not computed on the fly like this, unless desired by design.
    if (grp->fragmentationPattern.nobs() == 0){
        qDebug() << "MassCalcWidget::setPeakGroup(): grp->computeFragPattern() minMz="
                 << grp->minMz
                 << ", maxMz="
                 << grp->maxMz;
        grp->computeFragPattern(fragmentPPM->value());
    }

    //Issue 503: clear table to avoid staleness issues
    if(grp->fragmentationPattern.nobs() == 0){
        treeWidget->clear();
        treeWidget->update();
        return;
    }

    for(MassCalculator::Match& m: matches ) {
       Compound* cpd = m.compoundLink;
       m.fragScore = cpd->scoreCompoundHit(&grp->fragmentationPattern,fragmentPPM->value(),false);
       if (cpd->expectedRt > 0) m.rtdiff =  grp->meanRt - cpd->expectedRt;

    }

    if (scoringSchema->currentText().toStdString() == "rumsDB/clamDB") {
        showTablerumsDBMatches(grp);
    } else {
        showTable();
    }
}

void MassCalcWidget::setFragment(Fragment * f){
    qDebug() << "MassCalcWidget::setFragment()";
    if (!f) return;

    _mz = f->precursorMz;
    matches = DB.findMatchingCompounds(_mz,_ppm,_charge);
    lineEdit->setText(QString::number(_mz,'f',5));

    PeakGroup *grp = _mw->getEicWidget()->getSelectedGroup();

    for(MassCalculator::Match& m: matches ) {
       Compound* cpd = m.compoundLink;
       m.fragScore = cpd->scoreCompoundHit(f,fragmentPPM->value(),false);
       if (grp){
           m.diff = mzUtils::ppmDist((double) m.mass, (double) grp->meanMz);
           if (cpd->expectedRt > 0){
               m.rtdiff =  grp->meanRt - cpd->expectedRt;
           }
       }
    }

    showTable();

    lineEdit->repaint();
    repaint();
}

void MassCalcWidget::setFragmentationScan(Scan* scan) {
    qDebug() << "MassCalcWidget::setFragmentationScan()";
    if(!scan) return;

    //retrieve settings
    //scan filter
    float minFracIntensity = _mw->getSettings()->value("spnScanFilterMinIntensityFraction", 0).toFloat();
    float minSNRatio = _mw->getSettings()->value("spnScanFilterMinSN", 0).toFloat();
    int maxNumberOfFragments = _mw->getSettings()->value("spnScanFilterRetainTopX", -1).toInt();
    int baseLinePercentile = static_cast<int>(_mw->getSettings()->value("spnScanFilterBaseline", 5).toDouble());
    bool isRetainFragmentsAbovePrecursorMz = _mw->getSettings()->value("chkScanFilterRetainHighMzFragments", true).toBool();
    float precursorPurityPpm = -1; //TODO: parameter in settings?
    float minIntensity = _mw->getSettings()->value("spnScanFilterMinIntensity", 0).toFloat();

    //Issue 189: use settings
    Fragment f(scan,
               minFracIntensity,
               minSNRatio,
               maxNumberOfFragments,
               baseLinePercentile,
               isRetainFragmentsAbovePrecursorMz,
               precursorPurityPpm,
               minIntensity
               );

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
    _mw->setStatusText("Searching pubchem");
	QString requestStr( 
		tr("http://pubchem.ncbi.nlm.nih.gov/search/search.cgi?cmd=search&q_type=mf&q_data=%1&simp_schtp=mf")
    .arg(formula));
    
    _mw->setUrl(requestStr);
}

void MassCalcWidget::keggLink(QString formula){
    _mw->setStatusText("Searching Kegg");
	QString requestStr( 
		tr("http://www.genome.jp/ligand-bin/txtsearch?column=formula&query=%1&DATABASE=compound").arg(formula));
    _mw->setUrl(requestStr);
}

void MassCalcWidget::showInfo() {

    QList<QTreeWidgetItem*> items = treeWidget->selectedItems();
    if (items.size() == 0) {
        return;
    }

    QTreeWidgetItem* item = items.last();
    if(!item) return;

    if (!isInRumsDBMode) {

        if (matches.size() == 0) return;

        QVariant v = item->data(0,Qt::UserRole);
        int mNum = v.toInt();

        //this should not happen, but add this check here to avoid program crashing
        if (mNum < 0 || mNum >= matches.size()) return;

        MassCalculator::Match m = matches[mNum];
        if (m.compoundLink) {
            _mw->getEicWidget()->setCompound(m.compoundLink, m.adductLink, true, false, true);
            _mw->getEicWidget()->drawFocusLines();

            if (!_mw->fragmentationSpectraWidget->getCurrentScan()) {
                _mw->fragmentationSpectraWidget->showMS2CompoundSpectrum(m.compoundLink);
            } else {
                _mw->fragmentationSpectraWidget->overlayCompound(m.compoundLink, false);
            }
        }

    } else if (!_mw->rumsDBDatabaseName.isEmpty()) {

        QVariant compoundId = item->data(0, Qt::UserRole);
        QString adductName = item->data(1, Qt::UserRole).toString();

        Compound *compound = DB.findSpeciesById(to_string(compoundId.toInt()), _mw->rumsDBDatabaseName.toStdString(), _mw->isAttemptToLoadDB);

        if (compound) {


            if (!_mw->fragmentationSpectraWidget->getCurrentScan()) {
                _mw->fragmentationSpectraWidget->showMS2CompoundSpectrum(compound);
            } else {
                _mw->fragmentationSpectraWidget->overlayCompound(compound, false);
            }

            Adduct *adduct = DB.findAdductByName(adductName.toStdString());

            if (adduct) {
                _mw->getEicWidget()->setCompound(compound, adduct, true, false, true);
                _mw->getEicWidget()->drawFocusLines();
            }
        }
    }

}

MassCalcWidget::~MassCalcWidget() { 
    //delete_all(matches);
    matches.resize(0); 
}
