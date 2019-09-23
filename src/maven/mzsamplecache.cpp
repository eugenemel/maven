#include "mzsamplecache.h"

using namespace std;

mzSample* MzSampleCache::getLoadedSample(mzSample *sample) {

    for (auto loadedMzSample : loadedMzSamples){
        if (loadedMzSample->fileName == sample->fileName){
            return loadedMzSample;
        }
    }

    //clear out a slot in the list of loaded samples for this sample
    unsigned int lastIndex = loadedMzSamples.size()-1;
    delete(loadedMzSamples.at(lastIndex));
    loadedMzSamples.at(lastIndex) = nullptr;

    mzSample *loadedSample = loadSample(sample);

    loadedMzSamples.at(lastIndex) = sample;

    return sample;

}

mzSample* MzSampleCache::loadSample(mzSample *oldSample){

    mzSample *sample = new mzSample();
    sample->loadSample(oldSample->fileName.c_str());
    sample->sampleName = sample->cleanSampleName(oldSample->fileName.c_str());

    if (sample && sample->scans.size() > 0) {
        sample->enumerateSRMScans();
        sample->calculateMzRtRange();
        sample->fileName = oldSample->fileName.c_str();
        sample->isBlank = oldSample->isBlank;
//        if (oldSample->fileName.c_str().contains("blan",Qt::CaseInsensitive)){
//            sample->isBlank = true;
//        }
    }

    return sample;
}
