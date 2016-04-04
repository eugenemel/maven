#ifndef FRAGMENT_H
#define FRAGMENT_H

#include<iostream>
#include<vector>
#include<map>
#include<math.h>

class Compound;
class PeakGroup;
class Scan;

using namespace std;

struct FragmentationMatchScore {

    double fractionMatched;
    double spearmanRankCorrelation;
    double ticMatched;
    double numMatches;
    double ppmError;
    double mzFragError;
    double mergedScore;
    double dotProduct;
    double weightedDotProduct;
    double hypergeomScore;

    static vector<string> getScoringAlgorithmNames() {
        vector<string> names;
        names.push_back("HyperGeomScore");
        names.push_back("DotProduct");
        names.push_back("WeightedDotProduct");
        names.push_back("SpearmanRank");
        names.push_back("TICMatched");
        return names;
    }


    double getScoreByName(string scoringAlgorithm) {
        if (scoringAlgorithm == "HyperGeomScore" )         return hypergeomScore;
        else if (scoringAlgorithm == "DotProduct")         return dotProduct;
        else if (scoringAlgorithm == "SpearmanRank")       return spearmanRankCorrelation;
        else if (scoringAlgorithm == "TICMatched")         return ticMatched;
        else if (scoringAlgorithm == "WeightedDotProduct") return weightedDotProduct;
        else return hypergeomScore;

    }

    FragmentationMatchScore() {
        fractionMatched=0;
        spearmanRankCorrelation=0;
        ticMatched=0;
        numMatches=0;
        ppmError=1000;
        mzFragError=1000;
        mergedScore=0;
        dotProduct=0;
        weightedDotProduct=0;
        hypergeomScore=0;
    }

    FragmentationMatchScore& operator=(const FragmentationMatchScore& b) {
        fractionMatched= b.fractionMatched;
        spearmanRankCorrelation=b.spearmanRankCorrelation;
        ticMatched=b.ticMatched;
        numMatches=b.numMatches;
        ppmError=b.ppmError;
        mzFragError=b.mzFragError;
        mergedScore=b.mergedScore;
        dotProduct=b.dotProduct;
        weightedDotProduct=b.weightedDotProduct;
        hypergeomScore=b.hypergeomScore;
        return *this;
    }

};

class Fragment {
    public: 
        enum SortType {None=0,  Mz=1, Intensity=2 };

        double precursorMz;				//parent
        int polarity;					//scan polarity 	+1 or -1
        vector<float> mzs;				//mz values
        vector<float> intensity_array;		//intensity_array
        vector<Fragment*> brothers;		//pointers to similar fragments 
        string sampleName;				//name of Sample
        int scanNum;					//scan Number
        float rt;						//retention time of parent scan
        float collisionEnergy;
        int precursorCharge;
        bool isDecoy;
        SortType sortedBy;

        PeakGroup* group;

        //consensus pattern buuld from brothers generated on buildConsensus call.
        Fragment* consensus; //consensus pattern build on brothers
        vector<int>obscount; // vector size =  mzs vector size, with counts of number of times mz was observed
        map<int,string> annotations; //mz value annotations.. assume that values are sorted by mz

        inline unsigned int nobs() { return mzs.size(); }
        inline void addFragment(Fragment* b) { brothers.push_back(b); }

        //empty constructor
        Fragment();

        //copy constructor
        Fragment( Fragment* other);

        //build fragment based on MS2 scan
        Fragment(Scan* scan, float minFractionalIntensity, float minSigNoiseRatio,unsigned int maxFragmentSize);

        //delete
        ~Fragment();

        void appendBrothers(Fragment* other);
        void printMzList();
        int  findClosestHighestIntensityPos(float _mz, float tolr);
        void printFragment(float productPpmToll,unsigned int limitTopX);
        void printInclusionList(bool printHeader, ostream& outstream, string COMPOUNDNAME);
        void printConsensusMS2(ostream& outstream, double minConsensusFraction);
        void printConsensusMGF(ostream& outstream, double minConsensusFraction);
        void printConsensusNIST(ostream& outstream, double minConsensusFraction, float productPpmToll, Compound* compound);
        FragmentationMatchScore scoreMatch(Fragment* other, float productPpmToll);

        double compareToFragment(Fragment* other, float productPPMToll);
        static vector<int> compareRanks(Fragment* a, Fragment* b, float productPpmTolr);
        static vector<int> locatePositions( Fragment* a, Fragment* b, float productPpmToll);


        void buildConsensus(float productPpmTolr);
        vector<unsigned int> intensityOrderDesc();
        vector<int> mzOrderInc();


        void sortByIntensity();
        void sortByMz();
        void buildConsensusAvg();


        double spearmanRankCorrelation(const vector<int>& X);
        double fractionMatched(const vector<int>& X);
        double mzErr(const vector<int>& X, Fragment* other);

        double totalIntensity();
        double dotProduct(const vector<int>& X, Fragment* other);
        double ticMatched(const vector<int>& X);
        double mzWeightedDotProduct(const vector<int>& X, Fragment* other);
        bool hasMz(float mzValue, float ppmTolr);
        bool hasNLS(float NLS, float ppmTolr);
        void addNeutralLosses();

        double logNchooseK(int N,int k);
        double SHP(int matched, int len1, int len2, int N);

        static bool compPrecursorMz(const Fragment* a, const Fragment* b) { return a->precursorMz<b->precursorMz; }
        bool operator<(const Fragment* b) const{ return this->precursorMz < b->precursorMz; }
        bool operator==(const Fragment* b) const{ return fabs(this->precursorMz-b->precursorMz)<0.001; }
};


#endif
