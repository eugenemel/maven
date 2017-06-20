#ifndef GLOBAL_H
#define GLOBAL_H

//c++
#include <math.h>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

//local
#include "mzUtils.h"
#include "mzSample.h"
#include "parallelMassSlicer.h"
#include "database.h"
#include "classifier.h"
#include "classifierNeuralNet.h"

//non gui qt classes
#include <QString>

const QString rsrcPath = ":/images";
const QString PROGRAMNAME = "MAVEN";

static const double Pi = 3.14159265358979323846264338327950288419717;

Q_DECLARE_METATYPE(mzSample*)
Q_DECLARE_METATYPE(Peak*)
Q_DECLARE_METATYPE(Compound*)
Q_DECLARE_METATYPE(Adduct*)
Q_DECLARE_METATYPE(Scan*)
Q_DECLARE_METATYPE(PeakGroup*)
Q_DECLARE_METATYPE(mzSlice*)
Q_DECLARE_METATYPE(mzSlice)

enum itemType {
		SampleType=4999,
		PeakGroupType,
		CompoundType,
		ScanType,
		EICType,
		PeakType,
		mzSliceType, 
        mzLinkType,
        AdductType
};

extern Database DB; 


#endif
