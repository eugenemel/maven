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
  connect(mTable, SIGNAL(currentCellChanged(int,int,int,int)), SLOT(showCellInfo(int,int,int,int)));
}

void MassCalcWidget::setMass(float mz) { 
	if ( mz == _mz ) return;

    lineEdit->setText(QString::number(mz,'f',5)); 
    _mz = mz; 
    //delete_all(matches);
	matches.resize(0); //clean up
    getMatches();
    showTable();
}

void MassCalcWidget::setCharge(float charge) { 
    ionization->setValue(charge);
    _charge=charge;
}
void MassCalcWidget::setPPM(float diff) { maxppmdiff->setValue(diff); _ppm=diff; }

void MassCalcWidget::compute() {
	 bool isDouble =false;
	 _mz = 		lineEdit->text().toDouble(&isDouble);
  	 _charge =  ionization->value();
  	 _ppm = 	maxppmdiff->value();
     
	 if (!isDouble) return;
	 cerr << "massCalcGui:: compute() " << _charge << " " << _mz << endl;

     //delete_all(matches);
	 matches.resize(0); //clean up

    matches = DB.findMathchingCompounds(_mz,_ppm,_charge);

    if (_mz>0 and matches.size()==0) {
        _mw->setStatusText("Searching for formulas..");
        //matches=mcalc.enumerateMasses(_mz,_charge,_ppm);
    }


    _mw->setStatusText(tr("Found %1 formulas").arg(matches.size()));

     showTable(); 
}

void MassCalcWidget::showTable() {
        
    QTableWidget *p = mTable; 
    p->setUpdatesEnabled(false); 
    p->clear(); 
    p->setColumnCount(3);
    p->setRowCount( matches.size() ) ;
    p->setHorizontalHeaderLabels(  QStringList() << "Compound" << "Mass" << "ppmDiff");
    p->setSortingEnabled(false);
    p->setUpdatesEnabled(false);
    
    for(unsigned int i=0;  i < matches.size(); i++ ) {
        //no duplicates in the list
        Compound* c = matches[i].compoundLink;
        QString item1 = QString(matches[i].name.c_str() );
        QString item2 = QString::number( matches[i].mass , 'f', 4);
        QString item3 = QString::number( matches[i].diff , 'f', 4);

        QTableWidgetItem* item = new QTableWidgetItem(item1,0);

        p->setItem(i,0, item );
        p->setItem(i,1, new QTableWidgetItem(item2,0));
        p->setItem(i,2, new QTableWidgetItem(item3,0)); 
		if (c != NULL) item->setData(Qt::UserRole,QVariant::fromValue(c));
    }

    p->setSortingEnabled(true); 
    p->setUpdatesEnabled(true);
    p->update();

}

QSet<Compound*> MassCalcWidget::findMathchingCompounds(float mz, float ppm, float charge) {

	QSet<Compound*>uniqset;
    MassCalculator mcalc;
    Compound x("find", "", "",0);
    x.mass = mz-2;
    vector<Compound*>::iterator itr = lower_bound(DB.compoundsDB.begin(), DB.compoundsDB.end(), &x, Compound::compMass);

	//cerr << "findMathchingCompounds() mz=" << mz << " ppm=" << ppm << " charge=" <<  charge;
    for(;itr != DB.compoundsDB.end(); itr++ ) {
        Compound* c = *itr; if (!c) continue;
        double cmass = mcalc.computeMass(c->formula, charge);
        if ( mzUtils::ppmDist((double) cmass, (double) mz) < ppm && !uniqset.contains(c) ) uniqset << c;
        if (cmass > mz+2) break;
	}
	return uniqset;
}

void MassCalcWidget::getMatches() {
    matches = DB.findMathchingCompounds(_mz,_ppm,_charge);
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

void MassCalcWidget::showCellInfo(int row, int col, int lrow, int lcol) {

    int mNum = row-1; //header row
    if (matches.size() == 0 or mNum<0 or mNum>matches.size()) return;

    MassCalculator::Match m = matches[mNum];
    if(m.mass)  _mw->getEicWidget()->setMzSlice(m.mass);

    if (m.compoundLink) {
        _mw->fragmenationSpectraWidget->overlayCompound(m.compoundLink);
    }
}

MassCalcWidget::~MassCalcWidget() { 
    //delete_all(matches);
    matches.resize(0); 
}
