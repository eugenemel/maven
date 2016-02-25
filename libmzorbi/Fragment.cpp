#include "Fragment.h"
#include "mzSample.h"

//empty constructor
Fragment::Fragment() { precursorMz = 0; polarity=1; scanNum=0; rt=0; collisionEnergy=0; consensus=NULL; precursorCharge=0; isDecoy=false; sortedBy=None; group=NULL;}


//build fragment based on MS2 scan
Fragment::Fragment(Scan* scan, float minFractionalIntensity, float minSigNoiseRatio,unsigned int maxFragmentSize) {
    this->precursorMz = scan->precursorMz;
    this->collisionEnergy = scan->collisionEnergy;
    this->polarity = scan->getPolarity();
    this->sampleName = scan->sample->sampleName;
    this->scanNum = scan->scannum;
    this->precursorCharge = scan->precursorCharge;
    this->sortedBy= SortType::Mz;

    int baseLineLevel=5; //lowest 5% of data are considered to be baseline

    //don't worry about baseline.. keeping all points
    if(scan->nobs()<maxFragmentSize) { 
        minSigNoiseRatio=0; 
        minFractionalIntensity=0;
    }

    vector<pair<float,float> >mzarray = scan->getTopPeaks(minFractionalIntensity,minSigNoiseRatio,baseLineLevel);

    for(unsigned int j=0; j<mzarray.size() && j < maxFragmentSize; j++ ) {
        this->mzs.push_back(mzarray[j].second);
        this->intensity_array.push_back(mzarray[j].first);
    }
    this->obscount = vector<int>( this->mzs.size(), 1);
    this->group = NULL;
    this->consensus =NULL;
    this->rt = scan->rt;
}


//delete
Fragment::~Fragment() {
    mzUtils::delete_all(brothers);
    delete(consensus);
}

//make a copy of Fragment.
Fragment::Fragment( Fragment* other) { 
    this->precursorMz = other->precursorMz;
    this->polarity = other->polarity;
    this->mzs = other->mzs;
    this->intensity_array = other->intensity_array;
    this->obscount = other->obscount;
    this->consensus = other->consensus;
    this->scanNum = other->scanNum;
    this->sampleName = other->sampleName;
    this->collisionEnergy = other->collisionEnergy;
    this->precursorCharge= other->precursorCharge;
    this->sortedBy = other->sortedBy;
    this->group = other->group;
}

void Fragment::appendBrothers(Fragment* other) {
    //copy other's brothers
    for(unsigned int i=0; i < other->brothers.size(); i++ ) {
        this->brothers.push_back( other->brothers[i]);
    }

    //empty brothers link
    other->brothers.clear();

    //append other to brother link
    this->brothers.push_back(other);
}

void Fragment::printMzList() { 
    for(unsigned int i=0; i<mzs.size(); i++ ) { cerr << setprecision(3) << mzs[i] << " "; }
}

int Fragment::findClosestHighestIntensityPos(float _mz, float tolr) {
    float mzmin = _mz - tolr;
    float mzmax = _mz + tolr+0.001;

    vector<float>::iterator itr = lower_bound(mzs.begin(), mzs.end(), mzmin-1);
    int lb = itr-mzs.begin();
    float highestIntensity=0; 
    for(unsigned int k=lb; k < mzs.size(); k++ ) {
        if (mzs[k] < mzmin) continue; 
        if (mzs[k] > mzmax) break;
        if (intensity_array[k] > highestIntensity) highestIntensity=intensity_array[k];
    }

    int bestPos=-1; float bestScore=0;
    for(unsigned int k=lb; k < mzs.size(); k++ ) {
        if (mzs[k] < mzmin) continue; 
        if (mzs[k] > mzmax) break;
        float deltaMz = (mzs[k]-_mz); 
        float alignScore = sqrt(intensity_array[k] / highestIntensity)-(deltaMz*deltaMz);
        //	cerr << _mz << "\t" << k << "\t" << deltaMz << " " << alignScore << endl;
        if (bestScore < alignScore) { bestScore=alignScore; bestPos=k; }
    }
    //if(bestPos>=0) cerr << "best=" << bestPos << endl;
    return bestPos;
}


