#include "msmsscoringsettings.h"

MSMSScoringSettingsDialog::MSMSScoringSettingsDialog(QWidget *parent) :
	QDialog(parent) { 
        setupUi(this);
		setModal(false);
}

void MSMSScoringSettingsDialog::setLipidClassAdductFile(){

    const QString name = QFileDialog::getOpenFileName(this,
                "Select Lipid Class/Adduct Parameters ", ".",tr("Comma-Separated Values (*.csv)"));

    this->classAdductFileName->setText(name);
}

void MSMSScoringSettingsDialog::bringIntoView(){
    if (isVisible()){
        show();
        raise();
        activateWindow();
    } else {
        show();
    }
}
