#pragma once
#include "globals.h"

#include <queue>
class MzSampleCache {
public:
    //TODO: consider restructuring to use a queue

    vector<mzSample*> loadedMzSamples; //contains scans data
    vector<mzSample*> allMzSamples;

    mzSample* getLoadedSample(mzSample* mzSample);
    mzSample* loadSample(mzSample* mzSample);
};
