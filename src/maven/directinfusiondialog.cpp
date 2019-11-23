#include "directinfusiondialog.h"
#include "directinfusionprocessor.h"

extern Database DB;

DirectInfusionDialog::DirectInfusionDialog(QWidget *parent) : QDialog(parent) {
      setupUi(this);
      setModal(true);

      //populate algorithm options
      cmbSpectralDeconvolutionAlgorithm->addItem("List All Candidates");
      cmbSpectralDeconvolutionAlgorithm->addItem("Unique Fragments Intensity Ratio");

      connect(start, SIGNAL(clicked(bool)), SLOT(analyze()));
      connect(cmbSpectralDeconvolutionAlgorithm, SIGNAL(currentIndexChanged(int)), SLOT(updateSpectralCompositionDescription()));

      updateSpectralCompositionDescription();
}

DirectInfusionDialog::~DirectInfusionDialog(){
    delete(directInfusionUpdate);
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
                    "\nPeak groups are agglomerated across samples."
                    );
        break;
    case 1:
        text = QString(
                    "Compounds are only retained if they contain unique matches (in addition to other criteria).\n"
                    "Note that this will exclude chain isomers with identical fragments.\n"
                    "Assumes that intensities in spectral libraries are normalized to a common max intensity for each spectrum.\n"
                    "\nCompute:\n"
                    "\ncompound_relative_abundance = observed_intensity / normalized_theoretical_intensity\n\n"
                    "Enumerate for all unique fragments. In case there are more than one unique fragment per compound,\n"
                    "use the highest theoretical intensity.\n"
                    "\nRepeat for all compounds, and return relative proportion as groupRank.\n"
                    "\nPeak groups are agglomerated across samples. The average relative ratio is computed.\n"
                    "If a compound is missing from a sample, the relative proportion for that sample is 0.\n"
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

    directInfusionUpdate->params->minNumMatches = this->spnMatchXPeaks->value();
    directInfusionUpdate->params->productPpmTolr = this->spnFragTol->value();
    directInfusionUpdate->params->minNumUniqueMatches = 0; //TODO: currently unused, what does this do?
    directInfusionUpdate->params->isRequireAdductPrecursorMatch = this->isRequireAdductMatch;

    if (cmbSpectralDeconvolutionAlgorithm->currentText() == "List All Candidates"){
        directInfusionUpdate->params->spectralCompositionAlgorithm = SpectralCompositionAlgorithm::ALL_CANDIDATES;
    } else if (cmbSpectralDeconvolutionAlgorithm->currentText() == "Unique Fragments Intensity Ratio"){
        directInfusionUpdate->params->spectralCompositionAlgorithm = SpectralCompositionAlgorithm::MAX_THEORETICAL_INTENSITY_UNIQUE;
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
