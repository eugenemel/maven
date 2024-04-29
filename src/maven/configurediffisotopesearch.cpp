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

void ConfigureDiffIsotopeSearch::resetSamples(vector<mzSample*> loadedSamples) {
    this->listAvailableSamples->clear();
    this->listLabeledSamples->clear();
    this->listUnlabeledSamples->clear();

    this->populateSamples(loadedSamples);
}

/**
 * @brief ConfigureDiffIsotopeSearch::populateSamples
 * @param loadedSamples
 *
 * Note that everything is done with sample names, because attaching the samples themselves to the items
 * would require subclassing QListWidgetItem.
 * Instead, cross-reference the sample names to the list of samples stored in the main window.
 */
void ConfigureDiffIsotopeSearch::populateSamples(vector<mzSample*> loadedSamples){

    bool isSampleProcessed = false;

    for (mzSample* sample : loadedSamples) {

        //should never happen, but guard against possible NPE
        if (!sample) continue;

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
        this->listAvailableSamples->addItem(item);
    }
}

bool ConfigureDiffIsotopeSearch::isSampleInListWidget(mzSample* sample, QListWidget *widget){
    if (!sample) return (false);

    bool isSampleInListWidget = false;

    for (unsigned int i = 0; i < widget->count(); i++) {

        QListWidgetItem *item = widget->item(i);
        string itemSampleName = item->text().toStdString();

        if (itemSampleName == sample->sampleName) {
            isSampleInListWidget = true;
            break;
        }
    }

    return isSampleInListWidget;
}
