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
    if ( abs(mz-_mz)<0.0005) return;

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
    p->setColumnCount(6);
    p->setRowCount( matches.size() ) ;
    p->setHorizontalHeaderLabels(  QStringList() << "Compound" << "ppmDiff" << "fragScore" << "Adduct" << "Mass" << "DB");
    p->setSortingEnabled(false);
    p->setUpdatesEnabled(false);
    
    for(unsigned int i=0;  i < matches.size(); i++ ) {
        //no duplicates in the list
        Compound* c = matches[i].compoundLink;
        Adduct *  a = matches[i].adductLink;
        QString matchName = QString(matches[i].name.c_str() );
        QString preMz = QString::number( matches[i].mass , 'f', 4);
        QString ppmDiff = QString::number( matches[i].diff , 'f', 4);
        QString matchScore = QString::number( matches[i].fragScore.weightedDotProduct , 'f', 3);
        QString adductName = QString(a->name.c_str());
        QString db = QString(c->db.c_str());

        QTableWidgetItem* item = new QTableWidgetItem(matchName,0);

        p->setItem(i,0, item );
        p->setItem(i,1, new QTableWidgetItem(ppmDiff,0));
        p->setItem(i,2, new QTableWidgetItem(matchScore));
        p->setItem(i,3, new QTableWidgetItem(adductName,0));
        p->setItem(i,4, new QTableWidgetItem(preMz,0));
        p->setItem(i,5, new QTableWidgetItem(db,0));


        if (c != NULL) item->setData(Qt::UserRole,QVariant::fromValue(c));
    }

    p->setSortingEnabled(true); 
    p->setUpdatesEnabled(true);
    p->update();

}

void MassCalcWidget::setPeakGroup(PeakGroup* grp) {
    if(!grp) return;

    _mz = grp->meanMz;
    matches = DB.findMathchingCompounds(_mz,_ppm,_charge);
    cerr << "MassCalcWidget::setPeakGroup(PeakGroup* grp)" << endl;

    if(grp->ms2EventCount == 0) grp->computeFragPattern(_ppm);
    if(grp->fragmentationPattern.nobs() == 0) return;
    float productPpmTolr=20;

    for(MassCalculator::Match& m: matches ) {
       Compound* cpd = m.compoundLink;
       m.fragScore = cpd->scoreCompoundHit(&grp->fragmentationPattern,productPpmTolr,true);
    }
    showTable();
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

    int mNum = row; //header row
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
