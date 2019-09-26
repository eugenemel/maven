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

    string title = "Direct Infusion Analysis"; //TODO: configurability?
    TableDockWidget* peaksTable = mainwindow->addPeaksTable(title.c_str());
    peaksTable->setWindowTitle(title.c_str());

    connect(directInfusionUpdate, SIGNAL(newDirectInfusionAnnotation(DirectInfusionAnnotation*,bool, bool)), peaksTable, SLOT(addDirectInfusionAnnotation(DirectInfusionAnnotation*,bool,bool)));

    connect(directInfusionUpdate, SIGNAL(updateProgressBar(QString,int,int)), SLOT(setProgressBar(QString, int,int)));
    connect(directInfusionUpdate, SIGNAL(closeDialog()), SLOT(hide()));

    if ( ! directInfusionUpdate->isRunning() ) {
        directInfusionUpdate->start();	//start a background thread
    }

    //when the background thread completes, it will first close this dialog, then destroy itself.
}
