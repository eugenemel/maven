#include "peakdetectiondialog.h"

PeakDetectionDialog::PeakDetectionDialog(QWidget *parent) : 
    QDialog(parent) {
    setupUi(this);

    settings = nullptr;
    mainwindow = nullptr;
    peakupdater = nullptr;

    connect(computeButton, SIGNAL(clicked(bool)), SLOT(findPeaks()));
    connect(cancelButton, SIGNAL(clicked(bool)), SLOT(cancel()));
    connect(loadModelButton, SIGNAL(clicked(bool)), SLOT(loadModel()));
    connect(setOutputDirButton, SIGNAL(clicked(bool)), SLOT(setOutputDir()));
    connect(loadLibraryButton,SIGNAL(clicked(bool)), SLOT(showLibraryDialog()));

    setModal(true);

    scoringSettingsDialog = new MSMSScoringSettingsDialog(this);
    scoringSettingsDialog->setWindowFlags(scoringSettingsDialog->windowFlags() | Qt::WindowStaysOnTopHint);
    connect(scoringSettingsDialog->btnOK, SIGNAL(clicked(bool)), scoringSettingsDialog, SLOT(hide()));
    connect(btnFragMatchingAdvanced, SIGNAL(clicked()), scoringSettingsDialog, SLOT(bringIntoView()));
    connect(scoringSettingsDialog->btnLoadClassAdduct, SIGNAL(clicked()), scoringSettingsDialog, SLOT(setLipidClassAdductFile()));

    configureDiffIsotopeSearch = new ConfigureDiffIsotopeSearch(this);
    configureDiffIsotopeSearch->setWindowFlags(configureDiffIsotopeSearch->windowFlags() | Qt::WindowStaysOnTopHint);
    connect(configureDiffIsotopeSearch->btnOK, SIGNAL(clicked(bool)), configureDiffIsotopeSearch, SLOT(hide()));
    connect(configureDiffIsotopeSearch->btnCancel, SIGNAL(clicked(bool)), configureDiffIsotopeSearch, SLOT(hide()));
    connect(configureDiffIsotopeSearch->btnReset, SIGNAL(clicked(bool)), SLOT(resetDiffIsotopeSampleList()));

    connect(this->btnConfigureDifferentialIsotopeSearch, SIGNAL(clicked()), SLOT(populateDiffIsotopeSampleList()));
    connect(this->btnConfigureDifferentialIsotopeSearch, SIGNAL(clicked()), configureDiffIsotopeSearch, SLOT(bringIntoView()));

    _featureDetectionType= LibrarySearch;
}

void PeakDetectionDialog::setUIValuesFromSettings(){
    if (!settings) {
        qDebug() << "PeakDetectionDialog::setUIValuesFromSettings(): No settings currently set, unable to set UI values";
    }
    if (settings->contains("clsfModelFilename")) {
        classificationModelFilename->setText(settings->value("clsfModelFilename").toString());
    }
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

            setWindowTitle("QQQ Peak Detection");
            grpMassSlicingMethod->show();
            lblMassSlicingMethod->setText("Mass slices are determined based on loaded data files (samples).");

            featureOptions->hide();
            dbOptions->hide();
            grpBaseline->hide();
            grpMatchingOptions->hide();

    } else if (_featureDetectionType == PeaksSearch ) {

            setWindowTitle("Peak Detection");
            grpMassSlicingMethod->show();
            lblMassSlicingMethod->setText("Mass slices are determined based on loaded data files (samples).");

            featureOptions->show();
            displayCompoundOptions(false);

            grpBaseline->show();
            grpMatchingOptions->show();

    } else if (_featureDetectionType == LibrarySearch ) {

            setWindowTitle("Library Search");
            grpMassSlicingMethod->show();
            lblMassSlicingMethod->setText("Mass slices are derived from compound theoretical m/z.");

            featureOptions->hide();
            displayCompoundOptions(true);

            grpBaseline->show();
            grpMatchingOptions->show();

    }

	adjustSize();
}

void PeakDetectionDialog::displayCompoundOptions(bool isDisplayCompoundOptions) {
    if (isDisplayCompoundOptions) {
        lblCpdMatchingPolicy->show();
        peakGroupCompoundMatchPolicyBox->show();
        lblCpdMzMatchTolerance->show();
        compoundPPMWindow->show();
        lblCpdRtMatchTol->show();
        compoundRTWindow->show();
        compoundMustHaveMS2->show();
        compoundMatchRt->show();
    } else {
        lblCpdMatchingPolicy->hide();
        peakGroupCompoundMatchPolicyBox->hide();
        lblCpdMzMatchTolerance->hide();
        compoundPPMWindow->hide();
        lblCpdRtMatchTol->hide();
        compoundRTWindow->hide();
        compoundMustHaveMS2->hide();
        compoundMatchRt->hide();
    }
}

