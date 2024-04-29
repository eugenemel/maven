#include "configurediffisotopesearch.h"
#include "globals.h"

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

void ConfigureDiffIsotopeSearch::resetSamples() {

}

void ConfigureDiffIsotopeSearch::populateSamples(vector<mzSample*> loadedSamples){

    this->listAvailableSamples->clear();

    for (mzSample* sample : loadedSamples) {

       //check loaded samples to see if this sample already associated with unlabeled samples list
        vector<mzSample*>::iterator it = std::find(unlabeledSamples.begin(), unlabeledSamples.end(), sample);
        if (it != unlabeledSamples.end()) continue;

        //check unlabeled samples to see if sample is already associated with labeled samples list
        vector<mzSample*>::iterator it2 = std::find(labeledSamples.begin(), labeledSamples.end(), sample);
        if (it2 != labeledSamples.end()) continue;

        QListWidgetItem *item = new QListWidgetItem(sample->sampleName.c_str());

        item->setData(SampleType, QVariant::fromValue(sample));

        this->listAvailableSamples->addItem(item);
    }
}
