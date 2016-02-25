#include "mzSample.h"

Scan::Scan(mzSample* sample, int scannum, int mslevel, float rt, float precursorMz, int polarity) {
    this->sample = sample;
    this->rt = rt;
    this->scannum = scannum;
    this->precursorMz = precursorMz;
    this->mslevel = mslevel;
    this->polarity = polarity;
    this->productMz=0;
    this->collisionEnergy=0;
    this->centroided=0;
    this->precursorCharge=0;
    this->precursorIntensity=0;

    /*if ( polarity != 1 && polarity != -1 ) {
        cerr << "Warning: polarity of scan is not 1 or -1 " << polarity << endl;
    }*/
}

void Scan::deepcopy(Scan* b) {
    this->sample = b->sample;
    this->rt = b->rt;
    this->scannum = b->scannum;
    this->precursorMz = b->precursorMz;
    this->precursorIntensity= b->precursorIntensity;
    this->precursorCharge= b->precursorCharge;
    this->mslevel = b->mslevel;
    this->polarity = b->polarity;
    this->productMz= b->productMz;
    this->collisionEnergy= b->collisionEnergy;
    this->centroided= b->centroided;
    this->intensity = b->intensity;
    this->mz    = b->mz;    
    this->scanType = b->scanType;
    this->filterLine = b->filterLine;
    this->setPolarity( b->getPolarity() );

}

int Scan::findHighestIntensityPos(float _mz, float ppm) {
        float mzmin = _mz - _mz/1e6*ppm;
        float mzmax = _mz + _mz/1e6*ppm;

        vector<float>::iterator itr = lower_bound(mz.begin(), mz.end(), mzmin-0.1);
        int lb = itr-mz.begin();
        int bestPos=-1;  float highestIntensity=0;
        for(unsigned int k=lb; k < nobs(); k++ ) {
                if (mz[k] < mzmin) continue;
                if (mz[k] > mzmax) break;
                if (intensity[k] > highestIntensity ) {
                        highestIntensity=intensity[k];
                        bestPos=k;
                }
        }
        return bestPos;
}

int Scan::findClosestHighestIntensityPos(float _mz, float tolr) {
			float mzmin = _mz - tolr-0.001;
			float mzmax = _mz + tolr+0.001;

			vector<float>::iterator itr = lower_bound(mz.begin(), mz.end(), mzmin-0.1);
			int lb = itr-mz.begin();
			float highestIntensity=0; 
			for(unsigned int k=lb; k < mz.size(); k++ ) {
				if (mz[k] < mzmin) continue; 
				if (mz[k] > mzmax) break;
				if (intensity[k] > highestIntensity) highestIntensity=intensity[k];
			}
				
			int bestPos=-1; float bestScore=0;
			for(unsigned int k=lb; k < mz.size(); k++ ) {
				if (mz[k] < mzmin) continue; 
				if (mz[k] > mzmax) break;
				float deltaMz = (mz[k]-_mz); 
				float alignScore = sqrt(intensity[k] / highestIntensity)-(deltaMz*deltaMz);
			//	cerr << _mz << "\t" << k << "\t" << deltaMz << " " << alignScore << endl;
				if (bestScore < alignScore) { bestScore=alignScore; bestPos=k; }
			}
			//if(bestPos>=0) cerr << "best=" << bestPos << endl;
			return bestPos;
}


vector<int> Scan::findMatchingMzs(float mzmin, float mzmax) {
	vector<int>matches;
	vector<float>::iterator itr = lower_bound(mz.begin(), mz.end(), mzmin-1);
	int lb = itr-mz.begin();
	for(unsigned int k=lb; k < nobs(); k++ ) {
		if (mz[k] < mzmin) continue;
		if (mz[k] > mzmax) break;
		matches.push_back(k);
	}
//	cerr << "matches:" << mzmin << " " << mzmax << " " << matches.size() << endl;
	return matches;
}

