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

    connect(recomputeEICButton, SIGNAL(clicked(bool)), SLOT(recomputeEIC()));
    connect(eic_smoothingWindow, SIGNAL(valueChanged(int)), SLOT(recomputeEIC()));
    connect(eic_smoothingAlgorithm, SIGNAL(currentIndexChanged(int)), SLOT(recomputeEIC()));
    connect(grouping_maxRtWindow, SIGNAL(valueChanged(double)), SLOT(recomputeEIC()));
    connect(baseline_smoothing, SIGNAL(valueChanged(int)), SLOT(recomputeEIC()));
    connect(baseline_quantile, SIGNAL(valueChanged(int)), SLOT(recomputeEIC()));

    connect(ionizationMode, SIGNAL(currentIndexChanged(int)), SLOT(getFormValues()));
    connect(isotopeC13Correction, SIGNAL(toggled(bool)), SLOT(getFormValues()));
    connect(amuQ1, SIGNAL(valueChanged(double)), SLOT(getFormValues()));
    connect(amuQ3, SIGNAL(valueChanged(double)), SLOT(getFormValues()));

    //isotope detection setting
    connect(C13Labeled,SIGNAL(toggled(bool)),SLOT(recomputeIsotopes()));
    connect(N15Labeled,SIGNAL(toggled(bool)),SLOT(recomputeIsotopes()));
    connect(S34Labeled,SIGNAL(toggled(bool)),SLOT(recomputeIsotopes()));
    connect(D2Labeled, SIGNAL(toggled(bool)),SLOT(recomputeIsotopes()));
    connect(isotopeC13Correction, SIGNAL(toggled(bool)), SLOT(recomputeIsotopes()));
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

    //spectra widget display options
    //ms1 options
    connect(chkAutoMzMax, SIGNAL(toggled(bool)), SLOT(getFormValues()));
    connect(chkAutoMzMin, SIGNAL(toggled(bool)), SLOT(getFormValues()));
    connect(spnMzMinOffset, SIGNAL(valueChanged(double)), SLOT(getFormValues()));
    connect(spnMzMinVal, SIGNAL(valueChanged(double)), SLOT(getFormValues()));
    connect(spnMzMaxOffset, SIGNAL(valueChanged(double)), SLOT(getFormValues()));
    connect(spnMzMaxVal, SIGNAL(valueChanged(double)), SLOT(getFormValues()));

    connect(chkAutoMzMax, SIGNAL(toggled(bool)), SLOT(replotMS1Spectrum()));
    connect(chkAutoMzMin, SIGNAL(toggled(bool)), SLOT(replotMS1Spectrum()));
    connect(spnMzMinOffset, SIGNAL(valueChanged(double)), SLOT(replotMS1Spectrum()));
    connect(spnMzMinVal, SIGNAL(valueChanged(double)), SLOT(replotMS1Spectrum()));
    connect(spnMzMaxOffset, SIGNAL(valueChanged(double)), SLOT(replotMS1Spectrum()));
    connect(spnMzMaxVal, SIGNAL(valueChanged(double)), SLOT(replotMS1Spectrum()));

    //ms2 options
    connect(chkMs2AutoMzMax, SIGNAL(toggled(bool)), SLOT(getFormValues()));
    connect(chkMs2AutoMzMin, SIGNAL(toggled(bool)), SLOT(getFormValues()));
    connect(spnMs2MzMinOffset, SIGNAL(valueChanged(double)), SLOT(getFormValues()));
    connect(spnMs2MzMin, SIGNAL(valueChanged(double)), SLOT(getFormValues()));
    connect(spnMs2MzMaxOffset, SIGNAL(valueChanged(double)), SLOT(getFormValues()));
    connect(spnMs2MzMax, SIGNAL(valueChanged(double)), SLOT(getFormValues()));

    connect(chkMs2AutoMzMax, SIGNAL(toggled(bool)), SLOT(replotMS2Spectrum()));
    connect(chkMs2AutoMzMin, SIGNAL(toggled(bool)), SLOT(replotMS2Spectrum()));
    connect(spnMs2MzMinOffset, SIGNAL(valueChanged(double)), SLOT(replotMS2Spectrum()));
    connect(spnMs2MzMin, SIGNAL(valueChanged(double)), SLOT(replotMS2Spectrum()));
    connect(spnMs2MzMaxOffset, SIGNAL(valueChanged(double)), SLOT(replotMS2Spectrum()));
    connect(spnMs2MzMax, SIGNAL(valueChanged(double)), SLOT(replotMS2Spectrum()));

    //spectral agglomeration settings
    connect(spnScanFilterMinIntensity, SIGNAL(valueChanged(double)), SLOT(getFormValues()));
    connect(spnScanFilterMinIntensityFraction, SIGNAL(valueChanged(double)), SLOT(getFormValues()));
    connect(spnScanFilterMinSN, SIGNAL(valueChanged(double)), SLOT(getFormValues()));
    connect(spnScanFilterBaseline, SIGNAL(valueChanged(int)), SLOT(getFormValues()));
    connect(spnScanFilterRetainTopX, SIGNAL(valueChanged(int)), SLOT(getFormValues()));
    connect(chkScanFilterRetainHighMzFragments, SIGNAL(toggled(bool)), SLOT(getFormValues()));
    connect(spnConsensusPeakMatchTolerance, SIGNAL(valueChanged(double)), SLOT(getFormValues()));
    connect(spnConsensusMinPeakPresence, SIGNAL(valueChanged(int)), SLOT(getFormValues()));
    connect(spnConsensusMinPeakPresenceFraction, SIGNAL(valueChanged(double)), SLOT(getFormValues()));
    connect(chkConsensusAvgOnlyObserved, SIGNAL(toggled(bool)), SLOT(getFormValues()));
    connect(chkConsensusNormalizeTo10K, SIGNAL(toggled(bool)), SLOT(getFormValues()));

    connect(spnScanFilterMinIntensity, SIGNAL(valueChanged(double)), SLOT(recomputeConsensusSpectrum()));
    connect(spnScanFilterMinIntensityFraction, SIGNAL(valueChanged(double)), SLOT(recomputeConsensusSpectrum()));
    connect(spnScanFilterMinSN, SIGNAL(valueChanged(double)), SLOT(recomputeConsensusSpectrum()));
    connect(spnScanFilterBaseline, SIGNAL(valueChanged(int)), SLOT(recomputeConsensusSpectrum()));
    connect(spnScanFilterRetainTopX, SIGNAL(valueChanged(int)), SLOT(recomputeConsensusSpectrum()));
    connect(chkScanFilterRetainHighMzFragments, SIGNAL(toggled(bool)), SLOT(recomputeConsensusSpectrum()));
    connect(spnConsensusPeakMatchTolerance, SIGNAL(valueChanged(double)), SLOT(recomputeConsensusSpectrum()));
    connect(spnConsensusMinPeakPresence, SIGNAL(valueChanged(int)), SLOT(recomputeConsensusSpectrum()));
    connect(spnConsensusMinPeakPresenceFraction, SIGNAL(valueChanged(double)), SLOT(recomputeConsensusSpectrum()));
    connect(chkConsensusAvgOnlyObserved, SIGNAL(toggled(bool)), SLOT(recomputeConsensusSpectrum()));
    connect(chkConsensusNormalizeTo10K, SIGNAL(toggled(bool)), SLOT(recomputeConsensusSpectrum()));

    setModal(true);

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

