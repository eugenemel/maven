#include <fstream>
#include <vector>
#include <map>
#include <ctime>
#include <limits.h>
#include <sstream>

#include "mzSample.h"
#include "parallelMassSlicer.h"
#include "../maven/projectDB.h"

#include "../maven/classifierNeuralNet.h"
#include "../maven/database.h"
#include "./Fragment.h"
#include "pugixml.hpp"
#include <sys/time.h>

#include <QtCore>
#include <QCoreApplication>
#include <QPixmap>

#include <stdio.h>
#include <stdlib.h>

//fields
Database DB;
static string mzkitchenMspFile = "";
static string mzrollDBFile = "";

//functions
void processCLIArguments(int argc, char* argv[]);
void mzkitchenSearch();
PeakGroup* getPeakGroupFromDB(int groupId);
void assignBestMetaboliteToGroup(PeakGroup* g, vector<CompoundIon>& compounds, shared_ptr<MzkitchenMetaboliteSearchParameters> params,  bool debug);

int main(int argc, char* argv[]) {
    processCLIArguments(argc, argv);
    mzkitchenSearch();
    cout << "Test Passed" << endl;
}

void processCLIArguments(int argc, char* argv[]){
    for (int i = 1; i < argc ; i++) {
        if (strcmp(argv[i], "--mzkitchenMspFile") == 0) {
            mzkitchenMspFile = argv[i+1];
        } else if (strcmp(argv[i], "--mzrollDBFile") == 0) {
            mzrollDBFile = argv[i+1];
        }
    }
}