//removes intensities from scan that lower than X
void Scan::quantileFilter(int minQuantile) {
        if (intensity.size() == 0 ) return;
        if( minQuantile <= 0 || minQuantile >= 100 ) return;

        int vsize=intensity.size();
        vector<float>dist = quantileDistribution(this->intensity);
        vector<float>cMz;
        vector<float>cIntensity;
        for(int i=0; i<vsize; i++ ) {
            if ( intensity[i] > dist[ minQuantile ]) {
                cMz.push_back(mz[i]);
                cIntensity.push_back(intensity[i]);
            }
        }
        vector<float>(cMz).swap(cMz);
        vector<float>(cIntensity).swap(cIntensity);
        mz.swap(cMz);
        intensity.swap(cIntensity);
}


//removes intensities from scan that lower than X
void Scan::intensityFilter(int minIntensity) {
        if (intensity.size() == 0 ) return;

        //first pass.. find local maxima in intensity space
        int vsize=intensity.size();
        vector<float>cMz;
        vector<float>cIntensity;
        for(int i=0; i<vsize; i++ ) {
           if ( intensity[i] > minIntensity) { //local maxima
                cMz.push_back(mz[i]);
                cIntensity.push_back(intensity[i]);
            }
        }
        vector<float>(cMz).swap(cMz);
        vector<float>(cIntensity).swap(cIntensity);
        mz.swap(cMz);
        intensity.swap(cIntensity);
}

void Scan::simpleCentroid() { //centroid data

        if (intensity.size() < 5 ) return;

        //pass zero smooth..
		int smoothWindow = intensity.size() / 20;
		int order=2;

		if (smoothWindow < 1 )  { smoothWindow = 2; }
		if (smoothWindow > 10 ) { smoothWindow = 10; }

        mzUtils::SavGolSmoother smoother(smoothWindow,smoothWindow,order);
		//smooth once
        vector<float>spline = smoother.Smooth(intensity);
		//smooth twice
        spline = smoother.Smooth(spline);

        //find local maxima in intensity space
        int vsize=spline.size();
        vector<float>cMz;
        vector<float>cIntensity;
        for(int i=1; i<vsize-2; i++ ) {
            if ( spline[i] > spline[i-1] &&  spline[i] > spline[i+1] ) { //local maxima in spline space
					//local maxima in real intensity space
					float maxMz=mz[i]; float maxIntensity=intensity[i];
					for(int j=i-1; j<i+1; j++) {
							if (intensity[i] > maxIntensity) { maxIntensity=intensity[i]; maxMz=mz[i]; }
					}
                	cMz.push_back(maxMz);
                	cIntensity.push_back(maxIntensity);
            }
        }

        vector<float>(cMz).swap(cMz);
        vector<float>(cIntensity).swap(cIntensity);
        mz.swap(cMz);
        intensity.swap(cIntensity);

        centroided = true;
} 

bool Scan::hasMz(float _mz, float ppm) {
    float mzmin = _mz - _mz/1e6*ppm;
    float mzmax = _mz + _mz/1e6*ppm;
	vector<float>::iterator itr = lower_bound(mz.begin(), mz.end(), mzmin);
	//cerr << _mz  << " k=" << lb << "/" << mz.size() << " mzk=" << mz[lb] << endl;
	for(unsigned int k=itr-mz.begin(); k < nobs(); k++ ) {
        if (mz[k] >= mzmin && mz[k] <= mzmax )  return true;
		if (mz[k] > mzmax ) return false;
    } 
    return false;
}

void Scan::summary() {

    cerr << "Polarity=" << getPolarity()
         << " msLevel="  << mslevel
         << " rt=" << rt
         << " m/z size=" << mz.size()
         << " ints size=" << intensity.size()
         << " precursorMz=" << precursorMz
         << " productMz=" << productMz
         << " srmID=" << filterLine
         << " totalIntensty=" << this->totalIntensity()
         << endl;

}

//generate multi charges series..endingin in change Zx,Mx
vector<float> Scan::chargeSeries(float Mx, unsigned int Zx) {
    //Mx  = observed m/z
    //Zz  = charge of Mx
    //n =  number of charge states to g
    vector<float>chargeStates(Zx+20,0);
    double M = (Mx*Zx)-Zx;
    for(unsigned int z=1; z<Zx+20; z++) chargeStates[z]=(M+z)/z;  
    return(chargeStates);
}


