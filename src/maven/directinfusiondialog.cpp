#include "directinfusiondialog.h"

DirectInfusionDialog::DirectInfusionDialog(QWidget *parent) : QDialog(parent) {
      setupUi(this);
      setModal(true);
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
