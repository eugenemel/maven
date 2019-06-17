#include "peakdetectiondialog.h"

PeakDetectionDialog::PeakDetectionDialog(QWidget *parent) : 
    QDialog(parent) {
    setupUi(this);
    settings = NULL;
    mainwindow = NULL;

    peakupdater = NULL;
    connect(computeButton, SIGNAL(clicked(bool)), SLOT(findPeaks()));
    connect(cancelButton, SIGNAL(clicked(bool)), SLOT(cancel()));
    connect(loadModelButton, SIGNAL(clicked(bool)), SLOT(loadModel()));
    connect(setOutputDirButton, SIGNAL(clicked(bool)), SLOT(setOutputDir()));

    setModal(true);

    _featureDetectionType= CompoundDB;

}

PeakDetectionDialog::~PeakDetectionDialog() { 
	cancel();
	if (peakupdater) delete(peakupdater);
}

void PeakDetectionDialog::cancel() { 

	if (peakupdater) {
		if( peakupdater->isRunning() ) { peakupdater->stop();  return; }
	}
	close(); 
}

void PeakDetectionDialog::setFeatureDetection(FeatureDetectionType type) {
    _featureDetectionType = type;

    if (_featureDetectionType == QQQ ) {
            dbOptions->hide();  
            featureOptions->hide();
    } else if (_featureDetectionType == FullSpectrum ) {
            dbOptions->hide();  
            featureOptions->show();
    } else if (_featureDetectionType == CompoundDB ) {
            dbOptions->show();  
            featureOptions->hide();
    }
	adjustSize();
}
void PeakDetectionDialog::loadModel() { 

    const QString name = QFileDialog::getOpenFileName(this,
				"Select Classification Model", ".",tr("Model File (*.model)"));

	classificationModelFilename->setText(name); 
	Classifier* clsf = mainwindow->getClassifier();	//get classification model
	if (clsf ) clsf->loadModel( classificationModelFilename->text().toStdString() );
}

void PeakDetectionDialog::setOutputDir() { 
    const QString dirName = QFileDialog::getExistingDirectory(this, 
					"Save Reports to a Folder", ".", QFileDialog::ShowDirsOnly);
	outputDirName->setText(dirName);
}

void PeakDetectionDialog::show() {

    if ( mainwindow != NULL ) {
        QSettings* settings = mainwindow->getSettings();
        if ( settings ) {
            eic_smoothingWindow->setValue(settings->value("eic_smoothingWindow").toDouble());
            grouping_maxRtDiff->setValue(settings->value("grouping_maxRtWindow").toDouble());
        }
    }

    QStringList dbnames = DB.getLoadedDatabaseNames();
    dbnames.push_front("ALL");
    compoundDatabase->clear();
    for(QString db: dbnames ) {
        compoundDatabase->addItem(db);
    }

    fragScoringAlgorithm->clear();
    for(string scoringAlgorithm: FragmentationMatchScore::getScoringAlgorithmNames()) {
        fragScoringAlgorithm->addItem(scoringAlgorithm.c_str());
    }

    QString selectedDB = mainwindow->ligandWidget->getDatabaseName();
    compoundDatabase->setCurrentIndex(compoundDatabase->findText(selectedDB));
    compoundPPMWindow->setValue( mainwindow->getUserPPM() );  //total ppm window, not half sized.

    QDialog::show();
}

