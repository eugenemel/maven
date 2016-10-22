#include "calibratedialog.h"

CalibrateDialog::CalibrateDialog(QWidget *parent) : QDialog(parent) { 
	setupUi(this); 
    setModal(true);

    mw = (MainWindow*) parent;

    workerThread = new BackgroundPeakUpdate(this);
    workerThread->setMainWindow(mw);
    connect(workerThread, SIGNAL(updateProgressBar(QString,int,int)), mw, SLOT(setProgressBar(QString, int,int)));
    connect(workerThread, SIGNAL(finished()),SLOT(doCorrection()));
    connect(workerThread, SIGNAL(newPeakGroup(PeakGroup*,bool)), this, SLOT(addPeakGroup(PeakGroup*,bool)));
    connect(calibrateButton,SIGNAL(clicked(bool)),SLOT(runCalibration()));
    connect(undoCalibrationButton,SIGNAL(clicked(bool)),SLOT(undoCalibration()));
}

void CalibrateDialog::show() {

    /*if (mainwindow != NULL ) {
        QSettings* settings = mainwindow->getSettings();
        if ( settings ) {
            eic_smoothingWindow->setValue(settings->value("eic_smoothingWindow").toDouble());
            grouping_maxRtDiff->setValue(settings->value("grouping_maxRtWindow").toDouble());
        }
    }*/

    QStringList dbnames = DB.getLoadedDatabaseNames();
    compoundDatabase->clear();
    for(QString db: dbnames ) {
        compoundDatabase->addItem(db);
    }

    /*

    QString selectedDB = mainwindow->ligandWidget->getDatabaseName();
    compoundDatabase->setCurrentIndex(compoundDatabase->findText(selectedDB));
    compoundPPMWindow->setValue( mainwindow->getUserPPM() );  //total ppm window, not half sized.
    */

    QDialog::show();
}

void CalibrateDialog::undoCalibration() {
    for(mzSample* s: mw->getSamples()) {
        reverseFit(s) ;
    }
}

void CalibrateDialog::runCalibration() {

    allgroups.clear();

    QString dbName = compoundDatabase->currentText();
    workerThread->eic_ppmWindow = ppmSearch->value();
    workerThread->compoundPPMWindow = ppmSearch->value();
    workerThread->eic_smoothingAlgorithm = 10;
    workerThread->compoundDatabase = dbName;
    workerThread->setCompounds( DB.getCopoundsSubset(dbName.toStdString()));
    workerThread->matchRtFlag = true;
    //workerThread->minGoodPeakCount = 1;
    workerThread->minGroupIntensity =  minGroupIntensity->value();
    workerThread->minSignalBaseLineRatio = minSN->value();
    //workerThread->minNoNoiseObs =  10;
    workerThread->alignSamplesFlag=false;
    workerThread->keepFoundGroups=true;
    workerThread->eicMaxGroups=1;
    workerThread->mustHaveMS2=true;
    workerThread->productPpmTolr=2000;

    workerThread->setRunFunction("computePeaks");
    workerThread->start();
}


void CalibrateDialog::addPeakGroup(PeakGroup *grp, bool) {
    PeakGroup copygrp = *grp;

    qDebug() << "ADD GRROUP:" << copygrp.meanMz << " " << copygrp.meanRt;
    allgroups.push_back(copygrp);
    /*
    for(int i=0; i<allgroups.size(); i++) {
        PeakGroup& grpB = allgroups[i];
        float rtoverlap = mzUtils::checkOverlap(grp.minRt, grp.maxRt, grpB.minRt, grpB.maxRt );
        if ( rtoverlap > 0.9 && ppmDist(grpB.meanMz, grp.meanMz) < ppmMerge  ) {
            return false;
        }
    }*/

}

/*

void CalibrateDialog::doCorrection() {

   MassCalculator massCalc;
   Adduct* adduct = MassCalculator::MinusHAdduct;
   if (workerThread->ionizationMode > 0) adduct = MassCalculator::PlusHAdduct;

  for(mzSample* s: mw->getSamples() ) {
    cerr << s->sampleName << endl;
    vector<double>x;
    vector<double>y;

    for(PeakGroup& g: allgroups ) {
            Peak* p = g.getPeak(s);
            Compound* c = g.compound;

            if (p and c and !c->formula.empty())  {
                double M0 = adduct->computeAdductMass( massCalc.computeNeutralMass(c->formula) );
                x.push_back(p->medianMz);
                y.push_back(M0);

                cerr << setprecision(7) << "x=\t" << p->medianMz << "\ty=\t" << M0 << "\t" << p->medianMz-M0 << endl;
            }
    }

    leastSqrFit(x,y);
  }
}
*/


