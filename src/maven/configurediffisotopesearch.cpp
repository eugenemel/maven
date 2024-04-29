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

    bool isSampleProcessed = false;

    for (mzSample* sample : loadedSamples) {

        //check unlabeled samples
        isSampleProcessed = this->isSampleInListWidget(sample, this->listUnlabeledSamples);
        if (isSampleProcessed) continue;

        //check labeled samples
        isSampleProcessed = this->isSampleInListWidget(sample, this->listLabeledSamples);
        if (isSampleProcessed) continue;

        //check unassigned samples
        isSampleProcessed = this->isSampleInListWidget(sample, this->listAvailableSamples);
        if (isSampleProcessed) continue;

        //Add sample item to list of unassigned samples
        QListWidgetItem *item = new QListWidgetItem(sample->sampleName.c_str());
        item->setData(SampleType, QVariant::fromValue(sample));
        this->listAvailableSamples->addItem(item);
    }
}

bool ConfigureDiffIsotopeSearch::isSampleInListWidget(mzSample* sample, QListWidget *widget){

    bool isSampleInListWidget = false;

    for (unsigned int i = 0; i < widget->count(); i++) {

        QListWidgetItem *item = widget->item(i);
        QVariant v = item->data(SampleType);
        mzSample *labeledSample = v.value<mzSample*>();

        if (labeledSample && labeledSample == sample) {
            isSampleInListWidget = true;
            break;
        }
    }

    return isSampleInListWidget;
}