void mzkitchenSearch() {
    cout << "Performing mzkitchen msp search on identified peak groups." << endl;
    cout << setprecision(10);

    vector<Compound*> mzkitchenCompounds = DB.loadNISTLibrary(mzkitchenMspFile.c_str());

    sort(mzkitchenCompounds.begin(), mzkitchenCompounds.end(), [](const Compound* lhs, const Compound* rhs){
        return lhs->precursorMz < rhs->precursorMz;
    });

    cout << "MSP spectral library \'" << mzkitchenMspFile << "\' contains " << mzkitchenCompounds.size() << " compounds." << endl;

//    for (auto compound : mzkitchenCompounds) {
//        cout << compound->name << ": mz=" << compound->precursorMz << endl;
//    }

    //Issue 633: Failure to ID bug
    PeakGroup *tmaoPG = getPeakGroupFromDB(50);

    cout << "TMAO: mz=" << tmaoPG->meanMz << ", rt=" << tmaoPG->medianRt() << endl;

    double mzSumDouble = 0.0;
    float mzSum = 0;
    int nonZeroCount = 0;
    for(unsigned int i=0; i< tmaoPG->peaks.size(); i++) {
         if(tmaoPG->peaks[i].pos != 0) {
             mzSum += tmaoPG->peaks[i].baseMz;
             mzSumDouble += static_cast<double>(tmaoPG->peaks[i].baseMz);
             nonZeroCount++;
         } else {
             cout << "0 @ i=" << i << ": " << tmaoPG->peaks[i].baseMz << endl;
         }
    }
    float meanMz = mzSum/static_cast<float>(nonZeroCount);

    cout << "mzSum=" << mzSum << endl;
    cout << "mzSumDouble=" << mzSumDouble << endl;
    cout << "nonZeroCount=" << nonZeroCount << endl;
    cout << "meanMz recalculated=" << meanMz << endl;

    shared_ptr<MzkitchenMetaboliteSearchParameters> params = MzkitchenMetaboliteSearchParameters::decode("searchVersion=mzkitchen22_metabolite_search;scanFilterMinFracIntensity=0.010000;scanFilterMinSNRatio=1.000000;scanFilterMaxNumberOfFragments=1024;scanFilterBaseLinePercentile=5;scanFilterIsRetainFragmentsAbovePrecursorMz=0;scanFilterPrecursorPurityPpm=10.000000;scanFilterMinIntensity=0.000000;scanFilterMs1MinRt=-1.000000;scanFilterMs1MaxRt=-1.000000;scanFilterMs2MinRt=-1.000000;scanFilterMs2MaxRt=-1.000000;consensusIsIntensityAvgByObserved=0;consensusIsNormalizeTo10K=1;consensusIntensityAgglomerationType=MEAN;consensusMs1PpmTolr=10.000000;consensusMinNumMs1Scans=0;consensusMinFractionMs1Scans=0.000000;consensusPpmTolr=10.000000;consensusMinNumMs2Scans=0;consensusMinFractionMs2Scans=0.000000;ms1PpmTolr=10.000000;ms2MinNumMatches=0;ms2MinNumDiagnosticMatches=0;ms2MinNumUniqueMatches=0;ms2PpmTolr=20.000000;ms2MinIntensity=0.000000;ms2IsRequirePrecursorMatch=0;IDisRequireMatchingAdduct=0;mspFilePath=;rtIsRequireRtMatch=0;rtMatchTolerance=0.500000;matchingPolicy=SINGLE_TOP_HIT;isComputeAllFragScores=1;peakSmoothingWindow=10;peakRtBoundsMaxIntensityFraction=1.000000;peakRtBoundsSlopeThreshold=0.010000;peakBaselineSmoothingWindow=10;peakBaselineDropTopX=80;peakIsComputeBounds=1;peakIsReassignPosToUnsmoothedMax=0;eicBaselineEstimationType=DROP_TOP_X;mergedSmoothingWindow=10;mergedPeakRtBoundsMaxIntensityFraction=-1.000000;mergedPeakRtBoundsSlopeThreshold=0.010000;mergedSmoothedMaxToBoundsMinRatio=2.000000;mergedSmoothedMaxToBoundsIntensityPolicy=MINIMUM;mergedBaselineSmoothingWindow=10;mergedBaselineDropTopX=80;mergedIsComputeBounds=1;groupMaxRtDiff=0.250000;groupMergeOverlap=0.800000;filterMinGoodGroupCount=1;filterMinQuality=0.500000;filterMinNoNoiseObs=5;filterMinSignalBaselineRatio=1.100000;filterMinGroupIntensity=10000.000000;filterMinPrecursorCharge=0;");

    auto compoundIons = vector<CompoundIon>(mzkitchenCompounds.size());
    for (unsigned int i = 0; i <  mzkitchenCompounds.size(); i++) {
        compoundIons[i] = CompoundIon(mzkitchenCompounds[i]);
    }

    assignBestMetaboliteToGroup(tmaoPG, compoundIons, params, true);
}

#include "../maven/projectDB.h"

PeakGroup* getPeakGroupFromDB(int groupId) {
    ProjectDB *pdb = new ProjectDB(QString(mzrollDBFile.c_str()));

    PeakGroup *pg = new PeakGroup();
    pg->peaks = pdb->getPeaks(groupId);
    pg->groupStatistics();

    pdb->closeDatabaseConnection();

    return(pg);
}