void SettingsForm::recomputeEIC() { 
    getFormValues();
    if (mainwindow && mainwindow->getEicWidget()) {
        mainwindow->getEicWidget()->recompute();
        mainwindow->getEicWidget()->replot();
    }
}

void SettingsForm::updateSmoothingWindowValue(double value) {
    settings->setValue("eic_smoothingWindow",value);
    eic_smoothingWindow->setValue(value);
    recomputeEIC();
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

void SettingsForm::recomputeConsensusSpectrum() {
    if (!mainwindow) return;
    if (mainwindow->ms1ScansListWidget->isVisible() && mainwindow->ms1ScansListWidget->treeWidget->selectedItems().size() > 1){
        mainwindow->ms1ScansListWidget->showInfo();
    }
    if (mainwindow->fragmentationEventsWidget->isVisible() && mainwindow->fragmentationEventsWidget->treeWidget->selectedItems().size() > 1){
        mainwindow->fragmentationEventsWidget->showInfo();
    }
}

void SettingsForm::setFormValues() {
   // qDebug() << "SettingsForm::setFormValues()";

    if (!settings) return;

    eic_smoothingAlgorithm->setCurrentIndex(settings->value("eic_smoothingAlgorithm").toInt());
    eic_smoothingWindow->setValue(settings->value("eic_smoothingWindow").toDouble());
    grouping_maxRtWindow->setValue(settings->value("grouping_maxRtWindow").toDouble());
    maxNaturalAbundanceErr->setValue(settings->value("maxNaturalAbundanceErr").toDouble());
    maxIsotopeScanDiff->setValue(settings->value("maxIsotopeScanDiff").toDouble());
    minIsotopicCorrelation->setValue(settings->value("minIsotopicCorrelation").toDouble());
    baseline_smoothing->setValue(settings->value("baseline_smoothing").toDouble());
    baseline_quantile->setValue(settings->value("baseline_quantile").toDouble());


    C13Labeled->setCheckState( (Qt::CheckState) settings->value("C13Labeled").toInt() );
    N15Labeled->setCheckState( (Qt::CheckState) settings->value("N15Labeled").toInt()  );
    S34Labeled->setCheckState( (Qt::CheckState) settings->value("S34Labeled").toInt() );
    D2Labeled->setCheckState(  (Qt::CheckState) settings->value("D2Labeled").toInt()  );
    isotopeC13Correction->setCheckState(  (Qt::CheckState) settings->value("isotopeC13Correction").toInt()  );
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
    if (settings->contains("chkAutoMzMin"))
        chkAutoMzMin->setCheckState((Qt::CheckState) settings->value("chkAutoMzMin").toInt());

    if (settings->contains("chkAutoMzMax"))
        chkAutoMzMax->setCheckState((Qt::CheckState) settings->value("chkAutoMzMax").toInt());

    if (settings->contains("spnMzMinOffset"))
        spnMzMinOffset->setValue(settings->value("spnMzMinOffset").toDouble());

    if (settings->contains("spnMzMaxOffset"))
        spnMzMaxOffset->setValue(settings->value("spnMzMaxOffset").toDouble());

    if (settings->contains("spnMzMinVal"))
        spnMzMinVal->setValue(settings->value("spnMzMinVal").toDouble());

    if (settings->contains("spnMzMaxVal"))
        spnMzMaxVal->setValue(settings->value("spnMzMaxVal").toDouble());

    //ms2 settings
    if (settings->contains("chkMs2AutoMzMin"))
        chkMs2AutoMzMin->setCheckState((Qt::CheckState) settings->value("chkMs2AutoMzMin").toInt());

    if (settings->contains("chkMs2AutoMzMax"))
        chkMs2AutoMzMax->setCheckState((Qt::CheckState) settings->value("chkMs2AutoMzMax").toInt());

    if (settings->contains("spnMs2MzMinOffset"))
        spnMs2MzMinOffset->setValue(settings->value("spnMs2MzMinOffset").toDouble());

    if (settings->contains("spnMs2MzMaxOffset"))
        spnMs2MzMaxOffset->setValue(settings->value("spnMs2MzMaxOffset").toDouble());

    if (settings->contains("spnMs2MzMin"))
        spnMs2MzMin->setValue(settings->value("spnMs2MzMin").toDouble());

    if (settings->contains("spnMs2MzMax"))
        spnMs2MzMax->setValue(settings->value("spnMs2MzMax").toDouble());

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

}


void SettingsForm::getFormValues() {
    if (!settings) return;
    //qDebug() << "SettingsForm::getFormValues() ";


    settings->setValue("eic_smoothingAlgorithm",eic_smoothingAlgorithm->currentIndex());
    settings->setValue("eic_smoothingWindow",eic_smoothingWindow->value());
    settings->setValue("grouping_maxRtWindow",grouping_maxRtWindow->value());
    settings->setValue("maxNaturalAbundanceErr",maxNaturalAbundanceErr->value());
    settings->setValue("maxIsotopeScanDiff",maxIsotopeScanDiff->value());

    settings->setValue("minIsotopicCorrelation",minIsotopicCorrelation->value());
    settings->setValue("C13Labeled",C13Labeled->checkState() );
    settings->setValue("N15Labeled",N15Labeled->checkState() );
    settings->setValue("S34Labeled",S34Labeled->checkState() );
    settings->setValue("D2Labeled", D2Labeled->checkState()  );
    settings->setValue("isotopeC13Correction", isotopeC13Correction->checkState()  );
    settings->setValue("chkIgnoreNaturalAbundance", chkIgnoreNaturalAbundance->checkState() );
    settings->setValue("chkExtractNIsotopes", chkExtractNIsotopes->checkState() );
    settings->setValue("spnMaxIsotopesToExtract", spnMaxIsotopesToExtract->value());

    settings->setValue("amuQ1", amuQ1->value());
    settings->setValue("amuQ3", amuQ3->value());
    settings->setValue("baseline_quantile", baseline_quantile->value());
    settings->setValue("baseline_smoothing", baseline_smoothing->value());

    settings->setValue("centroid_scan_flag", centroid_scan_flag->checkState() );
    settings->setValue("scan_filter_min_intensity", scan_filter_min_intensity->value());
    settings->setValue("scan_filter_min_quantile", scan_filter_min_quantile->value());

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
    settings->setValue("chkAutoMzMin", chkAutoMzMin->checkState());
    settings->setValue("chkAutoMzMax", chkAutoMzMax->checkState());
    settings->setValue("spnMzMinOffset", spnMzMinOffset->value());
    settings->setValue("spnMzMinVal", spnMzMinVal->value());
    settings->setValue("spnMzMaxOffset", spnMzMaxOffset->value());
    settings->setValue("spnMzMaxVal", spnMzMaxVal->value());

    //ms2 display settings
    settings->setValue("chkMs2AutoMzMin", chkMs2AutoMzMin->checkState());
    settings->setValue("chkMs2AutoMzMax", chkMs2AutoMzMax->checkState());
    settings->setValue("spnMs2MzMinOffset", spnMs2MzMinOffset->value());
    settings->setValue("spnMs2MzMin", spnMs2MzMin->value());
    settings->setValue("spnMs2MzMaxOffset", spnMs2MzMaxOffset->value());
    settings->setValue("spnMs2MzMax", spnMs2MzMax->value());

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


