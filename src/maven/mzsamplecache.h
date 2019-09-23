#pragma once
#include "globals.h"

#include <queue>
class MzSampleCache {
public:
    queue<mzSample*> loadedMzSamples;
    vector<string> allMzSamples;

    mzSample* getSample();
};
