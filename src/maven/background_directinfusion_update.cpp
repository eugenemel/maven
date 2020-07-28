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

        if (!compound->category.empty()){
            compound->metaDataMap.insert(make_pair(LipidSummarizationUtils::getLipidClassSummaryKey(), compound->category.at(0)));
        } else {
            compound->metaDataMap.insert(make_pair(LipidSummarizationUtils::getLipidClassSummaryKey(), LipidSummarizationUtils::getLipidClassSummary(compound->name)));
        }

        //debugging
//        qDebug() << "Compound summary data for " << compound->name.c_str() << ": ";
//        for (map<string,string>::iterator it = compound->metaDataMap.begin(); it != compound->metaDataMap.end(); ++it){
//            qDebug() << it->first.c_str() << "=" << it->second.c_str();
//        }
//        qDebug() << endl;
    }

    stepNum++;

    int afterLipidSummarization = timer.elapsed();
    qDebug() << "Enumerated lipid summarizations in " << afterLipidSummarization << "msec.";

    emit(updateProgressBar("Preparing search database...", stepNum, numSteps));

    vector<Ms3Compound*> ms3Compounds;
    if (params->ms3IsMs3Search) {
        emit(updateProgressBar("Converting loaded compound library to MS3 Compounds...", stepNum, numSteps));
        ms3Compounds = DirectInfusionProcessor::getMs3CompoundSet(compounds, true);
    }

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

    int afterDatabasePreparation = timer.elapsed();
    qDebug() << "Restructured search database in " << afterDatabasePreparation << "msec.";

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

         if (params->ms3IsMs3Search) {
             vector<Ms3SingleSampleMatch*> ms3Annotations = DirectInfusionProcessor::processSingleMs3Sample(
                         sample,
                         ms3Compounds,
                         params,
                         true // debug
                         );
         } else {
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

    }

    int afterSampleProcessing = timer.elapsed();
    qDebug() << "Completed analysis of all individual samples in " << afterSampleProcessing << "msec.";

    //Organizing across samples
    stepNum++;
    updateProgressBar("Combining results across samples...", stepNum, numSteps);

    int totalSteps = numSteps * allDirectInfusionsAcrossSamples.size();
    int emitCounter = stepNum * allDirectInfusionsAcrossSamples.size();

    for (auto directInfusionAnnotation : allDirectInfusionsAcrossSamples) {
        if (!directInfusionAnnotation.second.empty()){

            int clusterNum = directInfusionAnnotation.first;

            if (params->isAgglomerateAcrossSamples) {
                emit(newDirectInfusionAnnotation(
                            DirectInfusionGroupAnnotation::createByAverageProportions(
                                directInfusionAnnotation.second,
                                params,
                                false //debug
                                ),
                            clusterNum)
                        );
            } else {

                for (auto directInfusionAnnotationEntry : directInfusionAnnotation.second) {

                    DirectInfusionGroupAnnotation *directInfusionGroupAnnotation = new DirectInfusionGroupAnnotation();

                    directInfusionGroupAnnotation->precMzMin = directInfusionAnnotationEntry->precMzMin;
                    directInfusionGroupAnnotation->precMzMax = directInfusionAnnotationEntry->precMzMax;

                    directInfusionGroupAnnotation->sample = directInfusionAnnotationEntry->sample;
                    directInfusionGroupAnnotation->scan = directInfusionAnnotationEntry->scan;

                    directInfusionGroupAnnotation->fragmentationPattern = directInfusionAnnotationEntry->fragmentationPattern;
                    directInfusionGroupAnnotation->compounds = directInfusionAnnotationEntry->compounds;

                    directInfusionGroupAnnotation->annotationBySample.insert(make_pair(directInfusionAnnotationEntry->sample, directInfusionAnnotationEntry));

                    emit(newDirectInfusionAnnotation(directInfusionGroupAnnotation, clusterNum));
                }

            }

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
