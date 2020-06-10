#include "directinfusiondialog.h"
#include "directinfusionprocessor.h"

extern Database DB;

DirectInfusionDialog::DirectInfusionDialog(QWidget *parent) : QDialog(parent) {
      setupUi(this);
      setModal(true);

      //populate algorithm options
      cmbSpectralDeconvolutionAlgorithm->addItem("List All Candidates");                                    //0
      cmbSpectralDeconvolutionAlgorithm->addItem("Summarize all with Identical Fragment Matches");          //1
      cmbSpectralDeconvolutionAlgorithm->addItem("Summarize to Acyl Chains or Sum Composition");            //2
      cmbSpectralDeconvolutionAlgorithm->addItem("Summarize and Unique Fragments Intensity Ratio");         //3

      connect(start, SIGNAL(clicked(bool)), SLOT(analyze()));
      connect(cmbSpectralDeconvolutionAlgorithm, SIGNAL(currentIndexChanged(int)), SLOT(updateSpectralCompositionDescription()));

      updateSpectralCompositionDescription();
}

DirectInfusionDialog::~DirectInfusionDialog(){
    if (directInfusionUpdate) delete(directInfusionUpdate);
}

void DirectInfusionDialog::show(){
    QDialog::show();
}

void DirectInfusionDialog::setProgressBar(QString text, int progress, int totalSteps){
    lblProgressMsg->setText(text);
    progressBar->setRange(0, totalSteps);
    progressBar->setValue(progress);
}

void DirectInfusionDialog::updateSpectralCompositionDescription() {

    int currentIndex = cmbSpectralDeconvolutionAlgorithm->currentIndex();

    QString text("");
    switch(currentIndex){
    case 0:
        text = QString(
                    "Return all compound matches, without any attempt to determine spectral composition.\n"
                    "\nThe group rank for all matches is is 0.0\n"
                    );
        break;
    case 1:
        text = QString(
                    "Compounds are automatically summarized to a higher level if they all contain identical fragment matches.\n"
                    "\nThe group rank for all matches is is 0.0\n"
                    );
        break;
    case 2:
        text = QString(
                    "Compounds are automatically summarized to a higher level if they all contain identical fragment matches\n"
                    "and have identical acyl chain isomeric forms, or the same summed composition.\n"
                    "\nThe group rank for all matches is is 0.0\n"
                    );
        break;
    case 3:
        text = QString(
                    "Compounds are automatically summarized to a higher level if they all contain identical fragment matches\n"
                    "and have identical acyl chain isomeric forms or the same summed composition.\n"
                    "\nThis approach assumes that intensities in spectral libraries are normalized to a common max intensity for each spectrum.\n"
                    "\nWhen a group of compounds are automatically summarized, the theoretical intensity of the summarized compound is"
                    " the average of the intensity of the all compounds in the group.\n"
                    "\nCompute:\n"
                    "\ncompound_relative_abundance = observed_intensity / normalized_theoretical_intensity\n\n"
                    "This is applied to all unique fragments in each summarized compound. In case there are more than one unique fragment per summarized compound,"
                    " the highest theoretical intensity is used.\n"
                    "\nThis is repeated for all summarized compounds.  The relative proportion of each summaried compound is recorded as the Rank.\n"
                    "\nIf peak groups are agglomerated across samples, the average relative ratio is computed.\n"
                    "\nIf a compound is missing from a sample, the relative proportion for that sample is 0.\n"
                    "\nIf a sample contains no compounds, the sample is not included in the proportion computation."
                    );
        break;
    }

    txtSpecCompDescription->setText(text);

}