ChargedSpecies* Scan::deconvolute(float mzfocus, float noiseLevel, float ppmMerge, float minSigNoiseRatio, int minDeconvolutionCharge, int maxDeconvolutionCharge, int minDeconvolutionMass, int maxDeconvolutionMass, int minChargedStates ) {

    int mzfocus_pos = this->findHighestIntensityPos(mzfocus,ppmMerge);
    if (mzfocus_pos < 0 ) { cout << "ERROR: Can't find parent " << mzfocus << endl; return NULL; }
    float parentPeakIntensity=this->intensity[mzfocus_pos];
    float parentPeakSN=parentPeakIntensity/noiseLevel;
    if(parentPeakSN <=minSigNoiseRatio) return NULL;

    //cout << "Deconvolution of " << mzfocus << " pSN=" << parentPeakSN << endl;

    int scanTotalIntensity=0;
    for(unsigned int i=0; i<this->nobs();i++) scanTotalIntensity+=this->intensity[i];

    ChargedSpecies* x = new ChargedSpecies();

    for(int z=minDeconvolutionCharge; z <= maxDeconvolutionCharge; z++ ) {
        float expectedMass = (mzfocus*z)-z;     //predict what M ought to be
        int countMatches=0;
        float totalIntensity=0;
        int upCount=0;
        int downCount=0;
        int minZ=z;
        int maxZ=z;

        if (expectedMass >= maxDeconvolutionMass || expectedMass <= minDeconvolutionMass ) continue;

        bool lastMatched=false;
        float lastIntensity=parentPeakIntensity;
        for(int ii=z; ii < z+50 && ii<maxDeconvolutionCharge; ii++ ) {
            float brotherMz = (expectedMass+ii)/ii;
            int pos = this->findHighestIntensityPos(brotherMz,ppmMerge);
            float brotherIntensity = pos>=0?this->intensity[pos]:0;
            float snRatio = brotherIntensity/noiseLevel;
            if (brotherIntensity < 1.1*lastIntensity && snRatio > 2 && withinXppm(this->mz[pos]*ii-ii,expectedMass,ppmMerge)) {
                maxZ = ii;
                countMatches++;
                upCount++;
                totalIntensity += brotherIntensity;
                lastMatched=true;
                lastIntensity=brotherIntensity;
                //cout << "up.." << ii << " pos=" << pos << " snRa=" << snRatio << "\t"  << " T=" << totalIntensity <<  endl;
            } else if (lastMatched == true) {   //last charge matched ..but this one didn't..
                break;
            }
        }

        lastMatched = false;
              lastIntensity=parentPeakIntensity;
              for(int ii=z-1; ii > z-50 && ii>minDeconvolutionCharge; ii--) {
                  float brotherMz = (expectedMass+ii)/ii;
                  int pos = this->findHighestIntensityPos(brotherMz,ppmMerge);
                  float brotherIntensity = pos>=0?this->intensity[pos]:0;
                  float snRatio = brotherIntensity/noiseLevel;
                  if (brotherIntensity < 1.1*lastIntensity && snRatio > 2 && withinXppm(this->mz[pos]*ii-ii,expectedMass,ppmMerge)) {
                      minZ = ii;
                      countMatches++;
                      downCount++;
                      totalIntensity += brotherIntensity;
                      lastMatched=true;
                      lastIntensity=brotherIntensity;
                      //cout << "up.." << ii << " pos=" << pos << " snRa=" << snRatio << "\t"  << " T=" << totalIntensity <<  endl;
                  } else if (lastMatched == true) {   //last charge matched ..but this one didn't..
                      break;
                  }
              }

              if (x->totalIntensity < totalIntensity && countMatches>minChargedStates && upCount >= 2 && downCount >= 2 ) {
                      x->totalIntensity = totalIntensity;
                      x->countMatches=countMatches;
                      x->deconvolutedMass = (mzfocus*z)-z;
                      x->minZ = minZ;
                      x->maxZ = maxZ;
                      x->scan = this;
                      x->observedCharges.clear();
                      x->observedMzs.clear();
                      x->observedIntensities.clear();
                      x->upCount = upCount;
                      x->downCount = downCount;

                      float qscore=0;
                      for(int ii=minZ; ii <= maxZ; ii++ ) {
                              int pos = this->findHighestIntensityPos( (expectedMass+ii)/ii, ppmMerge );
                              if (pos > 0 ) {
                                      x->observedCharges.push_back(ii);
                                      x->observedMzs.push_back( this->mz[pos] );
                                      x->observedIntensities.push_back( this->intensity[pos] );
                                      float snRatio = this->intensity[pos]/noiseLevel;
                                      qscore += log(pow(0.97,(int)snRatio));
                              //      if(ii == z) cout << '*';
                              //      cout << setprecision(2) << snRatio << ",";
                              }
                      }
                      x->qscore = -20*qscore;
                      //cout << " upC=" << x->upCount << " downC=" << x->downCount << " qscore=" << -qscore <<  " M=" << x->deconvolutedMass << endl;
              }
          }
    // done..
    if ( x->countMatches > minChargedStates ) {
            float totalError=0; float totalIntensity=0;
            for(unsigned int i=0; i < x->observedCharges.size(); i++ ) {
                    float My = (x->observedMzs[i]*x->observedCharges[i]) - x->observedCharges[i];
                    float deltaM = abs(x->deconvolutedMass - My);
                    totalError += deltaM*deltaM;
                    totalIntensity += x->observedIntensities[i];
            }
            //cout << "\t" << mzfocus << " matches=" << x->countMatches << " totalInts=" << x->totalIntensity << " Score=" << x->qscore << endl;
            x->error = sqrt(totalError/x->countMatches);
            //cout << "-------- total Error= " << sqrt(totalError/x->countMatches) << " total Intensity=" << totalIntensity << endl;

            return x;
    } else {
            delete(x);
            x=NULL;
            return(x);





    }
}


