#include "settingsform.h"

SettingsForm::SettingsForm(QSettings* s, MainWindow *w): QDialog(w) { 

    //create components
    setupUi(this);

    settings = s;
    mainwindow = w;

    //Fill out GUI based on current settings
    setFormValues();

    //Write values from GUI into current settings
    getFormValues();

    connect(tabWidget, SIGNAL(currentChanged(int)), SLOT(getFormValues()));

    //peak detection
    connect(recomputeEICButton, SIGNAL(clicked(bool)), SLOT(recomputeEIC()));
    connect(chkDisplayConsensusSpectrum, SIGNAL(toggled(bool)), SLOT(getFormValues()));
    connect(chkDisplayConsensusSpectrum, SIGNAL(toggled(bool)), mainwindow, SLOT(updateGUIWithLastSelectedPeakGroup()));

    connect(ionizationMode, SIGNAL(currentIndexChanged(int)), SLOT(getFormValues()));
//    connect(isotopeC13Correction, SIGNAL(toggled(bool)), SLOT(getFormValues()));
    connect(amuQ1, SIGNAL(valueChanged(double)), SLOT(getFormValues()));
    connect(amuQ3, SIGNAL(valueChanged(double)), SLOT(getFormValues()));

    //isotope detection setting
    connect(C13Labeled,SIGNAL(toggled(bool)),SLOT(recomputeIsotopes()));
    connect(N15Labeled,SIGNAL(toggled(bool)),SLOT(recomputeIsotopes()));
    connect(S34Labeled,SIGNAL(toggled(bool)),SLOT(recomputeIsotopes()));
    connect(D2Labeled, SIGNAL(toggled(bool)),SLOT(recomputeIsotopes()));
//    connect(isotopeC13Correction, SIGNAL(toggled(bool)), SLOT(recomputeIsotopes()));
    connect(chkIgnoreNaturalAbundance, SIGNAL(toggled(bool)), SLOT(recomputeIsotopes()));
    connect(chkExtractNIsotopes, SIGNAL(toggled(bool)), SLOT(recomputeIsotopes()));
    connect(spnMaxIsotopesToExtract, SIGNAL(valueChanged(int)), SLOT(recomputeIsotopes()));

    connect(maxNaturalAbundanceErr, SIGNAL(valueChanged(double)), SLOT(recomputeIsotopes()));
    connect(minIsotopicCorrelation, SIGNAL(valueChanged(double)), SLOT(recomputeIsotopes()));
    connect(maxIsotopeScanDiff, SIGNAL(valueChanged(int)), SLOT(recomputeIsotopes()));

    //remote url used to fetch compound lists, pathways, and notes
    connect(data_server_url, SIGNAL(textChanged(QString)), SLOT(getFormValues()));
    connect(scriptsFolderSelect, SIGNAL(clicked()), SLOT(selectScriptsFolder()));
    connect(methodsFolderSelect, SIGNAL(clicked()), SLOT(selectMethodsFolder()));
    connect(RProgramSelect, SIGNAL(clicked()), SLOT(selectRProgram()));

    connect(centroid_scan_flag,SIGNAL(toggled(bool)), SLOT(getFormValues()));
    connect(scan_filter_min_quantile, SIGNAL(valueChanged(int)), SLOT(getFormValues()));
    connect(scan_filter_min_intensity, SIGNAL(valueChanged(int)), SLOT(getFormValues()));
    connect(ionizationType,SIGNAL(currentIndexChanged(int)),SLOT(getFormValues()));

    //bookmark options
    connect(chkBkmkWarnMz, SIGNAL(toggled(bool)), SLOT(getFormValues()));
    connect(chkBkmkWarnMzRt, SIGNAL(toggled(bool)), SLOT(getFormValues()));
    connect(chkIncludeChildren, SIGNAL(toggled(bool)), SLOT(getFormValues()));

    //spectra widget display options
    //ms1 options
    connect(chkMs1Autoscale, SIGNAL(toggled(bool)), SLOT(getFormValues()));
    connect(spnMs1Offset, SIGNAL(valueChanged(double)), SLOT(getFormValues()));
    connect(spnMs1MzMinVal, SIGNAL(valueChanged(double)), SLOT(getFormValues()));
    connect(spnMs1MzMaxVal, SIGNAL(valueChanged(double)), SLOT(getFormValues()));

    connect(chkMs1Autoscale, SIGNAL(toggled(bool)), SLOT(replotMS1Spectrum()));
    connect(spnMs1Offset, SIGNAL(valueChanged(double)), SLOT(replotMS1Spectrum()));
    connect(spnMs1MzMinVal, SIGNAL(valueChanged(double)), SLOT(replotMS1Spectrum()));
    connect(spnMs1MzMaxVal, SIGNAL(valueChanged(double)), SLOT(replotMS1Spectrum()));

    //ms2 options
    connect(chkMs2Autoscale, SIGNAL(toggled(bool)), SLOT(getFormValues()));
    connect(spnMs2Offset, SIGNAL(valueChanged(double)), SLOT(getFormValues()));
    connect(spnMs2MzMin, SIGNAL(valueChanged(double)), SLOT(getFormValues()));
    connect(spnMs2MzMax, SIGNAL(valueChanged(double)), SLOT(getFormValues()));

    connect(chkMs2Autoscale, SIGNAL(toggled(bool)), SLOT(replotMS2Spectrum()));
    connect(spnMs2Offset, SIGNAL(valueChanged(double)), SLOT(replotMS2Spectrum()));
    connect(spnMs2MzMin, SIGNAL(valueChanged(double)), SLOT(replotMS2Spectrum()));
    connect(spnMs2MzMax, SIGNAL(valueChanged(double)), SLOT(replotMS2Spectrum()));

    //ms3 options
    connect(chkMs3Autoscale, SIGNAL(toggled(bool)), SLOT(getFormValues()));
    connect(spnMs3Offset, SIGNAL(valueChanged(double)), SLOT(getFormValues()));
    connect(spnMs3MzMin, SIGNAL(valueChanged(double)), SLOT(getFormValues()));
    connect(spnMs3MzMax, SIGNAL(valueChanged(double)), SLOT(getFormValues()));

    connect(chkMs3Autoscale, SIGNAL(toggled(bool)), SLOT(replotMS3Spectrum()));
    connect(spnMs3Offset, SIGNAL(valueChanged(double)), SLOT(replotMS3Spectrum()));
    connect(spnMs3MzMin, SIGNAL(valueChanged(double)), SLOT(replotMS3Spectrum()));
    connect(spnMs3MzMax, SIGNAL(valueChanged(double)), SLOT(replotMS3Spectrum()));

    //spectral agglomeration settings
    connect(spnScanFilterMinIntensity, SIGNAL(valueChanged(double)), SLOT(getFormValues()));
    connect(spnScanFilterMinIntensityFraction, SIGNAL(valueChanged(double)), SLOT(getFormValues()));
    connect(spnScanFilterMinSN, SIGNAL(valueChanged(double)), SLOT(getFormValues()));
    connect(spnScanFilterBaseline, SIGNAL(valueChanged(double)), SLOT(getFormValues()));
    connect(spnScanFilterRetainTopX, SIGNAL(valueChanged(int)), SLOT(getFormValues()));
    connect(chkScanFilterRetainHighMzFragments, SIGNAL(toggled(bool)), SLOT(getFormValues()));
    connect(spnConsensusPeakMatchTolerance, SIGNAL(valueChanged(double)), SLOT(getFormValues()));
    connect(spnConsensusMinPeakPresence, SIGNAL(valueChanged(int)), SLOT(getFormValues()));
    connect(spnConsensusMinPeakPresenceFraction, SIGNAL(valueChanged(double)), SLOT(getFormValues()));
    connect(chkConsensusAvgOnlyObserved, SIGNAL(toggled(bool)), SLOT(getFormValues()));
    connect(chkConsensusNormalizeTo10K, SIGNAL(toggled(bool)), SLOT(getFormValues()));
    connect(cmbConsensusAgglomerationType, SIGNAL(currentIndexChanged(int)), SLOT(getFormValues()));

    connect(spnScanFilterMinIntensity, SIGNAL(valueChanged(double)), SLOT(recomputeConsensusSpectrum()));
    connect(spnScanFilterMinIntensityFraction, SIGNAL(valueChanged(double)), SLOT(recomputeConsensusSpectrum()));
    connect(spnScanFilterMinSN, SIGNAL(valueChanged(double)), SLOT(recomputeConsensusSpectrum()));
    connect(spnScanFilterBaseline, SIGNAL(valueChanged(double)), SLOT(recomputeConsensusSpectrum()));
    connect(spnScanFilterRetainTopX, SIGNAL(valueChanged(int)), SLOT(recomputeConsensusSpectrum()));
    connect(chkScanFilterRetainHighMzFragments, SIGNAL(toggled(bool)), SLOT(recomputeConsensusSpectrum()));
    connect(spnConsensusPeakMatchTolerance, SIGNAL(valueChanged(double)), SLOT(recomputeConsensusSpectrum()));
    connect(spnConsensusMinPeakPresence, SIGNAL(valueChanged(int)), SLOT(recomputeConsensusSpectrum()));
    connect(spnConsensusMinPeakPresenceFraction, SIGNAL(valueChanged(double)), SLOT(recomputeConsensusSpectrum()));
    connect(chkConsensusAvgOnlyObserved, SIGNAL(toggled(bool)), SLOT(recomputeConsensusSpectrum()));
    connect(chkConsensusNormalizeTo10K, SIGNAL(toggled(bool)), SLOT(recomputeConsensusSpectrum()));
    connect(cmbConsensusAgglomerationType, SIGNAL(currentIndexChanged(int)), SLOT(recomputeConsensusSpectrum()));

}

