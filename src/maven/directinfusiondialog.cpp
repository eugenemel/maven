#include "directinfusiondialog.h"
#include "directinfusionprocessor.h"

extern Database DB;

DirectInfusionDialog::DirectInfusionDialog(QWidget *parent) : QDialog(parent) {
      setupUi(this);
      setModal(true);

      //populate algorithm options
      cmbSpectralDeconvolutionAlgorithm->addItem("List All Candidates");
      cmbSpectralDeconvolutionAlgorithm->addItem("Unique Fragments: Median Intensity Ratio");

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
                    "\nEach peak group corresponds to a single sample."
                    );
        break;
    case 1:
        text = QString(
                    "Compounds are only retained if they contain unique matches (in addition to other criteria).\n"
                    "Note that this will exclude chain isomers with identical fragments.\n"
                    "Assumes that intensities in spectral libraries are normalized to a common max intensity for each spectrum.\n"
                    "\nCompute:\n"
                    "\ncompound_relative_abundance = observed_intensity / normalized_theoretical_intensity\n\n"
                    "Enumerate for all unique fragments, and take median compound_relative_abundance.\n"
                    "Repeat for all compounds, and return relative proportion as groupRank.\n"
                    "\nEach peak group corresponds to a single sample."
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

    directInfusionUpdate->setSamples(mainwindow->samples); //TODO: getVisibleSamples()
    directInfusionUpdate->setCompounds(DB.compoundsDB);
    directInfusionUpdate->setAdducts(DB.adductsDB);

    //TODO: set these values from the GUI
    directInfusionUpdate->params->minNumMatches = this->spnMatchXPeaks->value();
    directInfusionUpdate->params->productPpmTolr = this->spnFragTol->value();
    directInfusionUpdate->params->minNumUniqueMatches = 0; //TODO: currently unused, what does this do?
    directInfusionUpdate->params->isRequireAdductPrecursorMatch = this->isRequireAdductMatch;

    if (cmbSpectralDeconvolutionAlgorithm->currentText() == "List All Candidates"){
        directInfusionUpdate->params->spectralCompositionAlgorithm = SpectralCompositionAlgorithm::ALL_CANDIDATES;
    } else if (cmbSpectralDeconvolutionAlgorithm->currentText() == "Unique Fragments: Median Intensity Ratio"){
        directInfusionUpdate->params->spectralCompositionAlgorithm = SpectralCompositionAlgorithm::MEDIAN_UNIQUE;
    }

    QString title = QString("Direct Infusion Analysis");

    TableDockWidget* resultsTable = mainwindow->addPeaksTable(title.toLatin1().data());
    resultsTable->setWindowTitle(title.toLatin1().data());

    connect(directInfusionUpdate, SIGNAL(newDirectInfusionAnnotation(DirectInfusionAnnotation*, int)), resultsTable, SLOT(addDirectInfusionAnnotation(DirectInfusionAnnotation*, int)));

    connect(directInfusionUpdate, SIGNAL(updateProgressBar(QString,int,int)), SLOT(setProgressBar(QString, int,int)));
    connect(directInfusionUpdate, SIGNAL(closeDialog()), SLOT(hide()));
    connect(directInfusionUpdate, SIGNAL(closeDialog()), resultsTable, SLOT(showAllGroups()));

    if ( ! directInfusionUpdate->isRunning() ) {
        directInfusionUpdate->start();	//start a background thread
    }

    //when the background thread completes, it will first close this dialog, then destroy itself.
}