//Issue 430: loading model does not affect mainwindow classifier model
//model is not actually loaded until later
void PeakDetectionDialog::loadModel() { 

    const QString name = QFileDialog::getOpenFileName(this,
				"Select Classification Model", ".",tr("Model File (*.model)"));

    //Issue 430: prefer model, if available
    if (QFile::exists(classificationModelFilename->text())) {
        classificationModelFilename->setText(name);
    }
}

void PeakDetectionDialog::setOutputDir() { 
    const QString dirName = QFileDialog::getExistingDirectory(this, 
					"Save Reports to a Folder", ".", QFileDialog::ShowDirsOnly);
	outputDirName->setText(dirName);
}

void PeakDetectionDialog::show() {


    QSettings* settings = mainwindow->getSettings();
    if ( settings ) {
            eic_smoothingWindow->setValue(settings->value("eic_smoothingWindow").toDouble());
            grouping_maxRtDiff->setValue(settings->value("grouping_maxRtWindow").toDouble());
   }

    updateLibraryList();

    compoundPPMWindow->setValue( mainwindow->getUserPPM() );  //total ppm window, not half sized.

    QDialog::show();
}

void PeakDetectionDialog::updateLibraryList(){

    QString text = compoundDatabase->currentText();

    QStringList dbnames = DB.getLoadedDatabaseNames();

    dbnames.push_front("ALL");
    compoundDatabase->clear();
    for(QString db: dbnames ) {
        compoundDatabase->addItem(db);
    }

    //Issue 166: if a previous text entry was used, take this value.
    if (!text.isEmpty()) {
         int index = compoundDatabase->findText(text);
         if (index != -1) {
             compoundDatabase->setCurrentIndex(index);
         }
    } else {

        QString selectedDB = mainwindow->ligandWidget->getDatabaseName();

        //Issue 444
        if (selectedDB == SELECT_DB){
            compoundDatabase->setCurrentIndex(compoundDatabase->findText("ALL"));
        } else {
            compoundDatabase->setCurrentIndex(compoundDatabase->findText(selectedDB));
        }
    }

}

