#ifndef PARALLELMASSSLICES_H
#define PARALLELMASSSLICES_H

#include "mzSample.h"
#include "mzUtils.h"

class mzSample;
using namespace std;

class ParallelMassSlicer { 

	public:
		ParallelMassSlicer()  { 
			_maxSlices=INT_MAX; 
			_minRt=FLT_MIN; _minMz=FLT_MIN; _minIntensity=FLT_MIN;
			_maxRt=FLT_MAX; _maxMz=FLT_MAX; _maxIntensity=FLT_MAX;
			_minCharge=0; _maxCharge=INT_MAX;
			_precursorPPM=1000;
		}
 
		~ParallelMassSlicer() { delete_all(slices); cache.clear(); }

		vector<mzSlice*> slices;
		mzSlice* sliceExists(float mz,float rt);
		void algorithmA();
        void algorithmB(float ppm, float minIntensity, int step);
        void algorithmC(float ppm, float minIntensity, float rtStep);
        void algorithmD(float ppm,float rtStep);
        void setMaxSlices( int x) { _maxSlices=x; }
		void setSamples(vector<mzSample*> samples)  { this->samples = samples; }
		void setMaxIntensity( float v) {  _maxIntensity=v; }
		void setMinIntensity( float v) {  _minIntensity=v; }
		void setMinRt       ( float v) {  _minRt = v; }
		void setMaxRt	    ( float v) {  _maxRt = v; }
        void setMaxMz       ( float v) {  _maxMz = v; }
        void setMinMz	    ( float v) {  _minMz = v; }
		void setMinCharge   ( float v) {  _minCharge = v; }
		void setMaxCharge   ( float v) {  _maxCharge = v; }
		void setPrecursorPPMTolr (float v) { _precursorPPM = v; }
		void addSlice(mzSlice* s);

	private:
		unsigned int _maxSlices;
		float _minRt;
		float _maxRt;
		float _minMz;
		float _maxMz;
		float _maxIntensity;
		float _minIntensity;
		int _minCharge;
		int _maxCharge;
		float _precursorPPM;

		vector<mzSample*> samples;
		multimap<int,mzSlice*>cache;

};
#endif
