#include "background_directinfusion_update.h"

#include "directinfusionprocessor.h"


BackgroundDirectInfusionUpdate::BackgroundDirectInfusionUpdate(QWidget*){ }

BackgroundDirectInfusionUpdate::~BackgroundDirectInfusionUpdate() {
    //TODO: need to delete any pointers?
}

void BackgroundDirectInfusionUpdate::run(void) {

    QTime timer;
    timer.start();

    qDebug() << "Direct infusion analysis started.";
    emit(updateProgressBar("Preparing search database...", 0, samples.size()));

    //TODO: actually get adductlist from main window
    vector<Adduct*> adducts;
    adducts.push_back(MassCalculator::MinusHAdduct);

    /**
     * ACTUAL WORK
     */
    shared_ptr<DirectInfusionSearchSet> searchDb =
            DirectInfusionProcessor::getSearchSet(samples.at(0),
                                                  compounds,
                                                  adducts,
                                                  true, //isRequireAdductPrecursorMatch
                                                  false //debug
                                                  );

    for (unsigned int i = 0; i < samples.size(); i++){

        mzSample* sample = samples.at(i);

         string msgStart = string("Processing sample ")
                 .append(to_string(i+1))
                 .append("/")
                 .append(to_string(samples.size()))
                 .append(": \"")
                 .append(sample->sampleName)
                 .append("\"");

         emit(updateProgressBar(msgStart.c_str(), (i+1), (samples.size()+1)));


         /**
          * ACTUAL WORK
          */
         vector<DirectInfusionAnnotation*> directInfusionAnnotations = DirectInfusionProcessor::processSingleSample(sample, searchDb, false);

         //DirectInfusionAnnotation* are deleted by the receiver (TableDockWidget)
         for (auto directInfusionAnnotation : directInfusionAnnotations) {
             emit(newDirectInfusionAnnotation(directInfusionAnnotation));
         }
    }

    qDebug() << "Direct infusion analysis completed in" << timer.elapsed() << "msec.";
    emit(closeDialog());
    quit();

    return;
}