void DirectInfusionDialog::analyze() {

    if (DB.compoundsDB.size() == 0 || mainwindow->samples.size() == 0) {
        qDebug() << "DirectInfusionDialog::analyze() could not be executed b/c no DI samples and/or compounds are loaded.";
        QDialog::hide();
        return;
    }

    if (directInfusionUpdate) {
        delete(directInfusionUpdate);
        directInfusionUpdate = nullptr;
    }

    directInfusionUpdate = new BackgroundDirectInfusionUpdate(this);

    vector<mzSample*> visibleSamples = mainwindow->getVisibleSamples();

    directInfusionUpdate->setSamples(visibleSamples);
    directInfusionUpdate->setCompounds(DB.compoundsDB);
    directInfusionUpdate->setAdducts(DB.adductsDB);

    //scan filter
    directInfusionUpdate->params->scanFilterMinIntensity = static_cast<float>(this->spnMinIndividualMs2ScanIntensity->value());

    //consensus spectrum formation
    directInfusionUpdate->params->consensusIsIntensityAvgByObserved = this->chkIsIntensityAvgByObserved->isChecked();
    directInfusionUpdate->params->consensusIsNormalizeTo10K = this->chkIsNormalizeIntensityArray->isChecked();
    directInfusionUpdate->params->consensusMinNumMs2Scans = this->spnMinNumMs2ScansForConsensus->value();
    directInfusionUpdate->params->consensusMinFractionMs2Scans = static_cast<float>(this->spnMinFractionMs2ScansForConsensus->value()/100.0); //displayed as a perentage

    //general
    directInfusionUpdate->params->ms1IsRequireAdductPrecursorMatch = this->isRequireAdductMatch->isChecked();
    directInfusionUpdate->params->isAgglomerateAcrossSamples = this->chkAgglomerateAcrossSamples->isChecked();

    //fragment comparison
    directInfusionUpdate->params->ms2MinNumMatches = this->spnMatchXPeaks->value();
    directInfusionUpdate->params->ms2MinNumDiagnosticMatches = this->spnMatchXDiagnosticPeaks->value();
    directInfusionUpdate->params->ms2PpmTolr = this->spnFragTol->value();
    directInfusionUpdate->params->ms2MinIntensity = this->spnFragMinIntensity->value();
    directInfusionUpdate->params->ms2MinNumUniqueMatches = this->spnMatchXUniquePeaks->value();

    //precursor comparison
    directInfusionUpdate->params->ms1IsFindPrecursorIon = this->chkFindPrecursorIon->isChecked();
    directInfusionUpdate->params->ms1PpmTolr = this->spnParTol->value();
    directInfusionUpdate->params->ms1MinIntensity = this->spnParentMinIntensity->value();
    directInfusionUpdate->params->ms1ScanFilter = this->txtMs1ScanFilter->toPlainText().toStdString();

    if (cmbSpectralDeconvolutionAlgorithm->currentText() == "List All Candidates"){
        directInfusionUpdate->params->spectralCompositionAlgorithm = SpectralCompositionAlgorithm::ALL_CANDIDATES;
    } else if (cmbSpectralDeconvolutionAlgorithm->currentText() == "Summarize to Acyl Chains or Sum Composition") {
        directInfusionUpdate->params->spectralCompositionAlgorithm = SpectralCompositionAlgorithm::AUTO_SUMMARIZED_ACYL_CHAINS_SUM_COMPOSITION;
    } else if (cmbSpectralDeconvolutionAlgorithm->currentText() == "Summarize all with Identical Fragment Matches") {
        directInfusionUpdate->params->spectralCompositionAlgorithm = SpectralCompositionAlgorithm::AUTO_SUMMARIZED_IDENTICAL_FRAGMENTS;
    } else if (cmbSpectralDeconvolutionAlgorithm->currentText() == "Summarize and Unique Fragments Intensity Ratio"){
        directInfusionUpdate->params->spectralCompositionAlgorithm = SpectralCompositionAlgorithm::AUTO_SUMMARIZED_MAX_THEORETICAL_INTENSITY_UNIQUE;
    }

    QString title = QString("Direct Infusion Analysis");

    title = mainwindow->getUniquePeakTableTitle(title);

    string encodedParams = directInfusionUpdate->params->encodeParams();
    string displayParams = encodedParams;
    replace(displayParams.begin(), displayParams.end(), ';', '\n');
    replace(displayParams.begin(), displayParams.end(), '=', ' ');

    TableDockWidget* resultsTable = mainwindow->addPeaksTable(title, QString(encodedParams.c_str()), QString(displayParams.c_str()));
    resultsTable->setWindowTitle(title);

    connect(directInfusionUpdate, SIGNAL(newDirectInfusionAnnotation(DirectInfusionGroupAnnotation*, int)), resultsTable, SLOT(addDirectInfusionAnnotation(DirectInfusionGroupAnnotation*, int)));

    connect(directInfusionUpdate, SIGNAL(updateProgressBar(QString,int,int)), SLOT(setProgressBar(QString, int,int)));
    connect(directInfusionUpdate, SIGNAL(closeDialog()), SLOT(hide()));
    connect(directInfusionUpdate, SIGNAL(closeDialog()), resultsTable, SLOT(showAllGroups()));

    if ( ! directInfusionUpdate->isRunning() ) {
        directInfusionUpdate->start();	//start a background thread
    }

    //when the background thread completes, it will first close this dialog, then destroy itself.
}