void PeakDetectionDialog::findPeaks() {
        if (mainwindow == nullptr) return;

		vector<mzSample*>samples = mainwindow->getSamples();
		if ( samples.size() == 0 ) return;

        if (peakupdater)  {
			if ( peakupdater->isRunning() ) cancel();
			if ( peakupdater->isRunning() ) return;
		}

        if (peakupdater) {
            delete(peakupdater);
            peakupdater=nullptr;
		}

		peakupdater = new BackgroundPeakUpdate(this);
		peakupdater->setMainWindow(mainwindow);

		connect(peakupdater, SIGNAL(updateProgressBar(QString,int,int)), SLOT(setProgressBar(QString, int,int)));
	
        if (settings) {
             peakupdater->eic_ppmWindow = settings->value("eic_ppmWindow").toDouble();
             peakupdater->eic_smoothingAlgorithm = settings->value("eic_smoothingAlgorithm").toInt();
		}

        peakupdater->baseline_smoothingWindow = baseline_smoothing->value();
        peakupdater->baseline_dropTopX        = baseline_quantile->value();
		peakupdater->eic_smoothingWindow= eic_smoothingWindow->value();
		peakupdater->grouping_maxRtWindow = grouping_maxRtDiff->value();
		peakupdater->matchRtFlag =  matchRt->isChecked();
		peakupdater->minGoodPeakCount = minGoodGroupCount->value();
		peakupdater->minNoNoiseObs = minNoNoiseObs->value();
		peakupdater->minSignalBaseLineRatio = sigBaselineRatio->value();
		peakupdater->minSignalBlankRatio = sigBlankRatio->value();
		peakupdater->minGroupIntensity = minGroupIntensity->value();
        peakupdater->pullIsotopesFlag = reportIsotopes->isChecked();
        peakupdater->ppmMerge =  ppmStep->value();
        peakupdater->compoundPPMWindow = compoundPPMWindow->value();
		peakupdater->compoundRTWindow = compoundRTWindow->value();
		peakupdater->avgScanTime = samples[0]->getAverageFullScanTime();
        peakupdater->rtStepSize = rtStep->value();
        peakupdater->mustHaveMS2 = compoundMustHaveMS2->isChecked() || featureMustHaveMs2->isChecked();
        peakupdater->productPpmTolr = productPpmTolr->value();
        peakupdater->scoringScheme  = fragScoringAlgorithm->currentText();
        peakupdater->minFragmentMatchScore = fragMinScore->value();
        peakupdater->minNumFragments = fragMinPeaks->value();
        peakupdater->searchAdductsFlag = reportAdducts->isChecked();
        peakupdater->excludeIsotopicPeaks = excludeIsotopicPeaks->isChecked();

        string policyText = peakGroupCompoundMatchPolicyBox->itemText(peakGroupCompoundMatchPolicyBox->currentIndex()).toStdString();

        cerr << "Policy Text:" << policyText << endl;

        if (policyText == "All matches"){
            peakupdater->peakGroupCompoundMatchingPolicy = ALL_MATCHES;
            cerr << "[peakdetectiondialog]: set to ALL_MATCHES" << endl;
        } else if (policyText == "All matches with highest MS2 score") {
            peakupdater->peakGroupCompoundMatchingPolicy = TOP_SCORE_HITS;
            cerr << "[peakdetectiondialog]: set to TOP_SCORE_HITS" << endl;
        } else if (policyText == "One match with highest MS2 score (earliest alphabetically)") {
            peakupdater->peakGroupCompoundMatchingPolicy = SINGLE_TOP_HIT;
            cerr << "[peakdetectiondialog]: set to SINGLE_TOP_HIT" << endl;
        }

        //peakupdater->peakGroupCompoundMatchingPolicy = //TODO

        if ( ! outputDirName->text().isEmpty()) {
            peakupdater->setOutputDir(outputDirName->text());
		    peakupdater->writeCSVFlag=true;
        } else {
		    peakupdater->writeCSVFlag=false;
		}

		QString title;
		if (_featureDetectionType == FullSpectrum )  title = "Detected Features";
                else if (_featureDetectionType == CompoundDB ) title = "DB Search " + compoundDatabase->currentText();
                else if (_featureDetectionType == QQQ ) title = "QQQ DB Search " + compoundDatabase->currentText();
     
		TableDockWidget* peaksTable = mainwindow->addPeaksTable(title);
		peaksTable->setWindowTitle(title);
        connect(peakupdater, SIGNAL(newPeakGroup(PeakGroup*,bool, bool)), peaksTable, SLOT(addPeakGroup(PeakGroup*,bool, bool)));
        connect(peakupdater, SIGNAL(finished()), peaksTable, SLOT(showAllGroups()));
   		connect(peakupdater, SIGNAL(terminated()), peaksTable, SLOT(showAllGroups()));
   		connect(peakupdater, SIGNAL(finished()), this, SLOT(close()));
   		connect(peakupdater, SIGNAL(terminated()), this, SLOT(close()));
		
		
		//RUN THREAD
		if ( _featureDetectionType == QQQ ) {
			runBackgroupJob("findPeaksQQQ");
		} else if ( _featureDetectionType == FullSpectrum ) {
			runBackgroupJob("processMassSlices");
        }  else {
            peakupdater->compoundDatabase = compoundDatabase->currentText();
            if(peakupdater->compoundDatabase == "ALL") {
                peakupdater->setCompounds( DB.compoundsDB );
            } else {
                peakupdater->setCompounds( DB.getCopoundsSubset(compoundDatabase->currentText().toStdString()) );
            }
			runBackgroupJob("computePeaks");
		}
}

void PeakDetectionDialog::runBackgroupJob(QString funcName) { 
    if (peakupdater == nullptr ) return;

	if ( peakupdater->isRunning() ) { 
			cancel(); 
	}

	if ( ! peakupdater->isRunning() ) { 
		peakupdater->setRunFunction(funcName);			//set thread function

        qDebug() << "peakdetectiondialog::runBackgroundJob(QString funcName) Started.";

        peakupdater->start();	//start a background thread

        qDebug() << "peakdetectiondialog::runBackgroundJob(QString funcName) Completed.";
    }
}

void PeakDetectionDialog::showInfo(QString text){ statusText->setText(text); }

void PeakDetectionDialog::setProgressBar(QString text, int progress, int totalSteps){
	showInfo(text);
	progressBar->setRange(0,totalSteps);
    progressBar->setValue(progress);
}

