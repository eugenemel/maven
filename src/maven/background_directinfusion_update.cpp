#include "background_directinfusion_update.h"

#include "directinfusionprocessor.h"


BackgroundDirectInfusionUpdate::BackgroundDirectInfusionUpdate(QWidget*){ }

BackgroundDirectInfusionUpdate::~BackgroundDirectInfusionUpdate() {
    //TODO: need to delete any pointers?
}

void BackgroundDirectInfusionUpdate::run(void) {

    int N = static_cast<int>(samples.size());

    /*
     * numSteps:
     * 1 * lipid summarizations
     * 1 * prepare search database
     * N * each sample individually
     * 1 * agglomerate across samples
     *
     * total = N + 3
     */
    int numSteps = N + 3;

    int stepNum = 0;

    QTime timer;
    timer.start();

    qDebug() << "Direct infusion analysis started.";

    emit(updateProgressBar("Enumerating lipid summarizations for compounds...", stepNum, numSteps));

    for (auto compound : compounds) {
        compound->metaDataMap.insert(make_pair(LipidSummarizationUtils::getAcylChainLengthSummaryAttributeKey(), LipidSummarizationUtils::getAcylChainLengthSummary(compound->name)));
        compound->metaDataMap.insert(make_pair(LipidSummarizationUtils::getAcylChainCompositionSummaryAttributeKey(), LipidSummarizationUtils::getAcylChainCompositionSummary(compound->name)));
        compound->metaDataMap.insert(make_pair(LipidSummarizationUtils::getLipidClassSummaryKey(), LipidSummarizationUtils::getLipidClassSummary(compound->name)));
    }

    stepNum++;

    emit(updateProgressBar("Preparing search database...", stepNum, numSteps));

    /**
     * ACTUAL WORK
     */
    shared_ptr<DirectInfusionSearchSet> searchDb =
            DirectInfusionProcessor::getSearchSet(samples.at(0),
                                                  compounds,
                                                  adducts,
                                                  params,
                                                  false //debug
                                                  );

    map<int, vector<DirectInfusionAnnotation*>> allDirectInfusionsAcrossSamples = {};

    for (set<int>::iterator it = searchDb->mapKeys.begin(); it != searchDb->mapKeys.end(); ++it){
        vector<DirectInfusionAnnotation*> rangeAnnotations;
        allDirectInfusionsAcrossSamples.insert(make_pair((*it), rangeAnnotations));
    }

    typedef map<int, DirectInfusionAnnotation*>::iterator diSampleIterator;

    for (unsigned int i = 0; i < samples.size(); i++){

        stepNum++;

        mzSample* sample = samples.at(i);

         string msgStart = string("Processing sample ")
                 .append(to_string(i+1))
                 .append("/")
                 .append(to_string(samples.size()))
                 .append(": \"")
                 .append(sample->sampleName)
                 .append("\"");

         emit(updateProgressBar(msgStart.c_str(), stepNum, numSteps));


         /**
          * ACTUAL WORK
          */
         map<int, DirectInfusionAnnotation*> directInfusionAnnotations =
                 DirectInfusionProcessor::processSingleSample(
                     sample,
                     searchDb,
                     params,
                     false //debug
                     );

         for (diSampleIterator it = directInfusionAnnotations.begin(); it != directInfusionAnnotations.end(); ++it){
             allDirectInfusionsAcrossSamples.at(it->first).push_back(it->second);
         }

    }

    //TODO: refactor as modular algorithm
    //Organizing across samples
    stepNum++;
    updateProgressBar("Combining results across samples...", stepNum, numSteps);

    int totalSteps = numSteps * allDirectInfusionsAcrossSamples.size();
    int emitCounter = stepNum * allDirectInfusionsAcrossSamples.size();

    /**
     * ACTUAL WORK
     */
    for (auto directInfusionAnnotation : allDirectInfusionsAcrossSamples) {
        if (!directInfusionAnnotation.second.empty()){
            emit(newDirectInfusionAnnotation(
                        DirectInfusionGroupAnnotation::createByAverageProportions(
                            directInfusionAnnotation.second,
                            params,
                            false //debug
                            ),
                        directInfusionAnnotation.first)
                    );
        }
        updateProgressBar("Populating results table...", emitCounter, totalSteps);
        emitCounter++;
    }

    qDebug() << "Direct infusion analysis completed in" << timer.elapsed() << "msec.";
    updateProgressBar("Direct infusion analysis not yet started", 0, 1);
    emit(closeDialog());
    quit();

    return;
}