void PeakDetectionDialog::findPeaks() {
        if (!mainwindow) return;

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

        QString currentText = cmbSmootherType->currentText();

        if (cmbSmootherType->currentText() == "Savitzky-Golay") {
            peakupdater->eic_smoothingAlgorithm = EIC::SmootherType::SAVGOL;
        } else if (cmbSmootherType->currentText() == "Gaussian") {
            peakupdater->eic_smoothingAlgorithm = EIC::SmootherType::GAUSSIAN;
        } else if (cmbSmootherType->currentText() == "Moving Average") {
            peakupdater->eic_smoothingAlgorithm = EIC::SmootherType::AVG;
        } else { //fall back to Gaussian
            peakupdater->eic_smoothingAlgorithm = EIC::SmootherType::GAUSSIAN;
        }

        //Issue 430: prefer model, if available
        if (QFile::exists(classificationModelFilename->text())) {

            peakupdater->clsf = new ClassifierNeuralNet();
            peakupdater->clsf->loadModel(classificationModelFilename->text().toStdString());
            if (!peakupdater->clsf->hasModel()) {
                qDebug() << "PeakDetectionDialog::findPeaks(): unable to load peak quality model file, using mainwindow->getClassifier()";
                delete(peakupdater->clsf);
                peakupdater->clsf = mainwindow->getClassifier();
            }
        } else {
            peakupdater->clsf = mainwindow->getClassifier();
        }

        peakupdater->baseline_smoothingWindow = baseline_smoothing->value();
        peakupdater->baseline_dropTopX        = baseline_quantile->value();
		peakupdater->eic_smoothingWindow= eic_smoothingWindow->value();
		peakupdater->grouping_maxRtWindow = grouping_maxRtDiff->value();
        peakupdater->mergeOverlap = static_cast<float>(spnMergeOverlap->value());
        peakupdater->matchRtFlag =  compoundMatchRt->isChecked();
        peakupdater->featureMatchRtFlag = this->featureMatchRts->isChecked();
		peakupdater->minGoodPeakCount = minGoodGroupCount->value();
        peakupdater->minQuality = static_cast<float>(spnMinPeakQuality->value());
		peakupdater->minNoNoiseObs = minNoNoiseObs->value();
		peakupdater->minSignalBaseLineRatio = sigBaselineRatio->value();
		peakupdater->minSignalBlankRatio = sigBlankRatio->value();
		peakupdater->minGroupIntensity = minGroupIntensity->value();
        peakupdater->pullIsotopesFlag = reportIsotopes->isChecked();

        peakupdater->isotopeMzTolr = static_cast<float>(ppmStep->value());

        peakupdater->ppmMerge =  static_cast<float>(this->spnMassSliceMzMergeTolr->value());
        peakupdater->rtStepSize = static_cast<float>(rtStep->value());
        peakupdater->featureCompoundMatchMzTolerance = static_cast<float>(this->spnFeatureToCompoundMatchTolr->value());
        peakupdater->featureCompoundMatchRtTolerance = static_cast<float>(this->spnFeatureToCompoundRtTolr->value());

        peakupdater->compoundPPMWindow = compoundPPMWindow->value();
		peakupdater->compoundRTWindow = compoundRTWindow->value();

        float averageFullScanTime = 0;
        for (auto sample : samples){
            averageFullScanTime += sample->getAverageFullScanTime();
        }
        averageFullScanTime /= samples.size();

        peakupdater->avgScanTime = averageFullScanTime;


        peakupdater->mustHaveMS2 = featureMustHaveMs2->isChecked();
        peakupdater->compoundMustHaveMs2 = compoundMustHaveMS2->isChecked();
        peakupdater->productPpmTolr = scoringSettingsDialog->spnMatchTol->value();
        peakupdater->scoringScheme  = fragScoringAlgorithm->currentText();
        peakupdater->minFragmentMatchScore = scoringSettingsDialog->spnMinScore->value();
        peakupdater->minNumFragments = scoringSettingsDialog->spnMinMatches->value();
        peakupdater->minNumDiagnosticFragments = scoringSettingsDialog->spnMinDiagnostic->value();
        peakupdater->excludeIsotopicPeaks = excludeIsotopicPeaks->isChecked();

        //Issue 720
        peakupdater->isDiffAbundanceIsotopeSearch = chkDifferentialIsotopeSearch->isChecked();
        peakupdater->labeledSamples = configureDiffIsotopeSearch->getLabeledSamples(mainwindow->getVisibleSamples());
        peakupdater->unlabeledSamples = configureDiffIsotopeSearch->getUnlabeledSamples(mainwindow->getVisibleSamples());

        if (peakupdater->isDiffAbundanceIsotopeSearch && (peakupdater->labeledSamples.empty() || peakupdater->unlabeledSamples.empty())) {
            QMessageBox::information(
                this,
                "Sample Groups Not Designated",
                tr("The 'Differential Abundance Isotope Search' checkbox is currently selected, "
                   "but the search has not been configured.<br><br>"

                   "If you would like to conduct a Differential Abundance Isotope Search, "
                   "click the 'Configure Differential Abundance Isotope Search' button and "
                   "designate at least 1 loaded sample as 'Unlabeled' and 1 loaded sample as 'Isotopically Labeled'.<br><br>"

                   "If you would not like to conduct a Differential Abundance Isotope Search, "
                   "please un-check the 'Differential Abundance Isotope Search' checkbox.<br><br>"),
                QMessageBox::Ok);
            return;
        }

        string diffIsoSampleSetAgglomerationTypeStr = this->configureDiffIsotopeSearch->cmbSampleSetAgglomeration->currentText().toStdString();
        if (diffIsoSampleSetAgglomerationTypeStr == "Median") {
            peakupdater->diffIsoAgglomerationType = Fragment::ConsensusIntensityAgglomerationType::Median;
        } else if (diffIsoSampleSetAgglomerationTypeStr == "Mean") {
            peakupdater->diffIsoAgglomerationType = Fragment::ConsensusIntensityAgglomerationType::Mean;
        } else if (diffIsoSampleSetAgglomerationTypeStr == "Max") {
            peakupdater->diffIsoAgglomerationType = Fragment::ConsensusIntensityAgglomerationType::Max;
        } else if (diffIsoSampleSetAgglomerationTypeStr == "Sum") {
            peakupdater->diffIsoAgglomerationType = Fragment::ConsensusIntensityAgglomerationType::Sum;
        }

        peakupdater->diffIsoReproducibilityThreshold = this->configureDiffIsotopeSearch->spnMinNumSamples->value();
        peakupdater->diffIsoIncludeSingleZero = this->configureDiffIsotopeSearch->chkIncludeOneMissingIsotope->isChecked();
        peakupdater->diffIsoIncludeDoubleZero = this->configureDiffIsotopeSearch->chkIncludeTwoMissingIsotopes->isChecked();

        string policyText = peakGroupCompoundMatchPolicyBox->itemText(peakGroupCompoundMatchPolicyBox->currentIndex()).toStdString();

        if (policyText == "All matches"){
            peakupdater->peakGroupCompoundMatchingPolicy = ALL_MATCHES;
        } else if (policyText == "All matches with highest MS2 score") {
            peakupdater->peakGroupCompoundMatchingPolicy = TOP_SCORE_HITS;
        } else if (policyText == "One match with highest MS2 score (earliest alphabetically)") {
            peakupdater->peakGroupCompoundMatchingPolicy = SINGLE_TOP_HIT;
        }

        if ( ! outputDirName->text().isEmpty()) {
            peakupdater->setOutputDir(outputDirName->text());
		    peakupdater->writeCSVFlag=true;
        } else {
		    peakupdater->writeCSVFlag=false;
		}

        peakupdater->isRequireMatchingAdduct = chkRequireAdductMatch->isChecked();
        peakupdater->isRetainUnmatchedCompounds = chkRetainUnmatched->isChecked();
        peakupdater->isClusterPeakGroups = chkClusterPeakgroups->isChecked();

        //Issue 347
        if (chkSRMSearch->isChecked()) {
            _featureDetectionType = QQQ;
        }

		QString title;
        if (_featureDetectionType == PeaksSearch )  title = "Detected Features";
        else if (_featureDetectionType == LibrarySearch ) title = "Library Search";
                else if (_featureDetectionType == QQQ ) title = "QQQ Compound DB Search";

        //Issue 606
        if (peakupdater->scoringScheme == MzKitchenProcessor::LIPID_SCORING_NAME ||
                peakupdater->scoringScheme == MzKitchenProcessor::METABOLITES_SCORING_NAME) {
            title = "clamDB";
        }

        title = mainwindow->getUniquePeakTableTitle(title);

        //Issue 197
        shared_ptr<PeaksSearchParameters> peaksSearchParameters = getPeaksSearchParameters();
        peakupdater->peaksSearchParameters = peaksSearchParameters;
        string encodedParams = peaksSearchParameters->encodeParams();

        //Issue 606: mzkitchen-specific parameter sets
        peakupdater->lipidSearchParameters = getLipidSearchParameters();
        peakupdater->mzkitchenMetaboliteSearchParameters = getMzkitchenMetaboliteSearchParameters();
        peakupdater->peakPickingAndGroupingParameters = getPeakPickingAndGroupingParameters();

        if (peakupdater->scoringScheme == MzKitchenProcessor::LIPID_SCORING_NAME) {
            encodedParams = peakupdater->lipidSearchParameters->encodeParams();
        } else if (peakupdater->scoringScheme == MzKitchenProcessor::METABOLITES_SCORING_NAME) {
            encodedParams = peakupdater->mzkitchenMetaboliteSearchParameters->encodeParams();
        }

        string displayParams = encodedParams;
        replace(displayParams.begin(), displayParams.end(), ';', '\n');
        replace(displayParams.begin(), displayParams.end(), '=', ' ');

        //Issue 746: Alphabetize parameters
        QStringList displayParamsList = QString(displayParams.c_str()).split("\n");
        displayParamsList.sort(Qt::CaseInsensitive);
        QString singleString = displayParamsList.join("\n");
        if (singleString.startsWith("\n")) {
            singleString.remove(0, 1);
        }
        displayParams = singleString.toStdString();

        //Issue 606: Avoid running background job or making peaks table when score threshold/type make valid results impossible
        bool isHighMs2ScoreThreshold = peakupdater->mustHaveMS2 && peakupdater->minFragmentMatchScore > 1.0f;

        string selectedScoringType = peakupdater->scoringScheme.toStdString();
        bool isLowScoreType = selectedScoringType == "NormDotProduct" ||
                selectedScoringType == MzKitchenProcessor::METABOLITES_SCORING_NAME ||
                selectedScoringType == "FractionRefMatched";

        if (isHighMs2ScoreThreshold && isLowScoreType) {
            auto isContinue = QMessageBox::question(
                        this,
                        tr("No Valid Matches Possible"),
                        tr(
                            "No compound matches are possible with the combination of MS/MS score type and threshold you have selected.\n\n"
                            "To change the MS/MS score threshold, please click "
                            "'Configure MS/MS Matching Scoring Settings' from the Peak Scoring tab.\n\n"
                            "Would you like to continue?"),
                        QMessageBox::Yes | QMessageBox::No);

            if (isContinue != QMessageBox::Yes) {
                return;
            }
        }

        TableDockWidget* peaksTable = mainwindow->addPeaksTable(title, QString(encodedParams.c_str()), QString(displayParams.c_str()));
		peaksTable->setWindowTitle(title);

        connect(peakupdater, SIGNAL(newPeakGroup(PeakGroup*,bool, bool)), peaksTable, SLOT(addPeakGroup(PeakGroup*,bool, bool)));

        connect(peakupdater, SIGNAL(finished()), peaksTable, SLOT(showAllGroupsThenSort()));
        connect(peakupdater, SIGNAL(finished()), scoringSettingsDialog, SLOT(close()));
        connect(peakupdater, SIGNAL(finished()), this, SLOT(close()));

        connect(peakupdater, SIGNAL(terminated()), peaksTable, SLOT(showAllGroupsThenSort()));
        connect(peakupdater, SIGNAL(terminated()), scoringSettingsDialog, SLOT(close()));
   		connect(peakupdater, SIGNAL(terminated()), this, SLOT(close()));

        //Issue 722: Automatically collapse results from isotopes search.
        if (peakupdater->isDiffAbundanceIsotopeSearch || peakupdater->pullIsotopesFlag) {
            connect(peakupdater, SIGNAL(finished()), peaksTable->treeWidget, SLOT(collapseAll()));
            connect(peakupdater, SIGNAL(terminated()), peaksTable->treeWidget, SLOT(collapseAll()));
        }

        //Set compounds (possibility of subset of all loaded compound libraries)
        if(peakupdater->compoundDatabase == "ALL") {

            vector<Compound*> allCompounds = DB.compoundsDB;
            allCompounds.erase(std::remove_if(allCompounds.begin(), allCompounds.end(), [](Compound *compound){return (compound->db == "summarized" || compound->db == "rumsdb");}), allCompounds.end());

            qDebug() << "PeakDetectionDialog::findPeaks(): Removed" << (DB.compoundsDB.size() - allCompounds.size()) << "rumsdb and summarized compounds prior to peaks search.";

            peakupdater->setCompounds(allCompounds);

        } else {
            peakupdater->setCompounds( DB.getCompoundsSubset(compoundDatabase->currentText().toStdString()) );
        }

		//RUN THREAD
		if ( _featureDetectionType == QQQ ) {
			runBackgroupJob("findPeaksQQQ");
        } else if ( _featureDetectionType == PeaksSearch ) {
            runBackgroupJob("processMassSlices");
        }  else {
            //Library Search
			runBackgroupJob("computePeaks");
		}
}