void Fragment::printFragment(float productAmuToll,unsigned int limitTopX=10) {
    cerr << setprecision(10) << " preMz=" << precursorMz << " ce=" << this->collisionEnergy <<  " scan=" << this->scanNum << endl;
    cerr << " mzs: \t";
    for(unsigned int i=0; i<mzs.size() && i < limitTopX; i++ ) {
        cerr << setprecision(6) << mzs[i] << " ";
        if (annotations.count(i))  {
            string ionName = annotations[i];
            cerr  << "[" << ionName << "] "; 
        }

        //cerr  << "[" << (int) intensity_array[i] << "] "; 
        // cerr << "[" << obscount[i] << "] ";
    }
    cerr << endl;

    for(unsigned int i=0; i<brothers.size(); i++) {
        double matchScore = this->compareToFragment(brothers[i],productAmuToll);
        cerr << setprecision(3) << " similarity=" << matchScore;

        cerr << " brother mzs=\t";
        for(unsigned int j=0; j<brothers[i]->mzs.size() && j< limitTopX; j++ ) {
            cerr << setprecision(3) << brothers[i]->mzs[j] << " ";
            //ycerr << "[" << (int) brothers[i]->intensity_array[j] << "] ";
            //cerr  << "[" << brothers[i]->obscount[j] << "] ";
        }
        cerr << endl;
    }

    if (this->consensus) { 
        cerr << "Cons mzs=\t";
        for(unsigned int j=0; j<this->consensus->mzs.size(); j++ )  cerr << setprecision(3) << this->consensus->mzs[j] << " "; 
        cerr << endl;
    }
}

void Fragment::printConsensusMS2(ostream& outstream, double minConsensusFraction) {

    if(this->consensus == NULL) return; 
    int consensusSize = this->brothers.size()+1;

    outstream << "S" << "\t" << consensus->scanNum << "\t" << consensus->scanNum << "\t" << consensus->precursorMz << endl;

    for(unsigned int i=0; i<consensus->mzs.size(); i++ ) { 
        float fracObserved = ((float) consensus->obscount[i])/consensusSize;
        if (fracObserved > minConsensusFraction )  {
            outstream << setprecision(5) << consensus->mzs[i] << "\t";
            outstream << consensus->intensity_array[i] << endl;
        }
    }
    outstream << endl;
}


void Fragment::printConsensusMGF(ostream& outstream, double minConsensusFraction) {

    if(this->consensus == NULL) return;
    int consensusSize = this->brothers.size()+1;
    int precursorCharge=this->precursorCharge;



    outstream << "BEGIN IONS" << endl;
    if (!sampleName.empty()) { outstream << "TITLE=" <<  sampleName << "." << consensus->scanNum << "." << consensus->scanNum << "." << precursorCharge << endl; }
    outstream << "PEPMASS=" << setprecision(8) << precursorMz << endl;
    outstream << "RTINSECONDS=" << setprecision(9) << rt*60 << "\n";
    outstream << "CHARGE=" << precursorCharge; if(polarity < 0) outstream << "-"; else outstream << "+"; outstream << endl;

    for(unsigned int i=0; i<consensus->mzs.size(); i++ ) {
        float fracObserved = ((float) consensus->obscount[i])/consensusSize;
        if (fracObserved > minConsensusFraction )  {
            outstream << setprecision(8) << consensus->mzs[i] << "\t";
            outstream << consensus->intensity_array[i] << endl;
        }
    }
    outstream << "END IONS" << endl;
}

