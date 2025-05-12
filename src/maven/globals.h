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
#include "classifierNeuralNet.h"
#include "directinfusionprocessor.h"

//non gui qt classes
#include <QString>

const QString rsrcPath = ":/images";
const QString PROGRAMNAME = "MAVEN";
const int darkModeBackgroundColor = 30; // QColor(ARGB 1, 0.117647, 0.117647, 0.117647) red == blue == green == 30

static const double Pi = 3.14159265358979323846264338327950288419717;

Q_DECLARE_METATYPE(mzSample*)
Q_DECLARE_METATYPE(Peak*)
Q_DECLARE_METATYPE(Compound*)
Q_DECLARE_METATYPE(Adduct*)
Q_DECLARE_METATYPE(Scan*)
Q_DECLARE_METATYPE(PeakGroup*)
Q_DECLARE_METATYPE(mzSlice*)
Q_DECLARE_METATYPE(mzSlice)
Q_DECLARE_METATYPE(DirectInfusionGroupAnnotation*)
Q_DECLARE_METATYPE(SRMTransition*)
Q_DECLARE_METATYPE(SRMTransition)
Q_DECLARE_METATYPE(PeakAndGroup)

enum itemType {
		SampleType=4999,
		PeakGroupType,
		CompoundType,
		ScanType,
		EICType,
		PeakType,
		mzSliceType, 
        mzLinkType,
        AdductType,
        ScanVectorType,
        SRMTransitionType,
        PeandAndGroupType
};

extern Database DB; 


#endif
