#include "directinfusiondialog.h"
#include "directinfusionprocessor.h"

extern Database DB;

DirectInfusionDialog::DirectInfusionDialog(QWidget *parent) : QDialog(parent) {
      setupUi(this);
      setModal(true);

      //populate algorithm options
      cmbSpectralDeconvolutionAlgorithm->addItem("List All Candidates");
      cmbSpectralDeconvolutionAlgorithm->addItem("Summarized Compound Unique Fragments Intensity Ratio");

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

    //TODO: hard-coded values
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
                    "Compounds are only retained if they contain unique matches (in addition to other criteria).\n"
                    "\nCompounds are automatically summarized to a higher level if they all contain identical fragment matches\n"
                    "and can be summarized to a higher level (eg, two acyl chain isomers are described as a single compound).\n"
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

    //general
    directInfusionUpdate->params->isRequireAdductPrecursorMatch = this->isRequireAdductMatch->isChecked();
    directInfusionUpdate->params->isAgglomerateAcrossSamples = this->chkAgglomerateAcrossSamples->isChecked();

    //fragment related
    directInfusionUpdate->params->minNumMatches = this->spnMatchXPeaks->value();
    directInfusionUpdate->params->minNumDiagnosticFragments = this->spnMatchXDiagnosticPeaks->value();
    directInfusionUpdate->params->productPpmTolr = this->spnFragTol->value();
    directInfusionUpdate->params->productMinIntensity = this->spnFragMinIntensity->value();

    //precursor related
    directInfusionUpdate->params->isFindPrecursorIonInMS1Scan = this->chkFindPrecursorIon->isChecked();
    directInfusionUpdate->params->parentPpmTolr = this->spnParTol->value();
    directInfusionUpdate->params->parentMinIntensity = this->spnParentMinIntensity->value();
    directInfusionUpdate->params->ms1ScanFilter = this->txtMs1ScanFilter->toPlainText().toStdString();

    //unused
    directInfusionUpdate->params->minNumUniqueMatches = 0; //TODO: currently unused, what does this do?

    if (cmbSpectralDeconvolutionAlgorithm->currentText() == "List All Candidates"){
        directInfusionUpdate->params->spectralCompositionAlgorithm = SpectralCompositionAlgorithm::ALL_CANDIDATES;
    } else if (cmbSpectralDeconvolutionAlgorithm->currentText() == "Summarized Compound Unique Fragments Intensity Ratio"){
        directInfusionUpdate->params->spectralCompositionAlgorithm = SpectralCompositionAlgorithm::AUTO_SUMMARIZED_MAX_THEORETICAL_INTENSITY_UNIQUE;
    }

    QString title = QString("Direct Infusion Analysis");

    title = mainwindow->getUniquePeakTableTitle(title);

    TableDockWidget* resultsTable = mainwindow->addPeaksTable(title.toLatin1().data());
    resultsTable->setWindowTitle(title.toLatin1().data());

    connect(directInfusionUpdate, SIGNAL(newDirectInfusionAnnotation(DirectInfusionGroupAnnotation*, int)), resultsTable, SLOT(addDirectInfusionAnnotation(DirectInfusionGroupAnnotation*, int)));

    connect(directInfusionUpdate, SIGNAL(updateProgressBar(QString,int,int)), SLOT(setProgressBar(QString, int,int)));
    connect(directInfusionUpdate, SIGNAL(closeDialog()), SLOT(hide()));
    connect(directInfusionUpdate, SIGNAL(closeDialog()), resultsTable, SLOT(showAllGroups()));

    if ( ! directInfusionUpdate->isRunning() ) {
        directInfusionUpdate->start();	//start a background thread
    }

    //when the background thread completes, it will first close this dialog, then destroy itself.
}