void CalibrateDialog::doCorrection() {

    MassCalculator massCalc;
    Adduct* adduct = MassCalculator::MinusHAdduct;
    if (workerThread->ionizationMode > 0) adduct = MassCalculator::PlusHAdduct;

    int minNumFragments=5;
    float productPpmTolr=100;
    double precursorPPM = ppmSearch->value();

    for(mzSample* s: mw->getSamples() ) {

        vector<double>x;
        vector<double>y;

        for(PeakGroup& g: allgroups ) {
            Peak* p  = g.getPeak(s);
            if (!p ) continue;
            //Fragment fcons = p->getConsensusFragmentation(precursorPPM,productPpmTolr);
            Fragment fcons =  g.fragmentationPattern;

            if (fcons.nobs() == 0) continue;

            //theoretical MS2
            Compound* cpd = g.compound;
            if(!cpd) continue;
            Fragment t;
            t.precursorMz = cpd->precursorMz;
            t.mzs = cpd->fragment_mzs;
            t.intensity_array = cpd->fragment_intensity;
            t.sortByMz();

            //compare MS2s..
            FragmentationMatchScore s = t.scoreMatch(&fcons,productPpmTolr);
            if (s.numMatches < minNumFragments ) continue;
            cerr << "\t: Matched:" << cpd->name << " #Peaks=" << s.numMatches << " obs=" << fcons.nobs() << " th=" << t.nobs() << endl;

            vector<int>matches = Fragment::compareRanks(&t,&fcons,productPpmTolr);

            for(int i=0; i<matches.size(); i++) {
                double obs = fcons.mzs[ matches[i]];
                if (obs < 400) continue;
                if ( matches[i] < 0 ) continue;
                x.push_back(obs);
                y.push_back(t.mzs[i]);
            }
        }

        vector<double> coef = leastSqrFit(x,y);
        cerr << "FIT: " << s->sampleName <<  " " << setprecision(7) << coef[0] << " " << coef[1] << " " << std::endl;

        if ( evaluateFit(x,y,coef) ) {
            applyFit(s,coef);
        }
    }
}

void CalibrateDialog::applyFit(mzSample* sample, vector<double>coef) {
    if(coef.size() == 0) return;
    sample->ppmCorrectionCoef = coef;
    qDebug() << "applyFit()";

    for(Scan* scan: sample->scans) {

        if (scan->mslevel > 1) {
            if (scan->precursorMz < 400) continue;
            scan->precursorMz =  coef[0] + coef[1]*scan->precursorMz;
        }

        for(int i=0; i<scan->nobs(); i++ ) {
            if (scan->mz[i] < 400) continue;
            scan->mz[i] =  coef[0] + coef[1]*scan->mz[i];
        }
    }
}


void CalibrateDialog::reverseFit(mzSample* sample) {
    vector<double> coef = sample->ppmCorrectionCoef;
    if(coef.size() == 0) return;
    qDebug() << "reverseFit()";

    for(Scan* scan: sample->scans) {

        if (scan->mslevel > 1) {
            if (scan->precursorMz < 400) continue;
            scan->precursorMz =  (scan->precursorMz-coef[0])/coef[1];
        }

        for(int i=0; i<scan->nobs(); i++ ) {
            if (scan->mz[i] < 400) continue;
             scan->mz[i] = (scan->mz[i] - coef[0] )/ coef[1];
        }
    }
    sample->ppmCorrectionCoef.clear();
}

bool CalibrateDialog::evaluateFit(vector<double>obs, vector<double>exp, vector<double> coef) {

    double sumSqrBefore=0;
    double sumSqrAfter =0;

    for(int i=0; i<obs.size(); i++) {
        double corrected = coef[0] + coef[1]*obs[i];
        double ppmBefore = (obs[i]-exp[i])/exp[i]*1e6;
        double ppmAfter =  (corrected-exp[i])/exp[i]*1e6;
        sumSqrBefore += ppmBefore*ppmBefore;
        sumSqrAfter  += ppmAfter*ppmAfter;
        cerr << "\t\t" << i << "\t" << obs[i] << " \t" << exp[i] << "\t" << corrected << "\t" << ppmBefore << " " << ppmAfter << std::endl;
    }

    cerr << "evaluateFit:" << sumSqrBefore << "=>" << sumSqrAfter << endl;
    return sumSqrAfter < sumSqrBefore;
}

vector<double> CalibrateDialog::leastSqrFit(vector<double>&x, vector<double>&y) {
    int poly_align_degree=1;
    double* result = new double[poly_align_degree+1];
    double* w = new double[(poly_align_degree+1)*(poly_align_degree+1)];
    int N = x.size();

    double* px = &x[0];
    double* py = &y[0];

    sort_xy(px, py, N, 1, 0);
    leasqu(N,px,py, poly_align_degree, w, poly_align_degree+1, result);

    vector<double>coef(poly_align_degree+1,0);
    for(int i=0; i < poly_align_degree+1; i++ ) coef[i]=result[i];

    delete[] result;
    delete[] w;
    return coef;
}
