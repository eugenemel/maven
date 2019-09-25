#include "background_directinfusion_update.h"

#include "directinfusionprocessor.h"

BackgroundDirectInfusionUpdate::BackgroundDirectInfusionUpdate(QWidget*){ }

BackgroundDirectInfusionUpdate::~BackgroundDirectInfusionUpdate() {
    //TODO: need to delete any pointers?
}

void BackgroundDirectInfusionUpdate::run(void) {

//    for (unsigned int i = 0; i < samples.size(); i++){

//        mzSample* sample = samples.at(i);

//         string msgStart = string("Started sample \"").append(sample->sampleName).append("\"");
//         emit(updateProgressBar(msgStart.c_str(), i, samples.size()));

//         DirectInfusionProcessor::processSingleSample(sample, compounds);

//         string msgFinish = string("Finished sample \"").append(sample->sampleName).append("\"");
//         emit(updateProgressBar("Finished sample" , i, samples.size()));

//    }

    qDebug() << "TODO: analyze direct infusion samples.";

    quit();
    return;
}
