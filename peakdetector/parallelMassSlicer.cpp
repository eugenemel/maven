#include "parallelMassSlicer.h"

void ParallelMassSlicer::algorithmA() {
    delete_all(slices);
    slices.clear();
    cache.clear();
    map< string, int> seen;

    for(unsigned int i=0; i < samples.size(); i++) {
        for(unsigned int j=0; j < samples[i]->scans.size(); j++ ) {
            Scan* scan = samples[i]->scans[j];
            if ( scan->filterLine.empty() ) continue;
            if ( seen.count( scan->filterLine ) ) continue;
            mzSlice* s = new mzSlice(scan->filterLine);
            slices.push_back(s);
            seen[ scan->filterLine ]=1;
        }
    }
    cerr << "#algorithmA" << slices.size() << endl;
}

	
void ParallelMassSlicer::algorithmB(float userPPM, float minIntensity, int rtStep) { 
	delete_all(slices);
	slices.clear();
	cache.clear();

	float rtWindow=2.0;
	this->_precursorPPM=userPPM;
	this->_minIntensity=minIntensity;

	if (samples.size() > 0 and rtStep > 0 ) rtWindow = (samples[0]->getAverageFullScanTime()*rtStep);

    cerr << "#Parallel algorithmB:" << endl;
    cerr << " PPM=" << userPPM << endl;
    cerr << " rtWindow=" << rtWindow << endl;
    cerr << " rtStep=" << rtStep << endl;
    cerr << " minCharge="     << _minCharge << endl;
    cerr << " maxCharge="     << _maxCharge << endl;
    cerr << " minIntensity="  << _minIntensity << endl;
    cerr << " maxIntensity="  << _maxIntensity << endl;
    cerr << " minMz="  << _minMz << endl;
    cerr << " maxMz="  << _maxMz << endl;
    cerr << " minRt="  << _minRt << endl;
    cerr << " maxRt="  << _maxRt << endl;

#ifdef OMP_PARALLEL
     #pragma omp parallel for ordered num_threads(4) schedule(static)  
#endif
	for(unsigned int i=0; i < samples.size(); i++) {
		//if (slices.size() > _maxSlices) break;
		//
		Scan* lastScan = NULL;
		cerr << "#algorithmB:" << samples[i]->sampleName << endl;

		for(unsigned int j=0; j < samples[i]->scans.size(); j++ ) {
			Scan* scan = samples[i]->scans[j];
			if (scan->mslevel != 1 ) continue;
            if (_maxRt and !isBetweenInclusive(scan->rt,_minRt,_maxRt)) continue;
			float rt = scan->rt;

                vector<int> charges;
                if (_minCharge > 0 or _maxCharge > 0) charges = scan->assignCharges(userPPM);

                for(unsigned int k=0; k < scan->nobs(); k++ ){
                if (_maxMz and !isBetweenInclusive(scan->mz[k],_minMz,_maxMz)) continue;
                if (_maxIntensity and !isBetweenInclusive(scan->intensity[k],_minIntensity,_maxIntensity)) continue;
                if ((_minCharge or _maxCharge) and !isBetweenInclusive(charges[k],_minCharge,_maxCharge)) continue;

				float mz = scan->mz[k];
				float mzmax = mz + mz/1e6*_precursorPPM;
				float mzmin = mz - mz/1e6*_precursorPPM;

               // if(charges.size()) {
                    //cerr << "Scan=" << scan->scannum << " mz=" << mz << " charge=" << charges[k] << endl;
               // }
				mzSlice* Z;
#ifdef OMP_PARALLEL
    #pragma omp critical
#endif
			    Z = sliceExists(mz,rt);
#ifdef OMP_PARALLEL
    #pragma omp end critical
#endif
		
				if (Z) {  //MERGE
					//cerr << "Merged Slice " <<  Z->mzmin << " " << Z->mzmax 
					//<< " " << scan->intensity[k] << "  " << Z->ionCount << endl;

					Z->ionCount = std::max((float) Z->ionCount, (float ) scan->intensity[k]);
					Z->rtmax = std::max((float)Z->rtmax, rt+2*rtWindow);
					Z->rtmin = std::min((float)Z->rtmin, rt-2*rtWindow);
					Z->mzmax = std::max((float)Z->mzmax, mzmax);
					Z->mzmin = std::min((float)Z->mzmin, mzmin);
					//make sure that mz windown doesn't get out of control
					if (Z->mzmin < mz-(mz/1e6*userPPM)) Z->mzmin =  mz-(mz/1e6*userPPM);
					if (Z->mzmax > mz+(mz/1e6*userPPM)) Z->mzmax =  mz+(mz/1e6*userPPM);
					Z->mz =(Z->mzmin+Z->mzmax)/2; Z->rt=(Z->rtmin+Z->rtmax)/2;
					//cerr << Z->mz << " " << Z->mzmin << " " << Z->mzmax << " " 
					//<< ppmDist((float)Z->mzmin,mz) << endl;
				} else { //NEW SLICE
					//				if ( lastScan->hasMz(mz, userPPM) ) {
                    //cerr << "\t" << rt << "  " << mzmin << "  "  << mzmax << endl;
					mzSlice* s = new mzSlice(mzmin,mzmax, rt-2*rtWindow, rt+2*rtWindow);
					s->ionCount = scan->intensity[k];
					s->rt=scan->rt; 
					s->mz=mz;
#ifdef OMP_PARALLEL
    #pragma omp critical
#endif
					addSlice(s);
#ifdef OMP_PARALLEL
    #pragma omp end critical
#endif
				}

                //if ( slices.size() % 10000 == 0) cerr << "ParallelMassSlicer count=" << slices.size() << endl;
			} //every scan m/z
			//lastScan = scan;
		} //every scan
	} //every samples

	cerr << "#algorithmB:  Found=" << slices.size() << " slices" << endl;
	sort(slices.begin(),slices.end(), mzSlice::compIntensity);
}

	void ParallelMassSlicer::addSlice(mzSlice* s) { 
		slices.push_back(s);
		int mzRange = s->mz*10;
		cache.insert( pair<int,mzSlice*>(mzRange, s));
	}


    void ParallelMassSlicer::algorithmC(float ppm, float minIntensity, float rtWindow) {
        delete_all(slices);
        slices.clear();
        cache.clear();

        for(unsigned int i=0; i < samples.size(); i++) {
            mzSample* s = samples[i];
            for(unsigned int j=0; j < s->scans.size(); j++) {
                Scan* scan = samples[i]->scans[j];
                if (scan->mslevel != 1 ) continue;
                vector<int> positions = scan->intensityOrderDesc();
                for(unsigned int k=0; k< positions.size() && k<10; k++ ) {
                    int pos = positions[k];
                    if (scan->intensity[pos] < minIntensity) continue;
                    float rt = scan->rt;
                    float mz = scan->mz[ pos ];
                    float mzmax = mz + mz/1e6*ppm;
                    float mzmin = mz - mz/1e6*ppm;
                    if(! sliceExists(mz,rt) ) {
                        mzSlice* s = new mzSlice(mzmin,mzmax, rt-2*rtWindow, rt+2*rtWindow);
                        s->ionCount = scan->intensity[pos];
                        s->rt=scan->rt;
                        s->mz=mz;
                        slices.push_back(s);
                        int mzRange = mz*10;
                        cache.insert( pair<int,mzSlice*>(mzRange, s));
                    }
                }
            }
        }
        cerr << "#algorithmC" << slices.size() << endl;
    }



    void ParallelMassSlicer::algorithmD(float ppm, float rtWindow) {
        delete_all(slices);
        slices.clear();
        cache.clear();

        for(unsigned int i=0; i < samples.size(); i++) {
            mzSample* s = samples[i];
            for(unsigned int j=0; j < s->scans.size(); j++) {
                Scan* scan = samples[i]->scans[j];
                if (scan->mslevel != 2 ) continue;
                float rt = scan->rt;
                float mz = scan->precursorMz;
                float mzmax = mz + mz/1e6*ppm;
                float mzmin = mz - mz/1e6*ppm;
                if(! sliceExists(mz,rt) ) {
                    mzSlice* s = new mzSlice(mzmin,mzmax, rt-2*rtWindow, rt+2*rtWindow);
                    s->ionCount = scan->totalIntensity();
                    s->rt=scan->rt;
                    s->mz=mz;
                    slices.push_back(s);
                    int mzRange = mz*10;
                    cache.insert( pair<int,mzSlice*>(mzRange, s));
                }
            }
        }
        cerr << "#algorithmD" << slices.size() << endl;
    }

mzSlice*  ParallelMassSlicer::sliceExists(float mz, float rt) {
	pair< multimap<int, mzSlice*>::iterator,  multimap<int, mzSlice*>::iterator > ppp;
	ppp = cache.equal_range( (int) (mz*10) );
	multimap<int, mzSlice*>::iterator it2 = ppp.first;

	float bestDist=FLT_MAX; 
	mzSlice* best=NULL;

	for( ;it2 != ppp.second; ++it2 ) {
		mzSlice* x = (*it2).second; 
		if (mz > x->mzmin && mz < x->mzmax && rt > x->rtmin && rt < x->rtmax) {
			float d = (mz-x->mzmin) + (x->mzmax-mz);
			if ( d < bestDist ) { best=x; bestDist=d; }
		}
	}
	return best;
}