void Fragment::printConsensusNIST(ostream& outstream, double minConsensusFraction, float productAmuToll, Compound* compound=0) {

    string COMPOUNDNAME = "Unknown";
    if (compound)  COMPOUNDNAME = compound->id;

    //compute average MVH score
    float avgMVH=0;
    //compute retention time window
    StatisticsVector<float>retentionTimes; retentionTimes.push_back(this->rt);
    for(unsigned int i=0; i<brothers.size();i++ ) retentionTimes.push_back(brothers[i]->rt);

    float precursorMz =  this->precursorMz;
    float MW=this->precursorMz;

    //scan list
    string scanList = this->sampleName + "." + integer2string(this->scanNum);
    for(unsigned int i=0; i<brothers.size();i++ )  scanList += ";" + brothers[i]->sampleName + "." + integer2string(brothers[i]->scanNum);

    int consensusSize = this->brothers.size()+1;

    if (compound) {
        outstream << "ID: " << compound->id << endl;
        if (!compound->formula.empty()) outstream << "FORMULA: " << compound->formula << endl;
        if (compound->category.size())    outstream << "CATEGORY: " << compound->category.front() << endl;
        if (!compound->smileString.empty()) outstream << "SMILE: " << compound->smileString << endl;
        outstream << "LOGP: " << compound->logP << endl;

    }
    outstream << "MW: " << setprecision(10) << MW << endl;
    outstream << "PrecursorMZ: " << setprecision(10) << precursorMz << endl;
    outstream << "Comment: ";
    outstream << " Spec=Consensus";
    outstream << " Parent=" << setprecision(10) << precursorMz;
    if(this->collisionEnergy) outstream << " collisionEnergy=" << this->collisionEnergy;
    if(avgMVH) outstream << " AvgMVH=" << avgMVH;
    outstream << " AvgRt=" << retentionTimes.mean();

    //outstream << " MaxRt=" << retentionTimes.maximum();
    outstream << " StdRt=" << sqrt(retentionTimes.variance());
    //outstream << " Scanlist=" << scanList;
    outstream << " ConsensusSize=" << consensusSize << endl;
    outstream << "NumPeaks: " << mzs.size() << endl;


    for(unsigned int i=0; i< mzs.size(); i++ ) {
        float fracObserved = ((float) obscount[i])/consensusSize;
        if (fracObserved > minConsensusFraction )  {

            //mz  ints  annotation
            if (productAmuToll < 0.1 ) outstream << setprecision(7) << mzs[i] << "\t";
            else outstream << setprecision(6) << mzs[i] << "\t";

            outstream << (int) intensity_array[i] << "\t";

            string ionName = "?";
            if (annotations.count(i))  {
                ionName = annotations[i];
            }
            outstream << "\"" << ionName << " " << obscount[i] << "/" <<  consensusSize << "\"" << endl;
        }
    }
    outstream << endl;
}

void Fragment::printInclusionList(bool printHeader, ostream& outstream, string COMPOUNDNAME) {
    if(this->consensus == NULL) return; 
    bool printAvgRtTime=true;

    if (printHeader) { 
        outstream << "mz,ionizationmode,rtmin,rtmax,ce,charge,rt,comment\n";
    }

    //compute retention time window
    StatisticsVector<float>retentionTimes; retentionTimes.push_back(this->rt);
    for(unsigned int i=0; i<brothers.size();i++ ) retentionTimes.push_back(brothers[i]->rt);

    float medRt = retentionTimes.median();
    float stdRt	 = sqrt(retentionTimes.variance());

    if (stdRt > 1 ) stdRt = 1;
    if (stdRt <= 0 ) stdRt = 0.5;
    float minRt =  medRt - 3*stdRt;
    float maxRt =  medRt + 3*stdRt;

    //start otput

    outstream << setprecision(10) << consensus->precursorMz << ",";
    this->polarity > 0 ? outstream << "Positive," : outstream << "Negative,";
    outstream << minRt << ",";
    outstream << maxRt << ",";

    if (this->collisionEnergy ) {
        outstream << this->collisionEnergy << ","; 	//CE
    } else {
        outstream << 25 << ","; 	//default CE set to 25
    }
    outstream << "1,";

    if (printAvgRtTime) { 
        outstream << medRt << ",";
    }

    outstream << COMPOUNDNAME << ":" << setprecision(8) << consensus->precursorMz << " cid:" << consensus->collisionEnergy << endl;

}