void SettingsForm::recomputeIsotopes() { 
    getFormValues();
    if (!mainwindow) return;

    //update isotope plot in EICview
    if (mainwindow->getEicWidget()->isVisible()) {
        PeakGroup* group =mainwindow->getEicWidget()->getSelectedGroup();
        cerr << "recomputeIsotopes() " << group << endl;
        mainwindow->isotopeWidget->setPeakGroup(group);
    }
}

void SettingsForm::bringIntoView(){
    if (isVisible()){
        show();
        raise();
        activateWindow();
    } else {
        show();
    }
}

void SettingsForm::recomputeEIC() { 
    getFormValues();
    if (mainwindow && mainwindow->getEicWidget()) {
        mainwindow->getEicWidget()->recompute();
        mainwindow->getEicWidget()->replot();
    }
}

void SettingsForm::replotMS1Spectrum(){
    if (mainwindow && mainwindow->spectraWidget && mainwindow->spectraDockWidget && mainwindow->spectraDockWidget->isVisible()) {
        mainwindow->spectraWidget->findBounds(true, true);
        mainwindow->spectraWidget->replot();
        mainwindow->spectraWidget->repaint();
    }
}

void SettingsForm::replotMS2Spectrum(){
    if (mainwindow && mainwindow->fragmentationSpectraWidget && mainwindow->fragmentationSpectraDockWidget && mainwindow->fragmentationSpectraDockWidget->isVisible()) {
        mainwindow->fragmentationSpectraWidget->findBounds(true, true);
        mainwindow->fragmentationSpectraWidget->replot();
        mainwindow->fragmentationSpectraWidget->repaint();
    }
}