//return pairing of mz,intensity values for top intensities.
//intensities are normalized to a maximum intensity in a scan * 100
//minFracCutoff specfies mininum relative intensity
//for example 0.05,  filters out all intensites below 5% of maxium scan intensity
vector <pair<float,float> > Scan::getTopPeaks(float minFracCutoff,float minSNRatio=1,int baseLineLevel=5) {
   unsigned int N = nobs();
   float baseline=1; 

    vector< pair<float,float> > selected;
	if(N == 0) return selected;

	vector<int> positions = this->intensityOrderDesc();	// [0]=highest-> [999]..loweset
	float maxI = intensity[positions[0]];				// intensity at highest position

   //compute baseline intensity.. 

   if(baseLineLevel>0) {
   		float cutvalueF = (100.0-(float) baseLineLevel)/101;	// baseLineLevel=0 -> cutValue 0.99 --> baseline=1
   		unsigned int mid = N * cutvalueF;						// baseline position
   		if(mid < N) baseline = intensity[positions[mid]];		// intensity at baseline 
   }

   for(unsigned int i=0; i<N; i++) {
		   int pos = positions[i];
		   if (intensity[pos]/baseline > minSNRatio && intensity[pos]/maxI > minFracCutoff) {
				   selected.push_back(make_pair(intensity[pos], mz[pos]));
		   } else {
				   break;
		   }
   }
    return selected;
}


vector<int> Scan::intensityOrderDesc() {
    vector<pair<float,int> > mzarray(nobs());
    vector<int>position(nobs());
    for(unsigned int pos=0; pos < nobs(); pos++ ) {
        mzarray[pos] = make_pair(intensity[pos],pos);
    }

   //reverse sort first key [ ie intensity ]
   sort(mzarray.rbegin(), mzarray.rend());

   //return positions in order from highest to lowest intenisty
   for(unsigned int i=0; i < mzarray.size(); i++) { position[i] = mzarray[i].second; }
   return position;
}

string Scan::toMGF() { 
    std::stringstream buffer;
    buffer << "BEGIN IONS" << endl;
    if (sample) { buffer << "TITLE=" <<  sample->sampleName << "." << scannum << "." << scannum << "." << precursorCharge << endl; }
    buffer << "PEPMASS=" << setprecision(8) << precursorMz << " " << setprecision(3) << precursorIntensity << endl;
    buffer << "RTINSECONDS=" << setprecision(9) << rt*60 << "\n";
    buffer << "CHARGE=" << precursorCharge; if(polarity < 0) buffer << "-"; else buffer << "+"; buffer << endl;
    for(unsigned int i=0; i < mz.size(); i++) {
        buffer << setprecision(8) << mz[i] << " " << setprecision(3) << intensity[i] << endl;
    }
    buffer << "END IONS" << endl;
    //cout << buffer;
    return buffer.str();
}