FragmentationMatchScore Fragment::scoreMatch(Fragment* other, float productAmuToll) {
    FragmentationMatchScore s;
    if (mzs.size() < 2 or other->mzs.size() < 2) return s;

    //which one is smaller;
    Fragment* a = this;
    Fragment* b =  other;

    s.ppmError = abs((a->precursorMz-b->precursorMz)/a->precursorMz*1e6);
    vector<int>ranks = compareRanks(a,b,productAmuToll);
    for(int rank: ranks) { if(rank != -1) s.numMatches++; }
    s.fractionMatched = s.numMatches / a->nobs();
    s.spearmanRankCorrelation = spearmanRankCorrelation(ranks);
    s.ticMatched = ticMatched(ranks);
    s.mzFragError =  mzErr(ranks,b);
    s.dotProduct = dotProduct(ranks,b);

    return s;
}


double Fragment::compareToFragment(Fragment* other, float productAmuToll) { 
    if (mzs.size() < 2 or other->mzs.size() < 2) return 0; 	
    //which one is smaller;
    Fragment* a = this;
    Fragment* b =  other;
    //if (b->mzs.size() < a->mzs.size() ) { a=other; b=this; }
    vector<int>ranks = compareRanks(a,b,productAmuToll);
    //return spearmanRankCorrelation(ranks);
    //return fractionMatched(ranks);
    return ticMatched(ranks);
}

vector<int> Fragment::compareRanks( Fragment* a, Fragment* b, float productAmuToll) {
    bool verbose=false;
    if (verbose) { cerr << "\t\t "; a->printMzList(); cerr << "\nvs \n"; b->printMzList();  }
    vector<int> ranks (a->mzs.size(),-1);	//missing value == -1
    for(unsigned int i=0; i<a->mzs.size(); i++ ) {
        for( unsigned int j=0; j < b->mzs.size(); j++ ) {
            if (abs(a->mzs[i]-b->mzs[j])<productAmuToll) { ranks[i] = j; break; }   //this needs optimization.. 
        }
    }
    if (verbose) { cerr << " compareranks: ";
        for(unsigned int i=0; i < ranks.size(); i++ ) cerr << ranks[i] << " "; }
        return ranks;
}


vector<int> Fragment::locatePositions( Fragment* a, Fragment* b, float productAmuToll) {
    bool verbose=false;
    if (verbose) { cerr << "\t\t "; a->printMzList(); cerr << "\nvs \n"; b->printMzList();  }
    vector<int> ranks (a->mzs.size(),-1);	//missing value == -1
    for(unsigned int i=0; i<a->mzs.size(); i++ ) {
        int pos = b->findClosestHighestIntensityPos(a->mzs[i],productAmuToll);
        ranks[i] = pos;
    }
    if (verbose) { cerr << " compareranks: "; for(unsigned int i=0; i < ranks.size(); i++ ) cerr << ranks[i] << " "; }
    return ranks;
}

void Fragment::buildConsensus(float productAmuToll) {
    if(this->consensus != NULL) {  delete(this->consensus); this->consensus=NULL; }

    Fragment* Cons = new Fragment(this);  //make a copy
    this->consensus = Cons;
    Cons->sortByMz();

    for(unsigned int i=0; i<brothers.size(); i++) {
        Fragment* brother = brothers[i];

        vector<int>ranks=locatePositions(brother,Cons,productAmuToll);	//location 

        for(unsigned int j=0; j<ranks.size(); j++ ) {
            int   posA = ranks[j];	
            float mzB = brother->mzs[j];
            float intB = brother->intensity_array[j];
            if (posA >= 0)  {
                Cons->intensity_array[ posA ] += intB;
                Cons->obscount[ posA ] += 1;
            } else if ( posA == -1 ) {
                Cons->mzs.push_back(mzB);
                Cons->intensity_array.push_back(intB);
                Cons->obscount.push_back(1);
            }
        }
        Cons->sortByMz();
        //cerr << "cons" << i  << " "; Cons->printFragment(productAmuToll);
    }

    //average values 
    int N = 1+brothers.size();
    for(unsigned int i=0; i<Cons->intensity_array.size(); i++) { Cons->intensity_array[i] /= N; }
    Cons->sortByIntensity();
    float maxValue = Cons->intensity_array[0];

    for(unsigned int i=0; i<Cons->intensity_array.size(); i++) { Cons->intensity_array[i] = Cons->intensity_array[i]/maxValue*10000; }
    this->consensus = Cons; 

    //cerr << "buildConsensus()" << endl; 
    //Cons->printFragment(productAmuToll);
    //cerr << endl << endl << endl;
}