void SettingsForm::replotMS3Spectrum() {
    if (mainwindow && mainwindow->ms3SpectraWidget && mainwindow->ms3SpectraDockWidget && mainwindow->ms3SpectraDockWidget->isVisible()) {
        mainwindow->ms3SpectraWidget->findBounds(true, true);
        mainwindow->ms3SpectraWidget->replot();
        mainwindow->ms3SpectraWidget->repaint();
    }
}

void SettingsForm::recomputeConsensusSpectrum() {
    if (!mainwindow) return;
    if (mainwindow->ms1ScansListWidget->isVisible() && mainwindow->ms1ScansListWidget->treeWidget->selectedItems().size() > 1){
        mainwindow->ms1ScansListWidget->showInfo();
    }
    if (mainwindow->ms2ScansListWidget->isVisible() && mainwindow->ms2ScansListWidget->treeWidget->selectedItems().size() > 1){
        mainwindow->ms2ScansListWidget->showInfo();
    }
    if (mainwindow->ms3ScansListWidget->isVisible() && mainwindow->ms3ScansListWidget->treeWidget->selectedItems().size() > 1){
        mainwindow->ms3ScansListWidget->showInfo();
    }
    if (mainwindow->ms2ConsensusScansListWidget->isVisible() && mainwindow->ms2ConsensusScansListWidget->treeWidget->selectedItems().size() > 0){
        mainwindow->ms2ConsensusScansListWidget->showInfo();
    }
}

