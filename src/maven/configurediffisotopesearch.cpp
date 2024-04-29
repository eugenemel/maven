#include "configurediffisotopesearch.h"

ConfigureDiffIsotopeSearch::ConfigureDiffIsotopeSearch(QWidget *parent) :
	QDialog(parent) { 
        setupUi(this);
		setModal(false);
}

void ConfigureDiffIsotopeSearch::bringIntoView(){
//    if (this->mainWindow) {
//        //TODO: populate samples
//    }
    if (isVisible()){
        show();
        raise();
        activateWindow();
    } else {
        show();
    }
}

void ConfigureDiffIsotopeSearch::resetSamples() {

}
