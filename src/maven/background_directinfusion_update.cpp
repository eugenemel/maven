#include "background_directinfusion_update.h"

#include "directinfusionprocessor.h"


BackgroundDirectInfusionUpdate::BackgroundDirectInfusionUpdate(QWidget*){ }

BackgroundDirectInfusionUpdate::~BackgroundDirectInfusionUpdate() {
    //TODO: need to delete any pointers?
}

void BackgroundDirectInfusionUpdate::run(void) {

    int numSteps = samples.size() + 2; //prepare search database + each sample + agglomerate across samples
    int stepNum = 0;

    QTime timer;
    timer.start();

    qDebug() << "Direct infusion analysis started.";
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

    multimap<int, DirectInfusionAnnotation*> allDirectInfusionsAcrossSamples = {};

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
             allDirectInfusionsAcrossSamples.insert(make_pair(it->first, it->second));
         }

    }

    //TODO: refactor as modular algorithm
    //Organizing across samples
    stepNum++;
    updateProgressBar("Combining results across samples...", stepNum, numSteps);

    for (auto directInfusionAnnotation : allDirectInfusionsAcrossSamples) {
        emit(newDirectInfusionAnnotation(directInfusionAnnotation.second, directInfusionAnnotation.first));
    }

//    typedef multimap<int, DirectInfusionAnnotation*>::iterator diMultimapIterator;

//    for (int mapKey : searchDb->mapKeys){

//        pair<diMultimapIterator, diMultimapIterator> directInfusionAnnotations = allDirectInfusionsAcrossSamples.equal_range(mapKey);

//        for (diMultimapIterator it = directInfusionAnnotations.first; it != directInfusionAnnotations.second; ++it){
//            emit(newDirectInfusionAnnotation(it->second, it->first));
//        }

//        //for (diMultimapIterator = allDirectInfusionsAcrossSamples.equal_range(mapKey);)
//    }

    //DirectInfusionAnnotation* are deleted by the receiver (TableDockWidget)
//    for (DirectInfusionAnnotation* directInfusionAnnotation : directInfusionAnnotations) {
//        emit(newDirectInfusionAnnotation(directInfusionAnnotation));
//    }

    qDebug() << "Direct infusion analysis completed in" << timer.elapsed() << "msec.";
    updateProgressBar("Direct infusion analysis not yet started", 0, 1);
    emit(closeDialog());
    quit();

    return;
}