void SettingsForm::setFormValues() {
   // qDebug() << "SettingsForm::setFormValues()";

    if (!settings) return;

    //Issue 255: disable Savitzky-Golay smoothing
    int smoothingAlg = settings->value("eic_smoothingAlgorithm").toInt();
    if(smoothingAlg == EIC::SmootherType::AVG){
        eic_smoothingAlgorithm->setCurrentText("Moving Average");
    } else {
        eic_smoothingAlgorithm->setCurrentText("Gaussian");
    }

    eic_smoothingWindow->setValue(settings->value("eic_smoothingWindow").toDouble());
    grouping_maxRtWindow->setValue(settings->value("grouping_maxRtWindow").toDouble());
    maxNaturalAbundanceErr->setValue(settings->value("maxNaturalAbundanceErr").toDouble());
    maxIsotopeScanDiff->setValue(settings->value("maxIsotopeScanDiff").toDouble());
    minIsotopicCorrelation->setValue(settings->value("minIsotopicCorrelation").toDouble());
    baseline_smoothing->setValue(settings->value("baseline_smoothing").toDouble());
    baseline_quantile->setValue(settings->value("baseline_quantile").toDouble());

    if (settings->contains("txtEICScanFilter"))
        this->txtEICScanFilter->setPlainText(settings->value("txtEICScanFilter").toString());

    if (settings->contains("chkDisplayConsensusSpectrum"))
        this->chkDisplayConsensusSpectrum->setCheckState((Qt::CheckState) settings->value("chkDisplayConsensusSpectrum").toInt());

    C13Labeled->setCheckState( (Qt::CheckState) settings->value("C13Labeled").toInt() );
    N15Labeled->setCheckState( (Qt::CheckState) settings->value("N15Labeled").toInt()  );
    S34Labeled->setCheckState( (Qt::CheckState) settings->value("S34Labeled").toInt() );
    D2Labeled->setCheckState(  (Qt::CheckState) settings->value("D2Labeled").toInt()  );
//    isotopeC13Correction->setCheckState(  (Qt::CheckState) settings->value("isotopeC13Correction").toInt()  );
    chkIgnoreNaturalAbundance->setCheckState(  (Qt::CheckState) settings->value("chkIgnoreNaturalAbundance").toInt()  );
    chkExtractNIsotopes->setCheckState( (Qt::CheckState) settings->value("chkExtractNIsotopes").toInt() );

    if (settings->contains("spnMaxIsotopesToExtract"))
        spnMaxIsotopesToExtract->setValue(settings->value("spnMaxIsotopesToExtract").toInt());

    centroid_scan_flag->setCheckState( (Qt::CheckState) settings->value("centroid_scan_flag").toInt());
    scan_filter_min_intensity->setValue( settings->value("scan_filter_min_intensity").toInt());
    scan_filter_min_quantile->setValue(  settings->value("scan_filter_min_quantile").toInt());

   QStringList folders;       folders << "scriptsFolder" << "methodsFolder" << "Rprogram";
   QList<QLineEdit*> items;    items  << scriptsFolder << methodsFolder << Rprogram;

   unsigned int itemCount=0;
    foreach(QString itemName, folders) {
        if(settings->contains(itemName)) items[itemCount]->setText( settings->value(itemName).toString());
        itemCount++;
    }

    if(settings->contains("remote_server_url"))
        data_server_url->setText( settings->value("data_server_url").toString());

    if(settings->contains("centroid_scan_flag"))
        centroid_scan_flag->setCheckState( (Qt::CheckState) settings->value("centroid_scan_flag").toInt());

    if(settings->contains("scan_filter_min_intensity"))
        scan_filter_min_intensity->setValue( settings->value("scan_filter_min_intensity").toInt());

    if(settings->contains("scan_filter_min_quantile"))
        scan_filter_min_quantile->setValue( settings->value("scan_filter_min_quantile").toInt());

    //spectra widget display options

    //ms1 settings
    if (settings->contains("chkAutoMzMin") && settings->contains("chkAutoMzMax")) {
        int checkStateAutoMin = settings->value("chkAutoMzMin").toInt();
        int checkStateAutoMax = settings->value("chkAutoMzMax").toInt();
        if (checkStateAutoMin == Qt::Unchecked && checkStateAutoMax == Qt::Unchecked) {
            chkMs1Autoscale->setCheckState(Qt::Unchecked);
        } else {
            chkMs1Autoscale->setCheckState(Qt::Checked);
        }
    }

    if (settings->contains("spnMzMinOffset") && settings->contains("spnMzMaxOffset")) {
        spnMs1Offset->setValue(0.5 * (settings->value("spnMzMinOffset").toDouble() + settings->value("spnMzMaxOffset").toDouble()));
    } else if (settings->contains("spnMzMinOffset") && !settings->contains("spnMzMaxOffset")) {
        spnMs1Offset->setValue(settings->value("spnMzMinOffset").toDouble());
    } else if (!settings->contains("spnMzMinOffset") && settings->contains("spnMzMaxOffset")) {
        spnMs1Offset->setValue(settings->value("spnMzMaxOffset").toDouble());
    }

    if (settings->contains("spnMzMinVal"))
        spnMs1MzMinVal->setValue(settings->value("spnMzMinVal").toDouble());

    if (settings->contains("spnMzMaxVal"))
        spnMs1MzMaxVal->setValue(settings->value("spnMzMaxVal").toDouble());

    //ms2 settings
    if (settings->contains("chkMs2AutoMzMin") && settings->contains("chkMs2AutoMzMax")) {
        int checkStateAutoMin = settings->value("chkMs2AutoMzMin").toInt();
        int checkStateAutoMax = settings->value("chkMs2AutoMzMax").toInt();
        if (checkStateAutoMin == Qt::Unchecked && checkStateAutoMax == Qt::Unchecked) {
            chkMs2Autoscale->setCheckState(Qt::Unchecked);
        } else {
            chkMs2Autoscale->setCheckState(Qt::Checked);
        }
    }

    if (settings->contains("spnMs2MzMinOffset") && settings->contains("spnMs2MzMaxOffset")) {
        spnMs2Offset->setValue(0.5 * (settings->value("spnMs2MzMinOffset").toDouble() + settings->value("spnMs2MzMaxOffset").toDouble()));
    } else if (settings->contains("spnMs2MzMinOffset") && !settings->contains("spnMs2MzMaxOffset")) {
        spnMs2Offset->setValue(settings->value("spnMs2MzMinOffset").toDouble());
    } else if (!settings->contains("spnMs2MzMinOffset") && settings->contains("spnMs2MzMaxOffset")) {
        spnMs2Offset->setValue(settings->value("spnMs2MzMaxOffset").toDouble());
    }

    if (settings->contains("spnMs2MzMin"))
        spnMs2MzMin->setValue(settings->value("spnMs2MzMin").toDouble());

    if (settings->contains("spnMs2MzMax"))
        spnMs2MzMax->setValue(settings->value("spnMs2MzMax").toDouble());

    //ms3 settings
    if (settings->contains("chkMs3AutoMzMin") && settings->contains("chkMs3AutoMzMax")) {
        int checkStateAutoMin = settings->value("chkMs3AutoMzMin").toInt();
        int checkStateAutoMax = settings->value("chkMs3AutoMzMax").toInt();
        if (checkStateAutoMin == Qt::Unchecked && checkStateAutoMax == Qt::Unchecked) {
            chkMs3Autoscale->setCheckState(Qt::Unchecked);
        } else {
            chkMs3Autoscale->setCheckState(Qt::Checked);
        }
    }

    if (settings->contains("spnMs3MzMinOffset") && settings->contains("spnMs3MzMaxOffset")) {
        spnMs3Offset->setValue(0.5 * (settings->value("spnMs3MzMinOffset").toDouble() + settings->value("spnMs3MzMaxOffset").toDouble()));
    } else if (settings->contains("spnMs3MzMinOffset") && !settings->contains("spnMs3MzMaxOffset")) {
        spnMs3Offset->setValue(settings->value("spnMs3MzMinOffset").toDouble());
    } else if (!settings->contains("spnMs3MzMinOffset") && settings->contains("spnMs3MzMaxOffset")) {
        spnMs3Offset->setValue(settings->value("spnMs3MzMaxOffset").toDouble());
    }

    if (settings->contains("spnMs3MzMin"))
        spnMs3MzMin->setValue(settings->value("spnMs3MzMin").toDouble());

    if (settings->contains("spnMs3MzMax"))
        spnMs3MzMax->setValue(settings->value("spnMs3MzMax").toDouble());

    //bookmark warnings
    if (settings->contains("chkBkmkWarnMzRt"))
        chkBkmkWarnMzRt->setCheckState((Qt::CheckState) settings->value("chkBkmkWarnMzRt").toInt());

    if (settings->contains("chkBkmkWarnMz"))
        chkBkmkWarnMz->setCheckState((Qt::CheckState) settings->value("chkBkmkWarnMz").toInt());

    if (settings->contains("spnBkmkmzTol"))
        spnBkmkmzTol->setValue(settings->value("spnBkmkmzTol").toDouble());

    if (settings->contains("spnBkmkRtTol"))
        spnBkmkRtTol->setValue(settings->value("spnBkmkRtTol").toDouble());

    if (settings->contains("chkIncludeChildren"))
        chkIncludeChildren->setCheckState( (Qt::CheckState) settings->value("chkIncludeChildren").toInt());

    //spectral agglomeration
    if (settings->contains("spnScanFilterMinIntensity"))
        spnScanFilterMinIntensity->setValue(settings->value("spnScanFilterMinIntensity").toDouble());

    if (settings->contains("spnScanFilterMinIntensityFraction"))
        spnScanFilterMinIntensityFraction->setValue(100.0 * settings->value("spnScanFilterMinIntensityFraction").toDouble());

    if (settings->contains("spnScanFilterMinSN"))
        spnScanFilterMinSN->setValue(settings->value("spnScanFilterMinSN").toDouble());

    if (settings->contains("spnScanFilterBaseline"))
        spnScanFilterBaseline->setValue(settings->value("spnScanFilterBaseline").toDouble());

    if (settings->contains("spnScanFilterRetainTopX"))
        spnScanFilterRetainTopX->setValue(settings->value("spnScanFilterRetainTopX").toInt());

    if (settings->contains("chkScanFilterRetainHighMzFragments"))
        chkScanFilterRetainHighMzFragments->setCheckState((Qt::CheckState) settings->value("chkScanFilterRetainHighMzFragments").toInt());

    if (settings->contains("spnConsensusPeakMatchTolerance"))
        spnConsensusPeakMatchTolerance->setValue(settings->value("spnConsensusPeakMatchTolerance").toDouble());

    if (settings->contains("spnConsensusMinPeakPresence"))
        spnConsensusMinPeakPresence->setValue(settings->value("spnConsensusMinPeakPresence").toInt());

    if (settings->contains("spnConsensusMinPeakPresenceFraction"))
        spnConsensusMinPeakPresenceFraction->setValue(100.0 * settings->value("spnConsensusMinPeakPresenceFraction").toDouble());

    if (settings->contains("chkConsensusAvgOnlyObserved"))
        chkConsensusAvgOnlyObserved->setCheckState((Qt::CheckState) settings->value("chkConsensusAvgOnlyObserved").toInt());

    if (settings->contains("chkConsensusNormalizeTo10K"))
        chkConsensusNormalizeTo10K->setCheckState((Qt::CheckState) settings->value("chkConsensusNormalizeTo10K").toInt());

    if (settings->contains("cmbConsensusAgglomerationType")){

        QString cmbConsensusAgglomerationTypeStr = QString(settings->value("cmbConsensusAgglomerationType").toString());

        for (int i = 0; i < cmbConsensusAgglomerationType->count(); i++){
            QString itemString = cmbConsensusAgglomerationType->itemText(i);
            if (itemString == cmbConsensusAgglomerationTypeStr){
                cmbConsensusAgglomerationType->setCurrentIndex(i);
                break;
            }
        }
    }

}