vector<unsigned int> Fragment::intensityOrderDesc() {
    unsigned int nobs = intensity_array.size();
    vector<pair<float,int> > _pairsarray(nobs);
    vector<unsigned int>position(nobs);
    for(unsigned int pos=0; pos < nobs; pos++ ) {
        _pairsarray[pos] = make_pair(intensity_array[pos],pos);
    }

    //reverse sort first key [ ie intensity ]
    sort(_pairsarray.rbegin(), _pairsarray.rend());

    //return positions in order from highest to lowest intenisty
    for(unsigned int i=0; i < _pairsarray.size(); i++) { position[i] = _pairsarray[i].second; }
    return position;
}


vector<int> Fragment::mzOrderInc() {
    unsigned int nobs = mzs.size();
    vector<pair<float,int> > _pairsarray(nobs);
    vector<int>position(nobs);
    for(unsigned int pos=0; pos < nobs; pos++ ) {
        _pairsarray[pos] = make_pair(mzs[pos],pos);
    }

    //forward sort first key
    sort(_pairsarray.begin(), _pairsarray.end());

    //return positions in order from highest to lowest intenisty
    for(unsigned int i=0; i < _pairsarray.size(); i++) { position[i] = _pairsarray[i].second; }
    return position;
}


void Fragment::sortByIntensity() { 
    vector<unsigned int>order = intensityOrderDesc();
    vector<float> a(mzs.size());
    vector<float> b(intensity_array.size());
    vector<int> c(obscount.size());
    map<int,string>d;
    for(unsigned int i=0; i<order.size(); i++) {
        b[i] = intensity_array[order[i]];
        a[i] = mzs[order[i]];
        if(order[i] < obscount.size()) c[i] = obscount[order[i]];
        if(annotations.count(order[i])>0) d[i] = annotations[order[i]];

    };
    mzs = a;
    intensity_array = b;
    obscount = c;
    annotations=d;
    sortedBy = SortType::Intensity;
}	

void Fragment::sortByMz() { 
    vector<int>order = mzOrderInc();
    vector<float> a(mzs.size());
    vector<float> b(intensity_array.size());
    vector<int> c(obscount.size());
    map<int,string>d;

    for(unsigned int i=0; i<order.size(); i++) {
        b[i] = intensity_array[order[i]];
        a[i] = mzs[order[i]];
        c[i] = obscount[order[i]];
        if (annotations.count(order[i])>0) d[i] = annotations[order[i]];

    };

    mzs = a;
    intensity_array = b;
    obscount = c;
    annotations=d;
    sortedBy = SortType::Mz;
}

void Fragment::buildConsensusAvg() { 
    map<float,double> mz_intensity_map;
    map<float,double> mz_bin_map;
    map<float,int> mz_count;

    vector<Fragment*> fragmentList = brothers;
    fragmentList.push_back(this);

    for(unsigned int i=0; i<fragmentList.size(); i++) {
        Fragment* brother = fragmentList[i];
        for(unsigned int j=0; j < brother->mzs.size(); j++) {
            float bin = (round(brother->mzs[j]+0.5)*10)/10;
            mz_intensity_map[bin] += ((double) brother->intensity_array[j]);
            mz_bin_map[bin] += ((double)(brother->intensity_array[j])*(brother->mzs[j]));
            mz_count[bin]++;
        }
    }
    map<float,double>::iterator itr;
    for(itr = mz_intensity_map.begin(); itr != mz_intensity_map.end(); ++itr ) {
        float bin = (*itr).first;
        double totalIntensity=(*itr).second;
        double avgMz =  mz_bin_map[bin] / totalIntensity;
        cerr << "\t" << setprecision(3) << avgMz << " " << totalIntensity/mz_count[bin] << endl;
        //avgScan->mz.push_back((float)avgMz);
        //avgScan->intensity.push_back((float) totalIntensity / mz_count[bin]);
    }
}


