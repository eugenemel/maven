#include "configurediffisotopesearch.h"

ConfigureDiffIsotopeSearch::ConfigureDiffIsotopeSearch(QWidget *parent) :
	QDialog(parent) { 
        setupUi(this);
		setModal(false);
}

void ConfigureDiffIsotopeSearch::bringIntoView(){
    if (isVisible()){
        show();
        raise();
        activateWindow();
    } else {
        show();
    }
}