void SettingsForm::getFormValues() {
    if (!settings) return;
    //qDebug() << "SettingsForm::getFormValues() ";


    settings->setValue("eic_smoothingAlgorithm",eic_smoothingAlgorithm->currentIndex());

    const QString currentText = eic_smoothingAlgorithm->currentText();

    //Issue 255: disable Savitzky-Golay smoothing - fall back to Gaussian
    if(currentText == "Moving Average"){
        settings->setValue("eic_smoothingAlgorithm",EIC::SmootherType::AVG);
    } else {
        settings->setValue("eic_smoothingAlgorithm",EIC::SmootherType::GAUSSIAN);
    }

    settings->setValue("eic_smoothingWindow",eic_smoothingWindow->value());
    settings->setValue("grouping_maxRtWindow",grouping_maxRtWindow->value());
    settings->setValue("maxNaturalAbundanceErr",maxNaturalAbundanceErr->value());
    settings->setValue("maxIsotopeScanDiff",maxIsotopeScanDiff->value());

    settings->setValue("minIsotopicCorrelation",minIsotopicCorrelation->value());
    settings->setValue("C13Labeled",C13Labeled->checkState() );
    settings->setValue("N15Labeled",N15Labeled->checkState() );
    settings->setValue("S34Labeled",S34Labeled->checkState() );
    settings->setValue("D2Labeled", D2Labeled->checkState()  );
//    settings->setValue("isotopeC13Correction", isotopeC13Correction->checkState()  );
    settings->setValue("chkIgnoreNaturalAbundance", chkIgnoreNaturalAbundance->checkState() );
    settings->setValue("chkExtractNIsotopes", chkExtractNIsotopes->checkState() );
    settings->setValue("spnMaxIsotopesToExtract", spnMaxIsotopesToExtract->value());

    settings->setValue("amuQ1", amuQ1->value());
    settings->setValue("amuQ3", amuQ3->value());
    settings->setValue("baseline_quantile", baseline_quantile->value());
    settings->setValue("baseline_smoothing", baseline_smoothing->value());

    settings->setValue("centroid_scan_flag", centroid_scan_flag->checkState());
    settings->setValue("scan_filter_min_intensity", scan_filter_min_intensity->value());
    settings->setValue("scan_filter_min_quantile", scan_filter_min_quantile->value());
    settings->setValue("txtEICScanFilter", txtEICScanFilter->toPlainText());
    settings->setValue("chkDisplayConsensusSpectrum", chkDisplayConsensusSpectrum->checkState());

    settings->setValue("data_server_url", data_server_url->text());

    settings->setValue("centroid_scan_flag", centroid_scan_flag->checkState());
    settings->setValue("scan_filter_min_intensity", scan_filter_min_intensity->value());
    settings->setValue("scan_filter_min_quantile", scan_filter_min_quantile->value());


    //change ionization mode
    /*
    int mode=0;
    if (ionizationMode->currentText().contains("+1") )     mode=1;
    else if ( ionizationMode->currentText().contains("-1") ) mode=-1;
    if(mainwindow) mainwindow->setIonizationMode(mode);
    settings->setValue("ionizationMode",mode);
    */

    //change ionization type

    if (ionizationType->currentText() == "EI")  MassCalculator::ionizationType = MassCalculator::EI;
    else MassCalculator::ionizationType = MassCalculator::ESI;

    mzSample::setFilter_centroidScans( centroid_scan_flag->checkState() == Qt::Checked );
    mzSample::setFilter_minIntensity( scan_filter_min_intensity->value() );
    mzSample::setFilter_intensityQuantile( scan_filter_min_quantile->value());

    if( scan_filter_polarity->currentText().contains("Positive") ) {
    	mzSample::setFilter_polarity(+1);
    } else if ( scan_filter_polarity->currentText().contains("Negative")  ) {
    	mzSample::setFilter_polarity(-1);
    } else {
    	mzSample::setFilter_polarity(0);
    }

    if( scan_filter_mslevel->currentText().contains("MS1") ) {
    	mzSample::setFilter_mslevel(1);
    } else if ( scan_filter_mslevel->currentText().contains("MS2")  ) {
    	mzSample::setFilter_mslevel(2);
    } else {
    	mzSample::setFilter_mslevel(0);
    }

    //spectra widget display options
    //ms1 display settings
    settings->setValue("chkAutoMzMin", chkMs1Autoscale->checkState());
    settings->setValue("chkAutoMzMax", chkMs1Autoscale->checkState());
    settings->setValue("spnMzMinOffset", spnMs1Offset->value());
    settings->setValue("spnMzMinVal", spnMs1MzMinVal->value());
    settings->setValue("spnMzMaxOffset", spnMs1Offset->value());
    settings->setValue("spnMzMaxVal", spnMs1MzMaxVal->value());

    //ms2 display settings
    settings->setValue("chkMs2AutoMzMin", chkMs2Autoscale->checkState());
    settings->setValue("chkMs2AutoMzMax", chkMs2Autoscale->checkState());
    settings->setValue("spnMs2MzMinOffset", spnMs2Offset->value());
    settings->setValue("spnMs2MzMin", spnMs2MzMin->value());
    settings->setValue("spnMs2MzMaxOffset", spnMs2Offset->value());
    settings->setValue("spnMs2MzMax", spnMs2MzMax->value());

    //ms3 display settings
    settings->setValue("chkMs3AutoMzMin", chkMs3Autoscale->checkState());
    settings->setValue("chkMs3AutoMzMax", chkMs3Autoscale->checkState());
    settings->setValue("spnMs3MzMinOffset", spnMs3Offset->value());
    settings->setValue("spnMs3MzMin", spnMs3MzMin->value());
    settings->setValue("spnMs3MzMaxOffset", spnMs3Offset->value());
    settings->setValue("spnMs3MzMax", spnMs3MzMax->value());

    //bookmark warn display options
    settings->setValue("chkBkmkWarnMzRt", chkBkmkWarnMzRt->checkState());
    settings->setValue("chkBkmkWarnMz", chkBkmkWarnMz->checkState());
    settings->setValue("spnBkmkmzTol", spnBkmkmzTol->value());
    settings->setValue("spnBkmkRtTol", spnBkmkRtTol->value());
    settings->setValue("chkIncludeChildren", chkIncludeChildren->checkState());

    //spectral agglomeration settings
    settings->setValue("spnScanFilterMinIntensity", spnScanFilterMinIntensity->value());
    settings->setValue("spnScanFilterMinIntensityFraction", (0.01 * spnScanFilterMinIntensityFraction->value()));
    settings->setValue("spnScanFilterMinSN", spnScanFilterMinSN->value());
    settings->setValue("spnScanFilterBaseline", spnScanFilterBaseline->value());
    settings->setValue("spnScanFilterRetainTopX", spnScanFilterRetainTopX->value());
    settings->setValue("chkScanFilterRetainHighMzFragments", chkScanFilterRetainHighMzFragments->checkState());

    settings->setValue("spnConsensusPeakMatchTolerance", spnConsensusPeakMatchTolerance->value());
    settings->setValue("spnConsensusMinPeakPresence", spnConsensusMinPeakPresence->value());
    settings->setValue("spnConsensusMinPeakPresenceFraction", (0.01 * spnConsensusMinPeakPresenceFraction->value()));
    settings->setValue("chkConsensusAvgOnlyObserved", chkConsensusAvgOnlyObserved->checkState());
    settings->setValue("chkConsensusNormalizeTo10K", chkConsensusNormalizeTo10K->checkState());
    settings->setValue("cmbConsensusAgglomerationType", cmbConsensusAgglomerationType->currentText());

}

void SettingsForm::selectFolder(QString key) {
    QString oFolder =  QApplication::applicationDirPath() + "../";
    if(settings->contains(key)) {
        oFolder =  settings->value(key).toString();
    }

    QString newFolder = QFileDialog::getExistingDirectory(this,oFolder);
    if (! newFolder.isEmpty()) {
        settings->setValue(key,newFolder);
        setFormValues();
    }
}

void SettingsForm::selectFile(QString key) {
    QString oFile = ".";
    if(settings->contains(key)) oFile =  settings->value(key).toString();
    QString newFile = QFileDialog::getOpenFileName(this,"Select file",".","*.exe");
    if (! newFile.isEmpty()) {
        settings->setValue(key,newFile);
        setFormValues();
    }
}