void assignBestMetaboliteToGroup(
        PeakGroup* g,
        vector<CompoundIon>& compounds,
        shared_ptr<MzkitchenMetaboliteSearchParameters> params,
        bool debug){

    float minMz = g->meanMz - (g->meanMz*params->ms1PpmTolr/1000000);
    float maxMz = g->meanMz + (g->meanMz*params->ms1PpmTolr/1000000);

    if (debug) {
        cout << "[minMz, maxMz] = [" << minMz << ", " << maxMz << "]" << endl;
    }

    auto lb = lower_bound(compounds.begin(), compounds.end(), minMz, [](const CompoundIon& lhs, const float& rhs){
        return lhs.precursorMz < rhs;
    });

    if (g->fragmentationPattern.mzs.empty()) {
        g->computeFragPattern(params.get());
    }

    vector<pair<CompoundIon, FragmentationMatchScore>> scores{};

    for (long pos = lb - compounds.begin(); pos < static_cast<long>(compounds.size()); pos++){

        if (debug) cout << "pos=" << pos << endl;

        CompoundIon ion = compounds[static_cast<unsigned long>(pos)];
        Compound* compound = ion.compound;
        Adduct *adduct = ion.adduct;

        if (!compound) continue;

        if (debug) cout << "compound: " << compound->name << endl;

        bool isMatchingAdduct = adduct && compound->adductString == adduct->name;
        if (params->IDisRequireMatchingAdduct && !isMatchingAdduct) {
            continue;
        }

        float precMz = ion.precursorMz;

        if (debug) {

            cout << "(minMz=" << minMz << ", maxMz=" << maxMz << "):\n";

            if (pos >= 2){
                cout << "compounds[" << (pos-2) << "]: " << compounds[pos-2].precursorMz << "\n";
                cout << "compounds[" << (pos-1) << "]: " << compounds[pos-1].precursorMz << "\n";
            }

            cout << "compounds[" << (pos) << "]: " << precMz << " <--> " << compound->id << "\n";

            if (pos <= static_cast<long>(compounds.size()-2)) {
                cout << "compounds[" << (pos+1) << "]: " << compounds[pos+1].precursorMz << "\n";
                cout << "compounds[" << (pos+2) << "]: " << compounds[pos+1].precursorMz << "\n";
            }

            cout << "\n";

        }

        //stop searching when the maxMz has been exceeded.
        if (precMz > maxMz) {
            break;
        }

        Fragment library;
        library.precursorMz = static_cast<double>(precMz);
        library.mzs = compound->fragment_mzs;
        library.intensity_array = compound->fragment_intensity;
        library.fragment_labels = compound->fragment_labels;

        //skip entries when the RT is required, and out of range
        float rtDiff = abs(g->medianRt() - compound->expectedRt);
        if (params->rtIsRequireRtMatch & (rtDiff > params->rtMatchTolerance)) {
            continue;
        }

        FragmentationMatchScore s = library.scoreMatch(&(g->fragmentationPattern), params->ms2PpmTolr);

        double normCosineScore = Fragment::normCosineScore(&library, &(g->fragmentationPattern), s.ranks);
        s.dotProduct = normCosineScore;

        //debugging
        if (debug) {
            cout << "Candidate Score: " << compound->id << ":\n";
            cout << "numMatches= " << s.numMatches
                 << ", hyperGeometricScore= " << s.hypergeomScore
                 << ", cosineScore= " << s.dotProduct
                 << " VS params->ms2MinNumMatches = " << params->ms2MinNumMatches
                 << " " << (s.numMatches >= params->ms2MinNumMatches ? "yes" : "no")
                 << "\n\n\n";
        }

        if (s.numMatches < params->ms2MinNumMatches) continue;


        scores.push_back(make_pair(ion, s));
    }

    //Issue 559: print empty groups
    if (debug) {
        cout << g->meanMz << "@" << g->medianRt() << " #ms2s=" << g->ms2EventCount << " :" << endl;
    }

    if (scores.empty()) return;

    g->compounds = scores;

    float maxScore = -1.0f;
    pair<CompoundIon, FragmentationMatchScore> bestPair;
    for (auto score : scores) {
        if (score.second.dotProduct > maxScore) {
            bestPair = score;
            maxScore = score.second.dotProduct;
        }
    }

    if (maxScore > -1.0f) {
        g->compound = bestPair.first.compound;
        if (bestPair.first.adduct) g->adduct = bestPair.first.adduct;
        g->fragMatchScore = bestPair.second;
        g->fragMatchScore.mergedScore = bestPair.second.dotProduct;
    }

    //Issue 546: debugging
    if (debug) {
        for (auto pair : g->compounds) {
            cout << "\t" << pair.first.compound->name << " "
                 << pair.second.numMatches << " "
                 << pair.second.fractionMatched << " "
                 << pair.second.dotProduct  << " "
                 << pair.second.hypergeomScore << " "
                 << endl;
        }
    }

}