void PeakDetectionDialog::runBackgroupJob(QString funcName) { 
    if (!peakupdater) return;

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

void PeakDetectionDialog::showLibraryDialog() {
    if(mainwindow) {
                mainwindow->libraryDialog->show();
                updateLibraryList();
    }
}

shared_ptr<PeaksSearchParameters> PeakDetectionDialog::getPeaksSearchParameters(){

        shared_ptr<PeaksSearchParameters> peaksSearchParameters = shared_ptr<PeaksSearchParameters>(new PeaksSearchParameters());

        //Issue 335
        if(strcmp(MAVEN_VERSION, "") == 0) {
            peaksSearchParameters->searchVersion = MAVEN_VERSION;
        }

        //Feature Detection (Peaks Search) and Compound Database (Compound DB Search)
        if (this->windowTitle() == "Library Search") {
            peaksSearchParameters->ms1PpmTolr = static_cast<float>(this->compoundPPMWindow->value());
            peaksSearchParameters->ms1RtTolr = static_cast<float>(this->compoundRTWindow->value());

            peaksSearchParameters->ms2IsMatchMs2 = this->compoundMustHaveMS2->isChecked();
            peaksSearchParameters->ms1IsMatchRtFlag = this->compoundMatchRt->isChecked();

            if (compoundDatabase->currentText() != "ALL") {
                peaksSearchParameters->matchingLibraries = compoundDatabase->currentText().toStdString();
            } else {
                peaksSearchParameters->matchingLibraries = DB.getLoadedDatabaseNames().join(", ").toStdString();
            }

            QString policyText = peakGroupCompoundMatchPolicyBox->currentText();
            if (policyText == "All matches"){
                peaksSearchParameters->matchingPolicy = ALL_MATCHES;
            } else if (policyText == "All matches with highest MS2 score") {
                peaksSearchParameters->matchingPolicy = TOP_SCORE_HITS;
            } else if (policyText == "One match with highest MS2 score (earliest alphabetically)") {
                peaksSearchParameters->matchingPolicy = SINGLE_TOP_HIT;
            }


        } else {

            peaksSearchParameters->ms1PpmTolr = static_cast<float>(this->spnFeatureToCompoundMatchTolr->value());
            peaksSearchParameters->ms1RtTolr = static_cast<float>(this->spnFeatureToCompoundRtTolr->value());

            peaksSearchParameters->ms2IsMatchMs2 = this->featureMustHaveMs2->isChecked();
            peaksSearchParameters->ms1IsMatchRtFlag = this->featureMatchRts->isChecked();

            peaksSearchParameters->matchingLibraries = DB.getLoadedDatabaseNames().join(", ").toStdString();
            peaksSearchParameters->matchingPolicy = PeakGroupCompoundMatchingPolicy::SINGLE_TOP_HIT;
        }

        //baseline
        peaksSearchParameters->baselineDropTopX = this->baseline_quantile->value();
        peaksSearchParameters->baselineSmoothingWindow = static_cast<float>(this->baseline_smoothing->value());

        //eic
        peaksSearchParameters->eicSmoothingWindow = this->eic_smoothingWindow->value();
        QString currentSmoother = this->cmbSmootherType->currentText();
        if (currentSmoother == "Moving Average") {
            peaksSearchParameters->eicEicSmoothingAlgorithm = "AVG";
        } else if (currentSmoother == "Gaussian") {
            peaksSearchParameters->eicEicSmoothingAlgorithm = "GAUSSIAN";
        } else if (currentSmoother == "Savitzky-Golay") {
            peaksSearchParameters->eicEicSmoothingAlgorithm = "SAVGOL";
        }
        peaksSearchParameters->eicMaxPeakGroupRtDiff = static_cast<float>(this->grouping_maxRtDiff->value());

        //grouping
        peaksSearchParameters->grpVersion = "EIC::groupPeaksE()";
        peaksSearchParameters->grpMergeOverlap = static_cast<float>(this->spnMergeOverlap->value());

        //quality
        peaksSearchParameters->qualitySignalBaselineRatio = static_cast<float>(this->sigBaselineRatio->value());
        peaksSearchParameters->qualitySignalBlankRatio = static_cast<float>(this->sigBlankRatio->value());
        peaksSearchParameters->qualityMinPeakGroupIntensity = static_cast<float>(this->minGroupIntensity->value());
        peaksSearchParameters->qualityMinPeakWidth = this->minNoNoiseObs->value();
        peaksSearchParameters->qualityMinGoodPeakPerGroup = this->minGoodGroupCount->value();
        peaksSearchParameters->qualityMinPeakQuality = static_cast<float>(this->spnMinPeakQuality->value());
        peaksSearchParameters->qualityClassifierModelName = this->classificationModelFilename->text().toStdString();

        //isotopes
        peaksSearchParameters->isotopesMzTolerance = static_cast<float>(this->ppmStep->value());
        
        //Issue 746, 748: this control isn't working - just disable for now
        peaksSearchParameters->isotopesIsRequireMonoisotopicPeaks = false;
        //peaksSearchParameters->isotopesIsRequireMonoisotopicPeaks = this->excludeIsotopicPeaks->isChecked();
        peaksSearchParameters->isotopesExtractIsotopicPeaks = this->reportIsotopes->isChecked();

        //ms1
        //peaksSearchParameters->ms1PpmTolr = this->spnFeatureToCompoundMatchTolr->value() / this->compoundPPMWindow->value()
        //peaksSearchParameters->ms1RtTolr = this->compoundRTWindow->value() / this->spnFeatureToCompoundRtTolr->value()
        //peaksSearchParameters->ms1IsMatchRtFlag = this->matchRt->isChecked() / this->featureMatchRts->isChecked()
        peaksSearchParameters->ms1MassSliceMergePpm = static_cast<float>(this->spnMassSliceMzMergeTolr->value());
        peaksSearchParameters->ms1MassSliceMergeNumScans = static_cast<float>(this->rtStep->value());

        //ms2 matching
        //peaksSearchParameters->ms2IsMatchMs2 = this->compoundMustHaveMS2->isChecked() / this->featureMustHaveMs2->isChecked()
        peaksSearchParameters->ms2MinNumMatches = scoringSettingsDialog->spnMinMatches->value();
        peaksSearchParameters->ms2MinNumDiagnosticMatches = scoringSettingsDialog->spnMinDiagnostic->value();
        peaksSearchParameters->ms2PpmTolr = static_cast<float>(scoringSettingsDialog->spnMatchTol->value());
        peaksSearchParameters->ms2ScoringAlgorithm = this->fragScoringAlgorithm->currentText().toStdString();
        peaksSearchParameters->ms2MinScore = static_cast<float>(scoringSettingsDialog->spnMinScore->value());

        //Matching Options
        peaksSearchParameters->matchingIsRequireAdductPrecursorMatch = this->chkRequireAdductMatch->isChecked();
        peaksSearchParameters->matchingIsRetainUnknowns = this->chkRetainUnmatched->isChecked();
        //peaksSearchParameters->matchingLibraries = DB.getLoadedDatabaseNames().join(", ").toStdString() /  peakGroupCompoundMatchPolicyBox->currentText().toStdString()
        //peaksSearchParameters->matchingPolicy = PeakGroupCompoundMatchingPolicy::SINGLE_TOP_HIT / peakGroupCompoundMatchPolicyBox->currentText()

        //peak group clustering is not retained, as clusters can be easily added or removed with different parameters in the GUI

        return peaksSearchParameters;
}

shared_ptr<LCLipidSearchParameters> PeakDetectionDialog::getLipidSearchParameters() {
    shared_ptr<LCLipidSearchParameters> lipidSearchParameters = shared_ptr<LCLipidSearchParameters>(new LCLipidSearchParameters());
    lipidSearchParameters->peakPickingAndGroupingParameters = getPeakPickingAndGroupingParameters();

    shared_ptr<PeaksSearchParameters> peaksSearchParameters = getPeaksSearchParameters();

    string encodedPeaksParameters = peaksSearchParameters->encodeParams();

    unordered_map<string, string> decodedMap = mzUtils::decodeParameterMap(encodedPeaksParameters);
    lipidSearchParameters->fillInBaseParams(decodedMap);

    lipidSearchParameters->ms2MinNumAcylMatches = scoringSettingsDialog->spnMinAcyl->value();
    lipidSearchParameters->ms2sn1MinNumMatches = scoringSettingsDialog->spnMinSn1->value();
    lipidSearchParameters->ms2sn2MinNumMatches = scoringSettingsDialog->spnMinSn2->value();
    lipidSearchParameters->ms2IsRequirePrecursorMatch = scoringSettingsDialog->chkRequirePrecursorInMS2->isChecked();

    QString classAdductFile = scoringSettingsDialog->classAdductFileName->text();
    if (QFile::exists(classAdductFile)) {
        lipidSearchParameters->addClassAdductParamsFromCSVFile(classAdductFile.toStdString(), false);
    } else {
        qDebug() << "Class Adduct File '" << classAdductFile << "' Could not be read or does not exist.";
    }

    // Issue 746: Note similarity to PeakParameters->matchingIsRequireAdductPrecursorMatch
    lipidSearchParameters->IDisRequireMatchingAdduct = this->chkRequireAdductMatch->isChecked();

    return lipidSearchParameters;
}

shared_ptr<MzkitchenMetaboliteSearchParameters> PeakDetectionDialog::getMzkitchenMetaboliteSearchParameters() {

    shared_ptr<MzkitchenMetaboliteSearchParameters> mzkitchenMetaboliteSearchParameters = shared_ptr<MzkitchenMetaboliteSearchParameters>(new MzkitchenMetaboliteSearchParameters());
    mzkitchenMetaboliteSearchParameters->peakPickingAndGroupingParameters = getPeakPickingAndGroupingParameters();

    shared_ptr<PeaksSearchParameters> peaksSearchParameters = getPeaksSearchParameters();

    string encodedPeaksParameters = peaksSearchParameters->encodeParams();

    unordered_map<string, string> decodedMap = mzUtils::decodeParameterMap(encodedPeaksParameters);
    mzkitchenMetaboliteSearchParameters->fillInBaseParams(decodedMap);

    mzkitchenMetaboliteSearchParameters->rtIsRequireRtMatch = this->featureMatchRts->isChecked();
    mzkitchenMetaboliteSearchParameters->rtMatchTolerance = static_cast<float>(this->spnFeatureToCompoundRtTolr->value());

    // Issue 746: Note similarity to PeakParameters->matchingIsRequireAdductPrecursorMatch
    mzkitchenMetaboliteSearchParameters->IDisRequireMatchingAdduct = this->chkRequireAdductMatch->isChecked();

    return mzkitchenMetaboliteSearchParameters;
}

//Issue 620
shared_ptr<PeakPickingAndGroupingParameters> PeakDetectionDialog::getPeakPickingAndGroupingParameters(){

    shared_ptr<PeakPickingAndGroupingParameters> peakPickingAndGroupingParameters = shared_ptr<PeakPickingAndGroupingParameters>(new PeakPickingAndGroupingParameters());

    float peakRtBoundsSlopeThreshold = static_cast<float>(spnPeakBoundarySlopeThreshold->value());
    if (peakRtBoundsSlopeThreshold < 0) {
        peakRtBoundsSlopeThreshold = -1.0f;
    }

    float peakRtBoundsMaxIntensityFraction = static_cast<float>(spnPeakBoundaryIntensityThreshold->value());
    if (peakRtBoundsMaxIntensityFraction < 0) {
        peakRtBoundsMaxIntensityFraction = -1.0f;
    }

    float mergedSmoothedMaxToBoundsMinRatio = static_cast<float>(spnPeakGroupSNThreshold->value());
    if (mergedSmoothedMaxToBoundsMinRatio < 0) {
        mergedSmoothedMaxToBoundsMinRatio = -1.0f;
    }

    // START EIC::getPeakPositionsD()
    //peak picking
    peakPickingAndGroupingParameters->peakSmoothingWindow = this->eic_smoothingWindow->value();
    peakPickingAndGroupingParameters->peakRtBoundsMaxIntensityFraction = peakRtBoundsMaxIntensityFraction;
    peakPickingAndGroupingParameters->peakRtBoundsSlopeThreshold = peakRtBoundsSlopeThreshold;
    peakPickingAndGroupingParameters->peakBaselineSmoothingWindow = this->baseline_smoothing->value();
    peakPickingAndGroupingParameters->peakBaselineDropTopX = this->baseline_quantile->value();
    peakPickingAndGroupingParameters->peakIsComputeBounds = true;
    peakPickingAndGroupingParameters->peakIsReassignPosToUnsmoothedMax = false;

     //eic
     int eicBaselineEstimationType = this->cmbEicBaselineType->currentIndex();
     peakPickingAndGroupingParameters->eicBaselineEstimationType = static_cast<EICBaselineEstimationType>(eicBaselineEstimationType);

     // END EIC::getPeakPositionsD()

     // START EIC::groupPeaksE()

     //merged EIC
     peakPickingAndGroupingParameters->mergedSmoothingWindow = this->eic_smoothingWindow->value();
//   peakPickingAndGroupingParameters->mergedPeakRtBoundsMaxIntensityFraction = mergedPeakRtBoundsMaxIntensityFraction;
     peakPickingAndGroupingParameters->mergedPeakRtBoundsSlopeThreshold = peakRtBoundsSlopeThreshold;
     peakPickingAndGroupingParameters->mergedSmoothedMaxToBoundsMinRatio = mergedSmoothedMaxToBoundsMinRatio;
     peakPickingAndGroupingParameters->mergedSmoothedMaxToBoundsIntensityPolicy = SmoothedMaxToBoundsIntensityPolicy::MINIMUM; // loosest - use smaller intensity in denominator
     peakPickingAndGroupingParameters->mergedBaselineSmoothingWindow = this->baseline_smoothing->value();
     peakPickingAndGroupingParameters->mergedBaselineDropTopX = this->baseline_quantile->value();
     peakPickingAndGroupingParameters->mergedIsComputeBounds = true;

     //grouping
     peakPickingAndGroupingParameters->groupMaxRtDiff = static_cast<float>(this->grouping_maxRtDiff->value());
     peakPickingAndGroupingParameters->groupMergeOverlap = static_cast<float>(this->spnMergeOverlap->value());

     //post-grouping filters
     peakPickingAndGroupingParameters->filterMinGoodGroupCount = minGoodGroupCount->value();
     peakPickingAndGroupingParameters->filterMinQuality = static_cast<float>(spnMinPeakQuality->value());
     peakPickingAndGroupingParameters->filterMinNoNoiseObs = this->minNoNoiseObs->value();
     peakPickingAndGroupingParameters->filterMinSignalBaselineRatio = static_cast<float>(this->sigBaselineRatio->value());
     peakPickingAndGroupingParameters->filterMinGroupIntensity = static_cast<float>(this->minGroupIntensity->value());
     peakPickingAndGroupingParameters->filterMinSignalBlankRatio = static_cast<float>(this->sigBlankRatio->value());
     //peakPickingAndGroupingParameters->filterMinPrecursorCharge = minPrecursorCharge;

     // END EIC::groupPeaksE()

    return peakPickingAndGroupingParameters;
}

void PeakDetectionDialog::setMainWindow(MainWindow *w){
    this->mainwindow = w;
    connect(mainwindow->libraryDialog, SIGNAL(loadLibrarySignal(QString)), SLOT(updateLibraryList()));
}

void PeakDetectionDialog::resetDiffIsotopeSampleList() {
    if (this->mainwindow) {
        this->configureDiffIsotopeSearch->resetSamples(mainwindow->getVisibleSamples());
    }
}

void PeakDetectionDialog::populateDiffIsotopeSampleList(){
    if (this->mainwindow) {
        this->configureDiffIsotopeSearch->populateSamples(mainwindow->getVisibleSamples());
    }
}