vector<int> Scan::assignCharges(float ppmTolr) {
    if ( nobs() == 0) {
        vector<int>empty;
        return empty;
    }

    int N = nobs(); //current scan size
    vector<int>chargeStates (N,0);
    vector<int>peakClusters = vector<int>(N,0);
    vector<int>parentPeaks = vector<int>(N,0);
    int clusterNumber=0;

    //order intensities from high to low
    vector<int>intensityOrder = intensityOrderDesc();
    double NMASS=C13_MASS-12.00;

    //a little silly, required number of peaks in a series in already to call a charge
                          //z=0,   z=1,    z=2,   z=3,    z=4,   z=5,    z=6,     z=7,   z=8,
    int minSeriesSize[9] = { 1,     2,     3,      3,      3,     4,      4,       4,     5  } ;

    //for every position in a scan
    for(int i=0; i < N; i++ ) {
        int pos=intensityOrder[i];
        float centerMz = mz[pos];
        float centerInts = intensity[pos];
       // float ppm = (0.125/centerMz)*1e6;
        float ppm = ppmTolr*2;
       // cerr << pos << " " <<  centerMz << " " << centerInts << " " << clusterNumber << endl;
        if (chargeStates[pos] != 0) continue;  //charge already assigned

        //check for charged peak groups
        int bestZ=0; int maxSeriesIntenisty=0;
        vector<int>bestSeries;


        //determine most likely charge state
        for(int z=5; z>=1; z--) {
            float delta = NMASS/z;
            int zSeriesIntensity=centerInts;
            vector<int>series;


            for(int j=1; j<6; j++) { //forward
                float mz=centerMz+(j*delta);

                int matchedPos = findHighestIntensityPos(mz,ppm);
                if (matchedPos>0 && intensity[matchedPos]<centerInts) {
                    series.push_back(matchedPos);
                    zSeriesIntensity += intensity[matchedPos];
                } else break;
            }

            for(int j=1; j<3; j++) {  //back
                float mz=centerMz-(j*delta);
                int matchedPos = findHighestIntensityPos(mz,ppm);
                if (matchedPos>0 && intensity[matchedPos]<centerInts) {
                    series.push_back(matchedPos);
                    zSeriesIntensity += intensity[matchedPos];
                } else break;
            } 
            //cerr << endl;
            if (zSeriesIntensity>maxSeriesIntenisty) { bestZ=z; maxSeriesIntenisty=zSeriesIntensity; bestSeries=series; }
        }

        //if ( i < 50) cerr << centerMz << " " << bestZ << " " << bestSeries.size() << " " << minSeriesSize[bestZ] << endl;

        //series with highest intensity is taken to be be the right one
        if(bestZ > 0 and bestSeries.size() >= minSeriesSize[bestZ] ) {
            clusterNumber++;
            int parentPeakPos=pos;
            for(unsigned int j=0; j<bestSeries.size();j++) {
                int brother_pos =bestSeries[j];
                if(bestZ > 1 and mz[brother_pos] < mz[parentPeakPos]
                        and intensity[brother_pos] < intensity[parentPeakPos]
                        and intensity[brother_pos] > intensity[parentPeakPos]*0.25)
                        parentPeakPos=brother_pos;
                chargeStates[brother_pos]=bestZ;
                peakClusters[brother_pos]=clusterNumber;
             }

           //if ( i < 50 ) cerr << "c12parent: " << mz[parentPeakPos] << endl;
            peakClusters[parentPeakPos]=clusterNumber;
            parentPeaks[parentPeakPos]=bestZ;


            //cerr << "z-series: " <<  mz[pos] << endl;
            //cerr << "parentPeak=" << mz[parentPeakPos] << " " << bestZ << endl;
        }
    }
    return parentPeaks;
}

float Scan::baseMz() {
    float maxIntensity = 0;
    float baseMz = 0;
    for(unsigned int i=0; i < nobs(); i++ ) {
        if ( intensity[i] > maxIntensity) {
            maxIntensity = intensity[i];
            baseMz = mz[i];
        }
    }
    return baseMz;
}

