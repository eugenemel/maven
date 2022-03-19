#ifndef SPECTRALHIT
#define SPECTRALHIT

#include "mzSample.h"
#include <QString>
#include <QVector>
#include <QMap>

class SpectralHit {

	public: 
		double score;
		Scan* scan;
		QString sampleName;
		QVector<double>mzList;
        QVector<double>intensityList;
        QVector<QString> fragLabelList;
		float productPPM;
		int matchCount;
		float precursorMz;
		float xcorr;
		float mvh;
		int rank;
		bool decoy;
		int charge;
		int massdiff;
 		int scannum;
		QString compoundId;
        QString fragmentId;
        QString originalCompoundId;


	SpectralHit() { 
        scan = nullptr;
		precursorMz=0;
		matchCount=0; 
		productPPM=0; 
		score=0;
		xcorr=0; 
		massdiff=0;
		mvh=0; 
		rank=0; 
		decoy=false;
		charge=0;
	}

        double getMaxIntensity() { 
            double maxIntensity=0; 
            foreach(double x, intensityList) if(x>maxIntensity) maxIntensity=x;
            return  maxIntensity;
        }

        double getMinMz() {
            if (mzList.empty()) return 0.0;
            double min = mzList.first();
            foreach(double x, mzList)  if(x<min) min=x; 
            return  min;
        }

        double getMaxMz() {
            if (mzList.empty()) return 1200.0;
            double max = mzList.last();
            foreach(double x, mzList)  if(x>max) max=x;
            return max;
        }

	static bool compScan(SpectralHit* a, SpectralHit* b ) { return a->scan < b->scan; }
	static bool compPrecursorMz(SpectralHit* a, SpectralHit* b) { return a->precursorMz < b->precursorMz; }
	static bool compScore(SpectralHit* a, SpectralHit* b ) { return a->score > b->score; }
	static bool compMVH(SpectralHit* a, SpectralHit* b ) { return a->mvh > b->mvh; }

};
//Q_DECLARE_METATYPE(SpectralHit*);
#endif
