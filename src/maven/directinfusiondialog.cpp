#include "directinfusiondialog.h"
#include "directinfusionprocessor.h"

extern Database DB;

DirectInfusionDialog::DirectInfusionDialog(QWidget *parent) : QDialog(parent) {
      setupUi(this);
      setModal(true);

      directInfusionUpdate = new BackgroundDirectInfusionUpdate(this);

      connect(start, SIGNAL(clicked(bool)), SLOT(analyze()));
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

void DirectInfusionDialog::analyze() {

    if (DB.compoundsDB.size() == 0 || mainwindow->samples.size() == 0) {
        qDebug() << "DirectInfusionDialog::analyze() could not be executed b/c no DI samples and/or compounds are loaded.";
        QDialog::hide();
        return;
    }

    directInfusionUpdate->setSamples(mainwindow->samples); //TODO: getVisibleSamples()
    directInfusionUpdate->setCompounds(DB.compoundsDB);
    directInfusionUpdate->setAdducts(DB.adductsDB);

    //TODO: set these values from the GUI
    directInfusionUpdate->params->minNumMatches = this->spnMatchXPeaks->value();
    directInfusionUpdate->params->productPpmTolr = this->spnFragTol->value();
    directInfusionUpdate->params->minNumUniqueMatches = 0; //TODO: currently unused, what does this do?
    directInfusionUpdate->params->isRequireAdductPrecursorMatch = this->isRequireAdductMatch;
    directInfusionUpdate->params->spectralCompositionAlgorithm = SpectralDeconvolutionAlgorithm::DO_NOTHING;

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
