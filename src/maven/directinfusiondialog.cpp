#include "directinfusiondialog.h"
#include "directinfusionprocessor.h"

extern Database DB;

DirectInfusionDialog::DirectInfusionDialog(QWidget *parent) : QDialog(parent) {
      setupUi(this);
      setModal(true);

      connect(start, SIGNAL(clicked(bool)), SLOT(analyze()));
}

DirectInfusionDialog::~DirectInfusionDialog(){ }

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

    directInfusionUpdate = new BackgroundDirectInfusionUpdate(this);
    directInfusionUpdate->setSamples(mainwindow->samples);
    directInfusionUpdate->setCompounds(DB.compoundsDB);

    connect(directInfusionUpdate, SIGNAL(updateProgressBar(QString,int,int)), SLOT(setProgressBar(QString, int,int)));

    if ( ! directInfusionUpdate->isRunning() ) {

        qDebug() << "DirectInfusionDialog::analyze() Started.";

        directInfusionUpdate->start();	//start a background thread
        directInfusionUpdate->wait();

        qDebug() << "DirectInfusionDialog::analyze() Completed.";
    }

    delete(directInfusionUpdate);

    QDialog::hide();
}
