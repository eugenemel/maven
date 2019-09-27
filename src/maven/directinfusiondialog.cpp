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
    label->setText(text);
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

    //TODO: set these values from the GUI
    directInfusionUpdate->params->minNumMatches = 8;
    directInfusionUpdate->params->productPpmTolr = 20;
    directInfusionUpdate->params->minNumUniqueMatches = 0;
    directInfusionUpdate->params->isRequireAdductPrecursorMatch = true;
    directInfusionUpdate->params->spectralCompositionAlgorithm = SpectralDeconvolutionAlgorithm::DO_NOTHING;

    string title = "Direct Infusion Analysis"; //TODO: configurability?
    TableDockWidget* resultsTable = mainwindow->addPeaksTable(title.c_str());
    resultsTable->setWindowTitle(title.c_str());

    connect(directInfusionUpdate, SIGNAL(newDirectInfusionAnnotation(DirectInfusionAnnotation*)), resultsTable, SLOT(addDirectInfusionAnnotation(DirectInfusionAnnotation*)));

    connect(directInfusionUpdate, SIGNAL(updateProgressBar(QString,int,int)), SLOT(setProgressBar(QString, int,int)));
    connect(directInfusionUpdate, SIGNAL(closeDialog()), SLOT(hide()));
    connect(directInfusionUpdate, SIGNAL(closeDialog()), resultsTable, SLOT(showAllGroups()));

    if ( ! directInfusionUpdate->isRunning() ) {
        directInfusionUpdate->start();	//start a background thread
    }

    //when the background thread completes, it will first close this dialog, then destroy itself.
}