double Fragment::spearmanRankCorrelation(const vector<int>& X) {
    double d2=0; 
    int N = X.size();
    int n=0;
    for(int i=0; i<N;i++ ) {	
        if (X[i] == -1 ) { //mising values set to average distance
            d2 += 2*i;
            n++;
        } else  {
            n++;
            d2 += (i-X[i])*(i-X[i]);
            //cerr << "n=" << n << " cor=" << d2 << endl;
        }
    }
    //double p = 1.00-(6.0*d2)/(n*((n*n)-1));
    return 1.00-(6.0*d2)/(n*((n*n)-1));
}

double Fragment::fractionMatched(const vector<int>& X) {
    if (X.size() == 0) return 0;
    int matchCount=0;
    for(unsigned int i=0; i<X.size();i++ ) {	if (X[i] != -1 ) matchCount++; }
    //if (verbose) { cerr << "\t\t fractionMatched:" << matchCount << endl; }
    return ((double)matchCount / X.size());
}

double Fragment::mzErr(const vector<int>& X, Fragment* other) {
    if (X.size() == 0) return 0;
    double ERR=1000;
    for(unsigned int i=0; i<X.size(); i++ ) if (X[i] != -1) ERR = POW2( mzs[i] - other->mzs[ X[i] ]);
    return sqrt(ERR);
}

double Fragment::totalIntensity() {
    double TIC=0;
    for(unsigned int i=0; i<this->nobs(); i++) TIC += this->intensity_array[i];
    return TIC;
}

double Fragment::dotProduct(const vector<int>& X, Fragment* other) {
    if (X.size() == 0) return 0;
    double dotP=0;
    double thisIntensity = this->totalIntensity();
    double otherIntensity = other->totalIntensity();

    for(unsigned int i=0; i<X.size(); i++ )  {
        if (X[i] != -1)  {
            dotP += (this->intensity_array[i])/thisIntensity * (other->intensity_array[X[i]])/otherIntensity;
        }
    }
    return dotP;
}

double Fragment::ticMatched(const vector<int>& X) {
    if (X.size() == 0) return 0;
    double TIC = this->totalIntensity();
    double matchedTIC=0;
    for(unsigned int i=0; i<this->nobs(); i++) TIC += this->intensity_array[i];
    for(unsigned int i=0; i<X.size(); i++ ) if (X[i] != -1) matchedTIC += this->intensity_array[i];
    /*
       for(int i=0; i<other->nobs();i++ ) TIC += other->intensity_array[i]; 
       for(int i=0; i<X.size();     i++ ) if (X[i] != -1) matchedTIC += other->intensity_array[ X[i] ];
       */
    if (TIC>0) return matchedTIC/TIC; else return 0;
}

bool Fragment::hasMz(double mzValue, double amuTolr) {
    for(unsigned int i=0; i < nobs(); i++)  if (abs(mzs[i]-mzValue) < amuTolr) return true;
    return false;
}

bool Fragment::hasNLS(double NLS, double amuTolr) {
    double mzValue = precursorMz-NLS;
    for(unsigned int i=0; i < nobs(); i++)  if (abs(mzs[i]-mzValue) < amuTolr) return true;
    return false;
}

void Fragment::addNeutralLosses() {
    int N = mzs.size();
    for(int i=0; i < N; i++) {
        float nLMass = mzs[i]-precursorMz;
        float nLIntensity = intensity_array[i];
        if(nLMass < -1) {
            mzs.push_back(nLMass);
            intensity_array.push_back(nLIntensity);
            obscount.push_back(1);
        }
    }
    sortByIntensity();
}

