#include <fstream>
#include <vector>
#include <map>
#include <ctime>
#include <limits.h>
#include <sstream>

#include "mzSample.h"
#include "parallelMassSlicer.h"
#include "../maven/projectDB.h"
#include "options.h"

#include "../maven/classifierNeuralNet.h"
#include "../maven/database.h"
#include "./Fragment.h"
#include "pugixml.hpp"
#include <sys/time.h>

#include <QtCore>
#include <QCoreApplication>
#include <QPixmap>

#include <strnatcmp.h>

#include <stdio.h>
#include <stdlib.h>

using namespace std;

#ifdef OMP_PARALLEL
#include "omp.h"
#define getTime() omp_get_wtime()
#else
#define getTime() get_wall_time()
#define omp_get_thread_num()  1
#endif

//variables
int majorVersion = 1;
int minorVersion = 1;
int modVersion =   4;

Database DB;
ClassifierNeuralNet clsf;
vector <mzSample*> samples;
vector <Compound*> compoundsDB;
vector<PeakGroup> allgroups;
MassCalculator mcalc;

time_t startTime, curTime, stopTime;

vector<string>filenames;
string alignmentFile;
string projectFile;
ProjectDB* project = nullptr;

bool alignSamplesFlag = false;
bool reduceGroupsFlag = true;
bool matchRtFlag = true;
bool writeReportFlag = true;
bool saveJsonEIC = false;
bool mustHaveMS2 = false;
bool writeConsensusMS2 = false;
bool samplesC13Labeled = true;
bool saveSqlLiteProject = true;
bool searchAdductsFlag = true;
bool saveScanData=true;
bool WRITE_CONSENSUS_MAPPED_COMPOUNDS_ONLY = true;
string algorithmType = "E"; //processMassSlices()
string groupingAlgorithmType = "D"; //Issue 692: incorporate reduceGroups()
bool isRunClustering = false;
bool isHIHRPAlignment = false;

string csvFileFieldSeperator = ",";
PeakGroup::QType quantitationType = PeakGroup::AreaTop;

string ligandDbFilename;
string methodsFolder = "../maven_core/bin/methods";
string fragmentsDbFilename = "FRAGMENTS.csv";
string adductDbFilename =  "ADDUCTS.csv";
string clsfModelFilename = "default.model";
string outputdir = "reports" + string(DIR_SEPARATOR_STR);
string nameSuffix = "peakdetector";
string anchorPointsFile;

float standardsAlignment_maxRtWindow = 5;
float standardsAlignment_precursorPPM = 20;
float standardsAlignment_minPeakIntensity = 1e4;

int  ionizationMode = -1;

//mass slice detection
int   rtStepSize = 10;
float precursorPPM = 5.0f;
float avgScanTime = 0.2f;

//peak detection
int eic_smoothingWindow = 5;
int baseline_smoothingWindow = 5;
int baseline_dropTopX = 60;
float peakRtBoundsSlopeThreshold = -1.0f;
float peakRtBoundsMaxIntensityFraction = 1.0f;

//eic
EICBaselineEstimationType eicBaselineEstimationType = EICBaselineEstimationType::DROP_TOP_X;

//merged EIC
float mergedPeakRtBoundsMaxIntensityFraction = -1.0f;
float mergedPeakRtBoundsSlopeThreshold = -1.0f;
float mergedSmoothedMaxToBoundsMinRatio = -1.0f;
SmoothedMaxToBoundsIntensityPolicy mergedSmoothedMaxToBoundsIntensityPolicy = SmoothedMaxToBoundsIntensityPolicy::MEDIAN;

//computed properties
PeakGroupBackgroundType groupBackgroundType = PeakGroupBackgroundType::NONE;

//peak grouping across samples
float grouping_maxRtWindow = 0.25f;
float mergeOverlap = 0.8f;

//peak filtering criteria
int minGoodGroupCount = 1;
float minSignalBlankRatio = 0.0f;
int minNoNoiseObs = 5;
float minSignalBaseLineRatio = 5;
float minGroupIntensity = 1000;
float minQuality = 0.5;
int minPrecursorCharge = 0;

//msp searches
bool isMzkitchenSearch = false;
static string mzkitchenSearchType = "";
static string mzkitchenMspFile = "";
static string mzkitchenSearchParameters = "";

// Issue 751: Isotopes, Need adducts for proper isotope extraction
static bool isExtractIsotopes = false;
static IsotopeParameters isotopeParameters;
static map<string, Adduct*> loadedAdducts{};

//translation list seaches
bool isQQQSearch = false;
shared_ptr<QQQSearchParameters> QQQparams = shared_ptr<QQQSearchParameters>(new QQQSearchParameters());
bool isQQQCSVExport = false;

// Issue 798: use provided anchor point RT as reference RT (instead of random sample)
bool rtAlignmentAnchorPointReference = false;

// Issue 815: new slice protocol
// If this string is not empty, use the compound library to define slices.
// this is orthogonal to the search type
string compoundLibraryFile = "";

// Issue 815: New search type with two effect:
// (1) During peak group generation, filter out peak groups based on isotope patterns
// (2) After all peak groups have been generated, label
bool isPeptideStabilitySearch = false;
string peptideStabilitySearchParamsStr = "";

//parameters
shared_ptr<PeakPickingAndGroupingParameters> peakPickingAndGroupingParameters = shared_ptr<PeakPickingAndGroupingParameters>(new PeakPickingAndGroupingParameters());
shared_ptr<MzkitchenMetaboliteSearchParameters> metaboliteSearchParams = shared_ptr<MzkitchenMetaboliteSearchParameters>(new MzkitchenMetaboliteSearchParameters());
shared_ptr<LCLipidSearchParameters> lipidSearchParams = shared_ptr<LCLipidSearchParameters>(new LCLipidSearchParameters());
shared_ptr<PeptideStabilitySearchParameters> peptideStabilitySearchParams = shared_ptr<PeptideStabilitySearchParameters>(new PeptideStabilitySearchParameters());

static map<QString, QString> searchTableData{};

static string sampleIdMappingFile;
static map<string, pair<int, bool>> sampleIdMapping{};
static map<mzSample*, vector<pair<float, float>>> sampleToUpdatedRts{};

/**
 * @brief minSmoothedPeakIntensity
 *
 * Smoothed peak intensity must be greater than or equal to this value,
 * otherwise, the peak will not be retained.
 */
float minSmoothedPeakIntensity = 1000; //USED ONLY FOR EIC::groupPeaksB

//size of retention time window
float rtWindow = 2;
int  eicMaxGroups = 100;

//MS2 matching
float minFragmentMatchScore = 2;
float productPpmTolr = 20;
string scoringScheme = "hypergeomScore"; // ticMatch | spearmanRank

void processOptions(int argc, char* argv[]);
void fillOutPeakPickingAndGroupingParameters();

void loadSamples(vector<string>&filenames);
void setSampleIdFromFile();
void printSettings();
void processSlices(vector<mzSlice*>&slices, string groupingAlgorithmType, string setName, bool writeReportFlag);
void processCompounds(vector<Compound*> set, string setName);
void reduceGroups();
void writeReport(string setName);
void writeQQQReport(string filename);
void writePeptideStabilityReport(string filename);
void writeCSVReport(string filename);
void writeConsensusMS2Spectra(string filename);
void classConsensusMS2Spectra(string filename);
void writeGroupInfoCSV(PeakGroup* group,  ofstream& groupReport);
void loadSpectralLibraries(string methodsFolder);
void makeImages(ParallelMassSlicer* massSlices, vector<mzSample*> sampleSubset, float rtAccur, float mzAccur);
void loadProject(string projectFile);
void anchorPointsBasedAlignment();

//special searches
void mzkitchenSearch();
bool isSpecialSearch();
void traverseAndAdd(PeakGroup& group, set<Compound*>& compoundSet);

vector<string> removeDuplicateFilenames(vector<string>& filenames);
vector<mzSlice*> processMassSlices(int limit = -1, string algorithmType = "D", string setName = "peakdetector");
void alignSamples();
void alignSamplesHIHRP();

int getChargeStateFromMS1(PeakGroup* grp);
bool isC12Parent(Scan* scan, float monoIsotopeMz);
bool isMonoisotope(PeakGroup* grp);

vector<mzSlice*> getSrmSlices();

//Issue 815: new slice generation approach
vector <mzSlice*> compoundLibrarySlices(string compoundLibraryFile, bool debug=false);

void matchFragmentation();

double get_wall_time();
double get_cpu_time();

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    double programStartTime = getTime();

    //read command line options
    processOptions(argc, argv);

    cout << "peakdetector ver=" << majorVersion << "." << minorVersion << "." << modVersion << endl;

    //load classification model
    cout << "Loading classification model: " << methodsFolder + "/" + clsfModelFilename << endl;
    clsfModelFilename = methodsFolder + "/" + clsfModelFilename;

    if (fileExists(clsfModelFilename)) {
        clsf.loadModel(clsfModelFilename);
    } else {
        cout << "Can't find peak classification model:" << clsfModelFilename << endl;
        exit(-1);
    }

    loadSpectralLibraries(methodsFolder);

    if (DB.compoundsDB.size() and searchAdductsFlag) {
        //processCompounds(compoundsDB, "compounds");
        DB.adductsDB = DB.loadAdducts(methodsFolder + "/" + adductDbFilename);
    }

    //load files
    double startLoadingTime = getTime();

    //if mzrollDB file set. reload samples from project
    if (!projectFile.empty()) {
        loadProject(projectFile);
    } else {
        loadSamples(filenames);
    }

    printf("Execution time (Sample loading) : %f seconds \n", getTime() - startLoadingTime);

    if (samples.size() == 0 ) {
        cout << "Exiting .. nothing to process " << endl;
        exit(1);
    }

    //ionization
    if (samples.size() > 0 ) {
        ionizationMode = samples[0]->getPolarity();
    }

    printSettings();

    //align samples
    if (alignSamplesFlag) {

        //standards-based RT alignment
        if (!anchorPointsFile.empty()) {
            cout << "Aligning samples using provided (m/z, RT) anchor points file:" << "\t" << anchorPointsFile << endl;
            anchorPointsBasedAlignment();
        } else if (!alignmentFile.empty()) {
            cout << "Aligning samples using provided alignment file:" << "\t" << alignmentFile << endl;
            Aligner aligner;
            aligner.setSamples(samples);
            sampleToUpdatedRts = aligner.loadAlignmentFile(alignmentFile); 
            
            //debugging
            qDebug() << "Using provided RT Alignment:";
            for (auto it = sampleToUpdatedRts.begin(); it != sampleToUpdatedRts.end(); ++it) {
            	mzSample* sample = it->first;
            	vector<pair<float, float>> rtVals = it->second;
            	
            	for (auto p : rtVals) {
            		qDebug() << sample->sampleName.c_str() << p.first << p.second;
            	}
            }
            aligner.doSegmentedAligment();
        } else if (isHIHRPAlignment) {
            cout << "Aligning samples using HIHRP (High Intensity High Reproducibility Peaks) algorithm." << endl;
            alignSamplesHIHRP();
        } else {
            cout << "Aligning samples without any prior knowledge."  << endl;
            alignSamples();
        }
    }

    vector<mzSlice*> slices{};

    if (isQQQSearch) {

        vector<string> missingTransitions{};

        cout << "Transition List File: \"" << QQQparams->transitionListFilePath << "\"" << endl;

        vector<Compound*> compounds = Database::loadCompoundCSVFile(
                    QString(QQQparams->transitionListFilePath.c_str()),
                    false // debug
                    );

//        //Issue 565: debugging
//        for (auto compound : compounds) {
//            bool isInternalStandard = compound->metaDataMap.at(QQQProcessor::getTransitionIsInternalStandardStringKey()) == "TRUE";
//            string preferredQuantType = compound->metaDataMap.at(QQQProcessor::getTransitionPreferredQuantTypeStringKey());
//            string ionType = compound->metaDataMap.at(QQQProcessor::getTransitionIonTypeFilterStringKey());
//            cout << compound->name
//                 << ": IS=" << (isInternalStandard ? "yes " : "no ")
//                 << " ionType=" << ionType
//                 << " preferredQuantType="
//                 << preferredQuantType
//                 << " rt=" << compound->expectedRt
//                 << endl;
//        }
//        exit(0);

        vector<Adduct*> adducts{};

        vector<SRMTransition*> transitions = QQQProcessor::getSRMTransitions(
                    samples,
                    QQQparams,
                    compounds,
                    adducts,
                    false // debug
                    );

        // Every peak group in an mzSlice is associated with every compound in the mzSlice->compoundVector.
        // If there are no compounds associated with an mzSlice, the peak group will not be rendered correctly in
        // MAVEN - MAVEN doesn't know how to show an mzSlice from SRM data that doesn't have a compound assocaited with it.
        //
        // For this reason, at least one compound is required to generate an mzSlice in the first place.

        slices = QQQProcessor::getMzSlices(
                    transitions,
                    true, //isRequireCompound
                    true  //debug
                    );

        cout << "Identified " << transitions.size() << " SRM transitions, generating " << slices.size() << " mz slices." << endl;

    // Issue 815: Compound-library specific search
    } else if (!compoundLibraryFile.empty() ) {
        slices = compoundLibrarySlices(compoundLibraryFile, false); // debug
    } else {
        slices = processMassSlices(-1, algorithmType, nameSuffix);
    }

    //extract EICs and form groups
    double functionStartTime = getTime();
    processSlices(slices, groupingAlgorithmType, nameSuffix, true);
    printf("Execution time (processSlices) : %f seconds \n", ((float)getTime() - (float)functionStartTime) );

    //cleanup
    delete_all(samples);
	delete_all(slices);
    samples.clear();
    allgroups.clear();

    //Issue 592: Explicitly close database as good programming practice
    if (project) {
        project->closeDatabaseConnection();
        delete(project);
        project = nullptr;
    }

    printf("Total program execution time : %f seconds \n", getTime() - programStartTime);

    return 0;
}

mzSlice getGlobalBounds(vector<mzSample*>sampleSubset) {

    mzSlice bounds;
    if (sampleSubset.size() == 0) return bounds;

    float minMz = 100;
    float maxMz = 3000;
    float minRt = 0;
    float maxRt = sampleSubset[0]->maxRt;

    for (mzSample* s : sampleSubset) {
        minMz = min(minMz, s->minMz);
        maxMz = max(maxMz, s->maxMz);
        minRt = min(minRt, s->minRt);
        maxRt = max(maxRt, s->maxRt);
    }

    maxRt = ceil(maxRt) + 1.0;
    maxMz = ceil(maxMz) + 1.0;

    bounds.mzmin = minMz;
    bounds.mzmax = maxMz;
    bounds.rtmin = minRt;
    bounds.rtmax = maxRt;

    return bounds;
}

void makeImages(ParallelMassSlicer* massSlices, vector<mzSample*> sampleSubset, float rtAccur = 0.1, float mzAccur = 0.1) {

    mzSlice GB = getGlobalBounds(sampleSubset);

    float rt_range = (GB.rtmax - GB.rtmin);
    float mz_range = (GB.mzmax - GB.mzmin);

    int imageWidth  = (int) max( rt_range / rtAccur, (float) 1.0);
    int imageHeight = (int) max( mz_range / mzAccur, (float) 1.0);


    for (mzSample* sample : sampleSubset) {

        float maxIntensity = exp(log(sample->maxIntensity) * 0.7);
        cout << "IMAGE W:" << imageWidth << " H:" << imageHeight << " " << rt_range << " " << mz_range << " " << maxIntensity << endl;

        QImage img(imageWidth, imageHeight, QImage::Format_RGB32);
        img.setText("title", sample->getSampleName().c_str());
        img.setText("author", "Eugene M");
        img.setText("comment", QString::number((double) GB.mzmin) + "-" + QString::number((double) GB.mzmax));

        //draw scan data
        for (Scan* s : sample->scans) {
            if (s->mslevel != 1) continue;
            float xcoord = (s->rt - GB.rtmin) / rt_range * imageWidth;
            for (int i = 0; i < s->nobs(); i++) {
                float ycoord = (s->mz[i] - GB.mzmin) / mz_range * imageHeight;
                float c = (s->intensity[i]) / maxIntensity * 254; if (c > 254) c = 254;
                img.setPixel(xcoord, ycoord, qRgba(0, 0, c, 0));
            }
        }

        //draw location of features
        for (mzSlice* s : massSlices->slices ) {
            float xcoord = (s->rt - GB.rtmin) / rt_range * imageWidth;
            float ycoord = (s->mz - GB.mzmin) / mz_range * imageHeight;
            img.setPixel(xcoord, ycoord, qRgba(255, 255, 255, 0));
        }

        //draw mz_ticks
        for (float y = GB.mzmin; y < GB.mzmax; y += 100.0) {
            float ycoord = (y - GB.mzmin) / mz_range * imageHeight;
            for (int x = 0; x < 10; x++)  img.setPixel(x, ycoord, qRgba(255, 255, 255, 0));
        }

        img.save(QString(sample->getSampleName().c_str()) + ".png");
    }
}

void alignSamples() {
    if (samples.size() <= 1) return;

    vector<mzSlice*> slices = processMassSlices(-1, algorithmType, nameSuffix);
    processSlices(slices, groupingAlgorithmType, "alignment", false);

    cout << "Aligning: " << allgroups.size() << endl;

    vector<PeakGroup*>agroups(allgroups.size());
    sort(allgroups.begin(), allgroups.end(), PeakGroup::compRt);

    for (unsigned int i = 0; i < allgroups.size(); i++) {
        agroups[i] = &allgroups[i];
        /*
        cout << "Alignments: " << setprecision(5)
                        << allgroups[i].meanRt << " "
                        << allgroups[i].maxIntensity << " "
                        << allgroups[i].peaks.size() << " "
                        << allgroups[i].meanMz << " "
                        << endl;
        */
    }
    Aligner aligner;
    aligner.doAlignment(agroups);
    allgroups.clear();
}

/**
 * @brief alignSamplesHIHRP() (High Intensity, High Reproducibility Peaks)
 *
 * Issue 371: A new alignment algorithm
 * this approach has some assumptions.
 * Namely, that the high intensity peaks are reproducible between samples.
 *
 * If samples are missing from an anchor point peak, interpolate the RT value based on
 * the sample numbers.  So, this approach also assumes that the sample ID corresponds
 * to the relative time at which samples are run.  If there is no term associated with
 * RT changes according to that time, this will not matter anyway, but if there is a term
 * and the samples are out of order, this approach may be off.
 *
 * This approach roughly attempts to mimic what is done by manually curating
 * peaks (as bookmarks), and transforming these bookmarks into an alignment file.
 *
 * However, the procedure for identifying good peaks to bookmark is amenable to automation, as
 * the main criteria for usage as an anchor point is intensity and reproducibility.
 *
 * Hence the name of this method, "HIHRP", stands for
 * "High Intensity, High Reproducibility Peaks"
 * RT drift can be pretty wild, though.
 *
 * This approach should prefer to be conservative, as the consequences of incorporating a bad anchor point
 * are catastrophic.  While it might be possible to survive a bad anchor point in the alignment, it is probably much
 * easier to focus efforts on excluding these anchor points from the alignment in the first place.
 */
void alignSamplesHIHRP() {

    //TODO: params
    float rtWindowSizePreAlignment = 4;

    /**
     * @brief peakIntensityThreshold
     * 5-point maximum (post-smoothing) minimum intensity value needed to consider peak valid
     */
    float peakIntensityThreshold = 1e6;

    /**
     * @brief peakReproducibilityFraction
     * Found peak in at least this proportion of samples, with intensity of at least @param peakIntensityThreshold.
     */
    float peakReproducibilityFraction = 1;

    ParallelMassSlicer massSlices;
    massSlices.setSamples(samples);
    massSlices.algorithmE(precursorPPM, rtWindowSizePreAlignment); // large RT window

    vector<mzSlice*> slices = massSlices.slices;

    int minNumSamples = round(peakReproducibilityFraction*samples.size());

    vector<PeakGroup*> groups;

    #pragma omp parallel for num_threads(16) schedule(dynamic) shared(groups)
    for (unsigned int i = 0; i < slices.size();  i++ ) {

        PeakGroup *group = new PeakGroup();
        group->groupId = i;

        for (auto& sample : samples) {
            EIC *eic = sample->getEIC(slices[i]->mzmin, slices[i]->mzmax, slices[i]->rtmin, slices[i]->rtmax, 1);
            if (eic) {
                eic->getSingleGlobalMaxPeak(eic_smoothingWindow);
                if (!eic->peaks.empty() && eic->peaks[0].peakIntensity > peakIntensityThreshold) {
                    group->addPeak(eic->peaks[0]);
                }
                delete(eic);
            }
        }

        if (group->peaks.size() >= minNumSamples) {

            group->groupStatistics();

            #pragma omp critical
            groups.push_back(group);
        }

    } //end multithreaded for

    cout << "HIHRP: Found " << groups.size() << " Anchor Point Peak Groups." << endl;

    sort(groups.begin(), groups.end(), [](const PeakGroup* lhs, const PeakGroup* rhs){
        return lhs->meanRt < rhs->meanRt;
    });

    //debugging
//    for (auto &x : groups) {
//        cout << "(" << x->meanMz << ", " << x->meanRt << "):" << endl;
//        vector<Peak> peaks = x->getPeaks();
//        sort(peaks.begin(), peaks.end(), [](const Peak& lhs, const Peak& rhs) {
//            return lhs.sample->sampleId < rhs.sample->sampleId;
//        });
//        for (auto &p : peaks) {
//            cout << p.sample->sampleName << ": " << "(" << p.peakMz << ", " << p.rt << ")" << endl;
//        }
//    }
//    exit(0);

     /**
      * Attempt 1: Use old RT alignment approach
      * Commment: This works OK for earlier points, not so great for later ones
      */
//    Aligner aligner;
//    aligner.setSamples(samples);
//    aligner.doAlignment(groups);

    /**
     * Attempt 2: Use new RT alignment approach
     * Comment:
     */

//    //Find a good representative peak group from the list of all available peak groups.
//    map<int, PeakGroup> peakGroups{};

//    for (auto &x : groups) {

//    }

    Aligner rtAligner;
    rtAligner.setSamples(samples);

    vector<AnchorPointSet> anchorPoints = rtAligner
            .groupsToAnchorPoints(
                samples,
                groups,
                eic_smoothingWindow,
                standardsAlignment_minPeakIntensity
                );

    sampleToUpdatedRts = rtAligner.anchorPointSetToUpdatedRtMap(anchorPoints, samples[0]);

    for (auto &x : sampleToUpdatedRts) {

        string sampleName = x.first->sampleName;

        float observedRtStart = 0.0f;
        float referenceRtStart = 0.0f;

        for (auto &pt : x.second) {

            AlignmentSegment *seg = new AlignmentSegment();

            float observedRt = pt.first;
            float referenceRt = pt.second;

            seg->sampleName = sampleName;

            seg->seg_start = observedRtStart;
            seg->seg_end =  observedRt;

            seg->new_start = referenceRtStart;
            seg->new_end =  referenceRt;

            //update for next iteration
            observedRtStart = observedRt;
            referenceRtStart = referenceRt;

            //debugging
            // cout << sampleName << ": " << seg->seg_start << "==>" << seg->new_start << endl;

            rtAligner.addSegment(sampleName, seg);
        }
    }
    exit(0);

    rtAligner.doSegmentedAligment();

    //clean up
    delete_all(groups);

}

void processCompounds(vector<Compound*> set, string setName) {
    cout << "processCompounds: " << setName << endl;
    if (set.size() == 0 ) return;
    vector<mzSlice*>slices;
    for (unsigned int i = 0; i < set.size();  i++ ) {
        Compound* c = set[i];
        mzSlice* slice = new mzSlice();
        slice->compound = c;

        double mass = c->getExactMass();
        if (!c->formula.empty()) {
            float formula_mass =  mcalc.computeMass(c->formula, ionizationMode);
            if (formula_mass) mass = formula_mass;
        }

        slice->mzmin = mass - precursorPPM * mass / 1e6;
        slice->mzmax = mass + precursorPPM * mass / 1e6;
        //cout << c->name << " " << slice->mzmin << " " << slice->mzmax << endl;

        slice->rtmin  = 0;
        slice->rtmax =  1e9;
        slice->ionCount = FLT_MAX;
        if ( matchRtFlag == true && c->expectedRt > 0 ) {
            slice->rtmin = c->expectedRt - rtWindow;
            slice->rtmax = c->expectedRt + rtWindow;
        }
        slices.push_back(slice);
    }

    reduceGroupsFlag = false;
    processSlices(slices, groupingAlgorithmType, setName, true);
    delete_all(slices);
}

vector<mzSlice*> getSrmSlices() {
    map<string, int>uniqSrms;
    vector<mzSlice*>slices;
    for (int i = 0; i < samples.size(); i++) {
        for (int j = 0; j < samples[i]->scans.size(); j++ ) {
            Scan* scan = samples[i]->scans[j];
            string filterLine = scan->filterLine;
            if ( uniqSrms.count(filterLine) == 0 ) {
                uniqSrms[filterLine] = 1;
                slices.push_back( new mzSlice(filterLine) );
            }
        }
    }
    cout << "getSrmSlices() " << slices.size() << endl;
    return slices;
}

vector<mzSlice*> processMassSlices(int limit, string algorithmType, string setName) {

    cout << "Process Mass Slices: algorithm=" << algorithmType << "\t" << endl;
    if (samples.size() > 0 ){
        avgScanTime = 0;
        for (auto sample : samples) {
            float singleSampleAvgFullScanTime = sample->getAverageFullScanTime();
            cout << sample->getSampleName() << ": " << singleSampleAvgFullScanTime << endl;
            avgScanTime += singleSampleAvgFullScanTime;
        }
        avgScanTime /= samples.size();
    }

    cout << "Average scan time: " << (60*avgScanTime) << " seconds" << endl;

    ParallelMassSlicer massSlices;
    massSlices.setSamples(samples);
    if (limit > 0) massSlices.setMaxSlices(limit);

    double functionAlgorithmTime = getTime();
    if (algorithmType == "A")  massSlices.algorithmA();  //SRM filter line based
    else if (algorithmType == "B")  massSlices.algorithmB(precursorPPM, minGroupIntensity, rtStepSize);	 //all peaks.. all isotopes
    else if (algorithmType == "C")  massSlices.algorithmC(precursorPPM, minGroupIntensity, rtStepSize, 20, minPrecursorCharge); //monoisotopic precursors
    else if (algorithmType == "D")  massSlices.algorithmD(precursorPPM, rtStepSize); //ms2 based slices
    else if (algorithmType == "E")  massSlices.algorithmE(precursorPPM, rtStepSize*avgScanTime); //ms2 based slices, sequential merge by overlap

    printf("Execution time: %f seconds \n", ((float)getTime() - (float)functionAlgorithmTime) );

    //the slices generated by algorithm E do not contain any intensity information, so the vector can be returned
    //without sorting
    if (algorithmType == "E") {
        vector<mzSlice*> goodslices = vector<mzSlice*>(massSlices.slices.size());
        for (unsigned int i = 0; i < goodslices.size(); i++){
            mzSlice *mzSlicePtr = massSlices.slices.at(i);
            goodslices.at(i) = new mzSlice(*mzSlicePtr);
        }
        return goodslices;
    }

    sort(massSlices.slices.begin(), massSlices.slices.end(), mzSlice::compIntensity);

    vector<mzSlice*>goodslices;
	for( mzSlice* s: massSlices.slices) { 
			goodslices.push_back( new mzSlice( *s ));
			if(limit > 0 and goodslices.size() > limit) break;
	}

    cout << " processMassSlices.. done. #slices=" << goodslices.size() << endl;
	return goodslices;
}

    

void processSlices(vector<mzSlice*>&slices, string groupingAlgorithmType, string setName, bool writeReportFlag = false) {

    cout << "Process Slices: grouping algorithm=" << groupingAlgorithmType << ", " << slices.size() << " slices." << endl;

    if (slices.size() == 0 ) return;
    allgroups.clear();

    startTime = time(nullptr);

    double EICPullTime = 0.0;
    double peakGroupingTime = 0.0;
    double midPhaseTime = 0.0;
    double convergenceTime = 0.0;
    double endPhaseTime = 0.0;

    double startForLoopTime = getTime();

    long totalEicCounter = 0;
    long totalPeakCounter = 0;
    long totalGroupPreFilteringCounter = 0;
    long totalGroupsToAppendCounter = 0;

    #pragma omp parallel for num_threads(16) schedule(dynamic) shared(allgroups) reduction(+:totalEicCounter) reduction(+:totalPeakCounter) reduction(+:totalGroupPreFilteringCounter) reduction(+:totalGroupsToAppendCounter)
    for (unsigned int i = 0; i < slices.size();  i++ ) {
        if (i % 1000 == 0)
            cout << setprecision(2) << "Processing slices: (TID " << omp_get_thread_num() << ") " <<  i / (float) slices.size() * 100 << "% groups=" << allgroups.size() << endl;

        mzSlice* slice = slices[i];
        float mzmin = slices[i]->mzmin;
        float mzmax = slices[i]->mzmax;
        float rtmin = slices[i]->rtmin;
        float rtmax = slices[i]->rtmax;
        Compound* compound = slices[i]->compound;
        Adduct* adduct = slices[i]->adduct;

        if (i % 1000 == 0) {
            if (isQQQSearch) {
                cout << "\tEIC Slice "<< fixed << setprecision(5) << "precursor m/z: " << slice->srmPrecursorMz << " product m/z: " << slice->srmProductMz << endl;
            } else {
                cout << "\tEIC Slice " << setprecision(5) << mzmin <<  "-" << mzmax << " @ " << rtmin << "-" << rtmax << " I=" << slices[i]->ionCount << endl;
            }
        }

        //PULL EICS
        double startEICPullTime = getTime();
        vector<EIC*>eics; 
		float eicMaxIntensity = 0;

        for (auto sample: samples) {
			EIC* eic = nullptr;

            if (isQQQSearch) {
                if (!slice->srmTransition) {
                    cerr << "mzSlice* does not contain SRMTransition* for QQQ Search! Exiting.";
                    abort();
                }
                eic = sample->getEIC(slice->srmTransition, QQQparams->consensusIntensityAgglomerationType);

            } else if ( !slice->srmId.empty() ) {
                eic = sample->getEIC(slice->srmId);
            } else {
                eic = sample->getEIC(mzmin, mzmax, rtmin, rtmax, 1);
            }

			if(eic) {
				eics.push_back(eic);
				if (eic->maxIntensity > eicMaxIntensity) eicMaxIntensity = eic->maxIntensity;
			}
        }
        EICPullTime += getTime() - startEICPullTime;

        double startMidPhaseTime = getTime();
        //optimization.. skip over eics with low intensities
        if ( eicMaxIntensity < minGroupIntensity)  {  delete_all(eics); continue; }

        totalEicCounter += eics.size();

        // ------------------------- //
        // PEAK PICKING AND GROUPING //
        // ------------------------- //

        //find peaks
        if (groupingAlgorithmType != "B") {
            //peaks are found during EIC::groupPeaksB()

            for(unsigned int j=0; j < eics.size(); j++ ) {
                eics.at(j)->getPeakPositionsD(peakPickingAndGroupingParameters, false);
            }
        }

        vector<PeakGroup> peakgroups{};
        if (groupingAlgorithmType == "A") {

            peakgroups = EIC::groupPeaks(eics, eic_smoothingWindow, grouping_maxRtWindow);

        } else if (groupingAlgorithmType == "B") {

            peakgroups = EIC::groupPeaksB(eics, eic_smoothingWindow, grouping_maxRtWindow, minSmoothedPeakIntensity);

        } else if (groupingAlgorithmType == "C") {

            peakgroups = EIC::groupPeaksC(eics, eic_smoothingWindow, grouping_maxRtWindow, baseline_smoothingWindow, baseline_dropTopX);

        } else if (groupingAlgorithmType == "D") {

            peakgroups = EIC::groupPeaksD(eics, eic_smoothingWindow, grouping_maxRtWindow, baseline_smoothingWindow, baseline_dropTopX, mergeOverlap);

        } else if (groupingAlgorithmType == "E") {

            peakgroups = EIC::groupPeaksE(eics, peakPickingAndGroupingParameters, false);
        }

        for (auto eic : eics){
            totalPeakCounter += eic->peaks.size();
        }

        // -------------------- //
        // PEAK GROUP FILTERING //
        // -------------------- //

        //sort peaks by intensity
        sort(peakgroups.begin(), peakgroups.end(), PeakGroup::compIntensity);
        midPhaseTime += getTime() - startMidPhaseTime;

        double startPeakGroupingTime = getTime();
        vector<PeakGroup*> groupsToAppend;

        totalGroupPreFilteringCounter += peakgroups.size();

        for (PeakGroup& group: peakgroups) {

            if (compound) {
                group.compound = compound;
            }
            if (adduct) {
                group.adduct = adduct;
            }

            if (clsf.hasModel()) {
                clsf.classify(&group); //assigns peak.quality values
                group.groupStatistics();
            }

            if (clsf.hasModel() && group.goodPeakCount < peakPickingAndGroupingParameters->filterMinGoodGroupCount) continue;
            if (clsf.hasModel() && group.maxQuality < peakPickingAndGroupingParameters->filterMinQuality) continue;

            if (group.maxNoNoiseObs < static_cast<unsigned int>(peakPickingAndGroupingParameters->filterMinNoNoiseObs)) continue;
            if (group.maxSignalBaselineRatio < peakPickingAndGroupingParameters->filterMinSignalBaselineRatio) continue;
            if (group.maxIntensity < peakPickingAndGroupingParameters->filterMinGroupIntensity ) continue;
            if (group.blankMax*peakPickingAndGroupingParameters->filterMinSignalBlankRatio > group.maxIntensity) continue;

            if (!isQQQSearch) {

                if (getChargeStateFromMS1(&group) < peakPickingAndGroupingParameters->filterMinPrecursorCharge) continue;

                //Issue 797: Respect search parameters from CLI
                //build consensus ms2 specta
                if (isMzkitchenSearch) {
                    group.computeFragPattern(metaboliteSearchParams.get());
                } else {
                    vector<Scan*>ms2events = group.getFragmentationEvents();
                    if (ms2events.size()) {
                        sort(ms2events.begin(), ms2events.end(), Scan::compIntensity);
                        Fragment f = Fragment(ms2events[0], 0.01f, 1, 1024);
                        for (Scan* s : ms2events) {  f.addFragment(new Fragment(s, 0, 0.01f, 1024)); }
                        f.buildConsensus(productPpmTolr);
                        group.fragmentationPattern = f.consensus;
                        group.ms2EventCount = static_cast<int>(ms2events.size());
                    }
                }

                if (mustHaveMS2 and group.ms2EventCount == 0 ) continue;

                if (!slice->srmId.empty()) group.srmId = slice->srmId;

                groupsToAppend.push_back(&group);

            } else { //isQQQSearch == true

                // Currently, this will generate a many-to-many mapping
                // where each peak group is assigned to each compound in slice.
                //
                // TODO: implement logic to assign each compound to each peakgroup (max one peak group per compound).
                // TODO: this could happen later, as the metadata associated with qual/quant ions is carried along in the compound.

                group.setType(PeakGroup::SRMTransitionType);
                group.srmPrecursorMz = slice->srmPrecursorMz;
                group.srmProductMz = slice->srmProductMz;

                if (!slice->compound && slice->compoundVector.empty()) {

                    //TODO: memory leak
                    PeakGroup *groupCopy = new PeakGroup(group);
                    groupsToAppend.push_back(groupCopy);

                } else {
                    for (Compound* qqqCompound : slice->compoundVector) {

                        //TODO: memory leak
                        PeakGroup *groupCopy = new PeakGroup(group);
                        groupCopy->compound = qqqCompound;

                        // debugging
                        // cout << fixed << setprecision(2) << "(" << groupCopy.srmPrecursorMz << ", " << groupCopy.srmProductMz << "): "<< groupCopy.medianRt() << " " << groupCopy.compound->id << endl;

                        groupsToAppend.push_back(groupCopy);
                    }
                }

            }

        }

        totalGroupsToAppendCounter += groupsToAppend.size();

        //cout << ".";

        int appendCount = 0;
        std::sort(groupsToAppend.begin(), groupsToAppend.end(), PeakGroup::compRankPtr);
        for (int j = 0; j < groupsToAppend.size(); j++) {
            if (appendCount >= eicMaxGroups) break;
            PeakGroup* group = groupsToAppend[j];
            appendCount++;
			#pragma omp critical
            allgroups.push_back(*group);
        }

        peakGroupingTime += getTime() - startPeakGroupingTime;
        double startEndPhaseTime = getTime();
        delete_all(eics);
        eics.clear();
        endPhaseTime += getTime() - startEndPhaseTime;
    } // threads ends

    cout << "Number of eics prior to peak grouping: " << totalEicCounter << endl;
    cout << "Number of peaks in all eics prior to same-mzSlice pre-processing and filtering: " << totalPeakCounter << endl;
    cout << "Number of peakgroups prior to classifier-based filtering: " << totalGroupPreFilteringCounter << endl;
    cout << "Number of peakgroups prior to eicMaxGroups filtering: " << totalGroupsToAppendCounter << endl;

    //Issue 629: record group ID field, for use with subsets
    //This gets overwritten during QQQProcessor::rollUpToCompoundQuant() for QQQ data
    unsigned long rankCounter = 0;
    for (auto& group : allgroups) {
        group.groupRank = static_cast<float>(rankCounter);
        rankCounter++;
    }

    //Issue 692: peakGroupsD() and later include reduceGroups()
    if (groupingAlgorithmType == "A" || groupingAlgorithmType == "B" || groupingAlgorithmType == "C") {
        reduceGroups();
    }

    printf("Execution time (for loop)     : %f seconds \n", getTime() - startForLoopTime);
    printf("\tExecution time (EICPulling)      : %f seconds \n", EICPullTime);
    printf("\tExecution time (ConvergenceTime) : %f seconds \n", convergenceTime);
    printf("\tExecution time (MidPhaseTime)    : %f seconds \n", midPhaseTime);
    printf("\tExecution time (PeakGrouping)    : %f seconds \n", peakGroupingTime);
    printf("\tExecution time (EndPhaseTime)    : %f seconds \n", endPhaseTime);
    printf("\tParallelisation (PeakGrouping)   : True\n");

    if (isMzkitchenSearch) {
        mzkitchenSearch();
    }

    if (isQQQSearch) {

        //Assign background type prior to filtering to ensure that filtering works correctly.
        QQQProcessor::setPeakGroupBackground(allgroups, QQQparams, false);

        //Issue 660: QQQ-specific blank filter handling
        //Run this before QQQProcessor::rollUpToCompoundQuant(), as the roll-up organizes
        //subordinate groups as children PeakGroups.
        vector<PeakGroup> blankFilteredPeakGroups = QQQProcessor::filterPeakGroups(allgroups, QQQparams, false);
        allgroups = blankFilteredPeakGroups;

        //Issue 565
        QQQProcessor::rollUpToCompoundQuant(allgroups, QQQparams, false);
        QQQProcessor::labelInternalStandards(allgroups, QQQparams, false);

        //Issue 635: clean up deleted flags
        vector<PeakGroup> updated_allgroups = vector<PeakGroup>{};

        for (PeakGroup pg : allgroups) {
            if (!pg.deletedFlag){
                updated_allgroups.push_back(pg);
            }
        }
        allgroups = updated_allgroups;

        //Issue 662: pass forward pre-labels
        for (auto& peakGroup : allgroups) {
            peakGroup.applyLabelsFromCompoundMetadata();
        }

    }

    if (isPeptideStabilitySearch) {

        allgroups = PeptideStabilityProcessor::processPeptideStabilitySet(
            allgroups,
            samples,
            peptideStabilitySearchParams,
            false);

        PeptideStabilityProcessor::labelPeptideStabilitySet(allgroups, peptideStabilitySearchParams, false);

    }

    double startWriteReportTime = getTime();
    if (writeReportFlag){
        if (isQQQSearch) {
            searchTableData.insert(make_pair("QQQ Peak Detection",
                                             QString(QQQparams->encodeParams().c_str())));
        }
        if (isPeptideStabilitySearch) {
            searchTableData.insert(make_pair("Peptide Stability Search",
                                             QString(peptideStabilitySearchParams->encodeParams().c_str())));
        }
        writeReport(setName);
        printf("Execution time (Write report) : %f seconds \n", getTime() - startWriteReportTime);
    } else {
        cout << "Report not written." << endl;
    }

    cout << "DONE!" << endl;
}

void processOptions(int argc, char* argv[]) {

    //command line options
    const char * optv[] = {
        "a?alignSamples",
        "b?minGoodGroupCount <int>",
        "c?matchRtFlag <int>",
        "d?db <string>",
        "e?algorithmType <string>",
        "f?groupingAlgorithmType <string>",
        "g?grouping_maxRtWindow <float>",
        "h?help",
        "i?minGroupIntensity <float>",
        "j?minSmoothedPeakIntensity <float>",
        "k?productPpmTolr <float>",
        "l?saveScanData <int>",
        "m?methodsFolder <string>",
        "n?eicMaxGroups <int>",
        "o?outputdir <string>",
        "p?ppmMerge <float>",
        "q?minQuality <float>",
        "r?rtStepSize <float>",
        "s?nameSuffix <string>",
        "t?scoringScheme <string>",
        "u?mergeOverlap <float>",
        "v?minStandardAlignmentPeakIntensity <float>",
        "w?minPeakWidth <int>",
        "x?minPrecursorCharge <int>",
        "y?eicSmoothingWindow <int>",
        "z?minSignalBaseLineRatio <float>",
        "1?mzkitchenMspFile <string>",
        "2?mustHaveMS2",
        "3?runClustering",
        "4?compoundLibraryFile <string>",
        "5?standardsAlignment_maxRtWindow <float>",
        "6?standardsAlignment_precursorPPM <float>",
        "7?baselineSmoothingWindow <int>",
        "8?baselineDropTopX <int>",
        "9?mzkitchenSearchParameters <string>",
        "0?mzKitchenSearchType <string>",
        nullptr
    };

    //parse input options
    Options  opts(*argv, optv);
    OptArgvIter  iter(--argc, ++argv);
    const char * optarg;

    //single character CL options are specified explicitly,
    //long options are handled separately.
    opts.ctrls(Options::OptCtrl::NOGUESSING);

    //Issue 629: All single-dash arguments should be supplied before any double dash (--) arguments.
    //any single-dash arguments are specified after any double dash argument will be ignored.

    while ( const char optchar = opts(iter, optarg) ) {
        switch (optchar) {
        case 'a' : alignSamplesFlag = atoi(optarg); break;
        case 'b' : minGoodGroupCount = atoi(optarg); break;
        case 'c' : rtWindow = atof(optarg); matchRtFlag = true; if (rtWindow == 0) matchRtFlag = false; break;
        case 'd' : ligandDbFilename = optarg; break;
        case 'e' : algorithmType = string(optarg); break;
        case 'f' : groupingAlgorithmType = string(optarg); break;
        case 'g' : grouping_maxRtWindow = atof(optarg); break;
        case 'h' : opts.usage(cout, "files ..."); exit(0); break;
        case 'i' : minGroupIntensity = atof(optarg); break;
        case 'j' : minSmoothedPeakIntensity = atof(optarg); break;
        case 'k' : productPpmTolr = atof(optarg); break;
        case 'm' : methodsFolder = optarg; break;
        case 'l' : saveScanData = (bool) atoi(optarg); break;
        case 'n' : eicMaxGroups = atoi(optarg); break;
        case 'o' : outputdir = optarg + string(DIR_SEPARATOR_STR); break;
        case 'p' : precursorPPM = atof(optarg); break;
        case 'q' : minQuality = atof(optarg); break;
        case 'r' : rtStepSize = atof(optarg); break;
        case 's' : nameSuffix = string(optarg); break;
        case 't' : scoringScheme = string(optarg); break;
        case 'u' : mergeOverlap = atof(optarg); break;
        case 'v' : standardsAlignment_minPeakIntensity = atof(optarg); break;
        case 'w' : minNoNoiseObs = atoi(optarg); break;
        case 'x' : minPrecursorCharge = atoi(optarg); break;
        case 'y' : eic_smoothingWindow = atoi(optarg); break;
        case 'z' : minSignalBaseLineRatio = atof(optarg); break;
        case '1' : mzkitchenMspFile = string(optarg); break;
        case '2' : mustHaveMS2 = true; writeConsensusMS2 = true; break;
        case '3' : isRunClustering = true; break;
        case '4' : compoundLibraryFile = string(optarg); break;
        case '5' : standardsAlignment_maxRtWindow = atof(optarg); break;
        case '6' : standardsAlignment_precursorPPM = atof(optarg); break;
        case '7' : baseline_smoothingWindow = atoi(optarg); break;
        case '8' : baseline_dropTopX = atoi(optarg); break;
        case '9' : mzkitchenSearchParameters = string(optarg); break;
        case '0' : mzkitchenSearchType = string(optarg); break;
        default : break;
        }
    }

    for (int i = 1; i < argc ; i++) {
        string optString(argv[i]);

        //CL options not covered in single character shortcuts
        if (strcmp(argv[i], "--mergedPeakRtBoundsMaxIntensityFraction") == 0) {
            mergedPeakRtBoundsMaxIntensityFraction = atof(argv[i+1]);
        } else if (strcmp(argv[i], "--mergedPeakRtBoundsSlopeThreshold") == 0) {
            mergedPeakRtBoundsSlopeThreshold = atof(argv[i+1]);
        } else if (strcmp(argv[i], "--mergedSmoothedMaxToBoundsMinRatio") == 0) {
            mergedSmoothedMaxToBoundsMinRatio = atof(argv[i+1]);
        } else if (strcmp(argv[i], "--mergedSmoothedMaxToBoundsIntensityPolicy") == 0) {
            string policy = string(argv[i+1]);
            if (policy == "MEDIAN") {
                mergedSmoothedMaxToBoundsIntensityPolicy = SmoothedMaxToBoundsIntensityPolicy::MEDIAN;
            } else if (policy == "MAXIMUM") {
                mergedSmoothedMaxToBoundsIntensityPolicy = SmoothedMaxToBoundsIntensityPolicy::MAXIMUM;
            } else if (policy == "MINIMUM") {
                mergedSmoothedMaxToBoundsIntensityPolicy = SmoothedMaxToBoundsIntensityPolicy::MINIMUM;
            }
        } else if (strcmp(argv[i], "--peakRtBoundsSlopeThreshold") == 0) {
            peakRtBoundsSlopeThreshold = atof(argv[i+1]);
        } else if (strcmp(argv[i], "--peakRtBoundsMaxIntensityFraction") == 0) {
            peakRtBoundsMaxIntensityFraction = atof(argv[i+1]);
        } else if (strcmp(argv[i], "--eicBaselineEstimationType") == 0) {
            string estimationType = argv[i+1];
            if (estimationType == "DROP_TOP_X") {
                eicBaselineEstimationType = EICBaselineEstimationType::DROP_TOP_X;
            } else if (estimationType == "EIC_NON_PEAK_MAX_SMOOTHED_INTENSITY") {
                eicBaselineEstimationType = EICBaselineEstimationType::EIC_NON_PEAK_MAX_SMOOTHED_INTENSITY;
            } else if (estimationType == "EIC_NON_PEAK_MEDIAN_SMOOTHED_INTENSITY") {
                eicBaselineEstimationType = EICBaselineEstimationType::EIC_NON_PEAK_MEDIAN_SMOOTHED_INTENSITY;
            }
        } else if (strcmp(argv[i], "--sampleIdMappingFile") == 0) {
            sampleIdMappingFile = argv[i+1];
        } else if (strcmp(argv[i], "--minSignalBlankRatio") == 0) {
            minSignalBlankRatio = stof(argv[i+1]);
        } else if (strcmp(argv[i], "--groupBackgroundType") == 0) {
            string groupBackgroundTypeStr = argv[i+1];
            if (groupBackgroundTypeStr == "MAX_BLANK_INTENSITY") {
                groupBackgroundType = PeakGroupBackgroundType::MAX_BLANK_INTENSITY;
            } else if (groupBackgroundTypeStr == "PREFERRED_QUANT_TYPE_MERGED_EIC_BASELINE") {
                groupBackgroundType = PeakGroupBackgroundType::PREFERRED_QUANT_TYPE_MERGED_EIC_BASELINE;
            } else if (groupBackgroundTypeStr == "PREFERRED_QUANT_TYPE_MAX_BLANK_SIGNAL") {
                groupBackgroundType = PeakGroupBackgroundType::PREFERRED_QUANT_TYPE_MAX_BLANK_SIGNAL;
            }
        } else if (strcmp(argv[i], "--isQQQCSVExport") == 0) {
            isQQQCSVExport = strcmp(argv[i], "1");
        } else if (strcmp(argv[i], "--isotopeParameters") == 0) {
            string isotopeParametersStr = argv[i+1];
            isotopeParameters = IsotopeParameters::decode(isotopeParametersStr);
            isExtractIsotopes = true;
        } else if (strcmp(argv[i], "--adductsFile") == 0) {
            vector<Adduct*> adducts = Adduct::loadAdducts(argv[i+1]);
            for (Adduct* adduct : adducts) {
                loadedAdducts.insert(make_pair(adduct->name, adduct));
                cout << "Adduct Loaded: " << adduct->name << endl;
            }
        } else if (strcmp(argv[i], "--rtAlignmentAnchorPointReference") == 0) {
            rtAlignmentAnchorPointReference = strcmp(argv[i], "1");
        } else if (strcmp(argv[i], "--peptideStabilitySearchParameters") == 0) {
            peptideStabilitySearchParamsStr = argv[i+1];
            isPeptideStabilitySearch = true;
        }

        if (mzUtils::ends_with(optString, ".rt")) alignmentFile = optString;
        if (mzUtils::ends_with(optString, ".apts")) anchorPointsFile = optString;
        if (mzUtils::ends_with(optString, ".mzrollDB")) projectFile = optString;
        if (mzUtils::ends_with(optString, ".mzXML")) filenames.push_back(optString);
        if (mzUtils::ends_with(optString, ".mzML"))  filenames.push_back(optString);

        if (mzUtils::ends_with(optString, "/")) {
            vector<string> dirFileNames = mzUtils::getMzSampleFilesFromDirectory(optString.c_str());
            for (auto dirFileName : dirFileNames) {
                filenames.push_back(dirFileName);
            }
        }
    }

    filenames = removeDuplicateFilenames(filenames);

    cout << "#Project:\t" << projectFile << endl;
    cout << "#Command:\t";
    for (int i = 0; i < argc; i++) {
        cout << argv[i] << " "; cout << endl;
    }

    fillOutPeakPickingAndGroupingParameters();

    //Issue 553: handle QQQ searches
    if (mzkitchenSearchType == "qqqSearch") {
        isQQQSearch = true;
        QQQparams = QQQSearchParameters::decode(mzkitchenSearchParameters);
        saveScanData = false; //override setting, saving scans for QQQ data doesn't make any sense
        isMzkitchenSearch = false; //QQQ search is vastly different from mzkitchen search, avoid accidental calling of mzkitchenSearch() function
        isRunClustering = false; //In this context, clustering is RT elution time based, doesn't make sense for SRM data

        //Override any previously encoded peak picking parameters with CL options
        QQQparams->peakPickingAndGroupingParameters = peakPickingAndGroupingParameters;

    //Issue 797: define both searches here, decode silently skips over any parameters that it does not understand
    } else if (mzkitchenSearchType == "lipidSearch" || mzkitchenSearchType == "metaboliteSearch") {
        isMzkitchenSearch = true;

        lipidSearchParams = LCLipidSearchParameters::decode(mzkitchenSearchParameters);
        lipidSearchParams->IDisRequireMatchingAdduct = false;  //this parameter must be false in a peakdetector context, because no Adduct* are provided
        lipidSearchParams->searchVersion = "lipids_lclipidprocessor_v1";
        lipidSearchParams->peakPickingAndGroupingParameters = peakPickingAndGroupingParameters; //override encoded values with CL options

        metaboliteSearchParams = MzkitchenMetaboliteSearchParameters::decode(mzkitchenSearchParameters);
        metaboliteSearchParams->IDisRequireMatchingAdduct = false; //this parameter must be false in a peakdetector context, because no Adduct* are provided
        metaboliteSearchParams->searchVersion = "mzkitchen32_metabolite_search";
        metaboliteSearchParams->peakPickingAndGroupingParameters = peakPickingAndGroupingParameters; //override encoded values with CL options
    } else if (isPeptideStabilitySearch) {
        peptideStabilitySearchParams = PeptideStabilitySearchParameters::decode(peptideStabilitySearchParamsStr);
        peptideStabilitySearchParams->peakPickingAndGroupingParameters = peakPickingAndGroupingParameters;
        if (isExtractIsotopes) {
            peptideStabilitySearchParams->isotopeParameters = isotopeParameters;
        }
    }
}

void fillOutPeakPickingAndGroupingParameters() {

    // START EIC::getPeakPositionsD()
    //peak picking
    peakPickingAndGroupingParameters->peakSmoothingWindow = eic_smoothingWindow;
    peakPickingAndGroupingParameters->peakRtBoundsMaxIntensityFraction = peakRtBoundsMaxIntensityFraction;
    peakPickingAndGroupingParameters->peakRtBoundsSlopeThreshold = peakRtBoundsSlopeThreshold;
    peakPickingAndGroupingParameters->peakBaselineSmoothingWindow = baseline_smoothingWindow;
    peakPickingAndGroupingParameters->peakBaselineDropTopX = baseline_dropTopX;
    peakPickingAndGroupingParameters->peakIsComputeBounds = true;

    //eic
    peakPickingAndGroupingParameters->eicBaselineEstimationType = eicBaselineEstimationType;

    // END EIC::getPeakPositionsD()

    // START EIC::groupPeaksE()

    //merged EIC
    peakPickingAndGroupingParameters->mergedSmoothingWindow = eic_smoothingWindow;
    peakPickingAndGroupingParameters->mergedPeakRtBoundsMaxIntensityFraction = mergedPeakRtBoundsMaxIntensityFraction;
    peakPickingAndGroupingParameters->mergedPeakRtBoundsSlopeThreshold = mergedPeakRtBoundsSlopeThreshold;
    peakPickingAndGroupingParameters->mergedSmoothedMaxToBoundsMinRatio = mergedSmoothedMaxToBoundsMinRatio;
    peakPickingAndGroupingParameters->mergedSmoothedMaxToBoundsIntensityPolicy = mergedSmoothedMaxToBoundsIntensityPolicy;
    peakPickingAndGroupingParameters->mergedBaselineSmoothingWindow = baseline_smoothingWindow;
    peakPickingAndGroupingParameters->mergedBaselineDropTopX = baseline_dropTopX;
    if (mergedPeakRtBoundsMaxIntensityFraction > 0 || mergedPeakRtBoundsSlopeThreshold > 0 || mergedSmoothedMaxToBoundsMinRatio > 0) {
        peakPickingAndGroupingParameters->mergedIsComputeBounds = true;
    }

    //grouping
    peakPickingAndGroupingParameters->groupMaxRtDiff = grouping_maxRtWindow;
    peakPickingAndGroupingParameters->groupMergeOverlap = mergeOverlap;

    //computed properties
    peakPickingAndGroupingParameters->groupBackgroundType = groupBackgroundType;

    //post-grouping filters
    peakPickingAndGroupingParameters->filterMinGoodGroupCount = minGoodGroupCount;
    peakPickingAndGroupingParameters->filterMinQuality = minQuality;
    peakPickingAndGroupingParameters->filterMinNoNoiseObs = minNoNoiseObs;
    peakPickingAndGroupingParameters->filterMinSignalBaselineRatio = minSignalBaseLineRatio;
    peakPickingAndGroupingParameters->filterMinGroupIntensity = minGroupIntensity;
    peakPickingAndGroupingParameters->filterMinPrecursorCharge = minPrecursorCharge;
    peakPickingAndGroupingParameters->filterMinSignalBlankRatio = minSignalBlankRatio;

    // END EIC::groupPeaksE()
}

void printSettings() {

    cout << "\n=================================================" << endl;
    cout << "#Files (and related):" << endl;
    cout << "#Ligand Database file:\t" << ligandDbFilename << endl;
    cout << "#methodsFolder:\t" << methodsFolder << endl;
    cout << "#Classification Model file\t" << clsfModelFilename << endl;
    cout << "#Output folder:\t" <<  outputdir << endl << endl;
    cout << "#avgScanTime (computed from sample list)=" << avgScanTime << endl;
    cout << "#nameSuffix=" << nameSuffix << endl;
    cout << "#anchorPointsFile=" << anchorPointsFile << endl;

    cout << "\n=================================================" << endl;
    cout << "#m/z Slice Determination:" << endl;
    cout << "#slicing algorithm code: " << algorithmType << endl;
    cout << "#rtStepSize=" << rtStepSize << endl;
    cout << "#m/z slice merge precursorPPM="  << precursorPPM << endl;

    cout << "\n=================================================" << endl;
    cout << "#Peak Picking:" << endl;
    cout << "#eic_smoothingWindow=" << eic_smoothingWindow << endl;
    cout << "#ionizationMode=" << ionizationMode << endl;
    cout << "#baseline_smoothingWindow=" << baseline_smoothingWindow << endl;
    cout << "#baseline_dropTopX=" << baseline_dropTopX << endl;
    cout << "#peakRtBoundsSlopeThreshold=" << peakRtBoundsSlopeThreshold << endl;
    cout << "#Baseline Estimation Type: ";
    if (eicBaselineEstimationType == EICBaselineEstimationType::DROP_TOP_X) {
        cout << "DROP_TOP_X" << endl;
    } else if (eicBaselineEstimationType == EICBaselineEstimationType::EIC_NON_PEAK_MAX_SMOOTHED_INTENSITY) {
        cout << "EIC_NON_PEAK_MAX_SMOOTHED_INTENSITY" << endl;
    } else if (eicBaselineEstimationType == EICBaselineEstimationType::EIC_NON_PEAK_MEDIAN_SMOOTHED_INTENSITY) {
        cout << "EIC_NON_PEAK_MEDIAN_SMOOTHED_INTENSITY" << endl;
    }

    cout << "\n=================================================" << endl;
    cout << "#Peak Grouping:" << endl;
    cout << "#Peak Grouping Algorithm:" << groupingAlgorithmType << endl;
    cout << "#grouping_maxRtWindow=" << grouping_maxRtWindow << endl;
    cout << "#mergeOverlap=" << mergeOverlap << endl;

    cout << "\n=================================================" << endl;
    cout << "#Merged EIC Grouping Settings" << endl;
    cout << "mergedSmoothingWindow="<< eic_smoothingWindow << endl;
    cout << "mergedPeakRtBoundsMaxIntensityFraction=" << mergedPeakRtBoundsMaxIntensityFraction << endl;
    cout << "mergedPeakRtBoundsSlopeThreshold=" << mergedPeakRtBoundsSlopeThreshold << endl;
    cout << "mergedSmoothedMaxToBoundsMinRatio=" << mergedSmoothedMaxToBoundsMinRatio << endl;
    cout << "mergedSmoothedMaxToBoundsIntensityPolicy=";
    if (mergedSmoothedMaxToBoundsIntensityPolicy == SmoothedMaxToBoundsIntensityPolicy::MEDIAN) {
        cout << "MEDIAN" << endl;
    } else if (mergedSmoothedMaxToBoundsIntensityPolicy == SmoothedMaxToBoundsIntensityPolicy::MAXIMUM) {
        cout << "MAXIMUM" << endl;
    } else if (mergedSmoothedMaxToBoundsIntensityPolicy == SmoothedMaxToBoundsIntensityPolicy::MINIMUM) {
        cout << "MINIMUM" << endl;
    }
    cout << "mergedBaselineSmoothingWindow=" << baseline_smoothingWindow << endl;
    cout << "mergedBaselineDropTopX=" << baseline_dropTopX << endl;

    cout << "\n=================================================" << endl;
    cout << "#Special Search Type:" << endl;
    cout << "#isQQQSearch? " << isQQQSearch << endl;
    if (isQQQSearch) {
        cout << "#QQQParams: " << QQQparams->encodeParams() << endl;
    }
    cout << "#isMzkitchenSearch? " << isMzkitchenSearch << endl;
    cout << "#mzkitchenSearchType=" << mzkitchenSearchType << endl;
    cout << "#mzkitchenMspFile=" << mzkitchenMspFile << endl;
    cout << "#mzkitchenSearchParameters: " << mzkitchenSearchParameters << endl;

    cout << "\n=================================================" << endl;

    cout << "#isPeptideStabilitySearch? " << isPeptideStabilitySearch << endl;
    if (isPeptideStabilitySearch) {
        cout << "#PeptideStabilitySearchParams: " << peptideStabilitySearchParams->encodeParams() << endl;
    }

    cout << "\n=================================================" << endl;
    cout << "#Peak Group Cleaning and Filtering" << endl;
    cout << "#minGoodGroupCount=" << minGoodGroupCount << endl;
    cout << "#minQuality=" << minQuality << endl;
    cout << "#minSignalBlankRatio=" << minSignalBlankRatio << endl;
    cout << "#minSignalBaseLineRatio=" << minSignalBaseLineRatio << endl;

    cout << "\n=================================================" << endl;
    cout << "#Samples:" << endl;

    for (unsigned int i = 0; i < samples.size(); i++) {
        cout << "#Sample " << i << ": " << samples[i]->sampleName << endl;
    }
    cout << endl;
}

vector<string> removeDuplicateFilenames(vector<string>& filenames) {
    //remove duplicates from filename list
    //
    map<string,bool> uniq_filenames_map;
    vector<string>   uniq_filenames_vec;
    for(string f: filenames) uniq_filenames_map[f] = 1;
    for(auto p : uniq_filenames_map) uniq_filenames_vec.push_back(p.first);
    return uniq_filenames_vec;
}


void loadSamples(vector<string>&filenames) {
    cout << "Loading samples" << endl;


    #pragma omp parallel for ordered num_threads(4) schedule(static)
    for (unsigned int i = 0; i <filenames.size(); i++ ) {

        #pragma omp critical
        cout << "Thread # " << omp_get_thread_num() << endl;

        #pragma omp critical
        cout << "Loading " << filenames[i] << endl;

        mzSample* sample = new mzSample();
        sample->loadSample(filenames[i].c_str());

        if (isQQQSearch) {
            sample->enumerateSRMScans();
        }

        if ( sample->scans.size() >= 1 ) {

            #pragma omp critical
            samples.push_back(sample);

            #pragma omp critical
            sample->summary();

        } else {
            if (sample) {
                delete sample;
                sample = nullptr;
            }
        }
    }

    cout << "loadSamples() done: loaded " << samples.size() << " samples\n";

    if (sampleIdMappingFile.empty()) {
        
        // Issue 746: Enforce natural ordering
    	QCollator collator;
		collator.setNumericMode(true);
	
		sort(samples.begin(), samples.end(), [collator](mzSample* lhs, mzSample* rhs){
		
			QString leftName = QString(lhs->sampleName.c_str());
			QString rightName = QString(rhs->sampleName.c_str());
		
			return collator.compare(leftName, rightName) < 0;
		});

        //ids match natural order
        for (unsigned int i = 0; i < samples.size(); i++){
            samples.at(i)->setSampleId(static_cast<int>(i));
            samples.at(i)->setSampleOrder(static_cast<int>(i));
        }
    } else {
        setSampleIdFromFile();
    }
}

void setSampleIdFromFile() {

    set<int> usedIds{};
    string line, word;
    vector<string> row;
    fstream sampleIdMappingFileStream(sampleIdMappingFile);
    if (sampleIdMappingFileStream.is_open()) {
        while(getline(sampleIdMappingFileStream, line)) {

            if (line[0] == '#') { // skip commented lines
                continue;
            }

            stringstream str(line);
            while(getline(str, word, '\t')) {
                row.push_back(word);
            }

            if (row.size() >= 3) {
                string sampleName = row[0];
                int sampleId = stoi(row[1]);
                bool isAnchorPointSample = row[2] == "1";

                cout << "sampleName=" << sampleName << " sampleId=" << sampleId << endl;

                if (sampleIdMapping.find(sampleName) != sampleIdMapping.end()) {
                    cerr << "Duplicate sampleName '" << sampleName << "' in sampleIdMappingFile! Exiting." << endl;
                    abort();
                }

                if (usedIds.find(sampleId) != usedIds.end()) {
                    cerr << "Duplicate ID: " << sampleId << " in sampleMapping File! Exiting." << endl;
                    abort();
                }

                usedIds.insert(sampleId);
                sampleIdMapping.insert(make_pair(sampleName, make_pair(sampleId, isAnchorPointSample)));

                row.clear();
            }

        }
    }

    //Every sample must receive an id and sample order number from the map
    for (auto sample : samples){
        if (sampleIdMapping.find(sample->sampleName) != sampleIdMapping.end()) {
            pair<int, bool> sampleData= sampleIdMapping[sample->sampleName];
            int sampleId = sampleData.first;
            bool isAnchorPointSample = sampleData.second;

            sample->setSampleId(sampleId);
            sample->setSampleOrder(sampleId);
            sample->isAnchorPointSample = isAnchorPointSample;

        } else {
            cerr << "setSampleIdFromFile() Failed! sample '" << sample->sampleName << "' did not have an ID\n"
                 << "in the sampleIdMapping file '" << sampleIdMappingFile << "'.\n"
                 << "All samples must have an ID in the sampleMappingFile." << endl;
            abort();
        }
    }

    //Issue 629: sort all samples based on sampleId to avoid stochasticity with different alignments
    sort(samples.begin(), samples.end(), [](const mzSample* lhs, const mzSample* rhs){
        return lhs->sampleId < rhs->sampleId;
    });
}
/**
 * @brief reduceGroups
 * @deprecated groupPeaksD() now incorporates this step
 */
void reduceGroups() {

    sort(allgroups.begin(), allgroups.end(), [](const PeakGroup& lhs, const PeakGroup&rhs){
        if (lhs.meanMz == rhs.meanMz) {
            return lhs.meanRt < rhs.meanRt;
        } else {
            return lhs.meanMz < rhs.meanMz;
        }
    });

    cout << "reduceGroups(): start with " << allgroups.size() << " groups." << endl;

    //init deleteFlag
    for (unsigned int i = 0; i < allgroups.size(); i++) {
        allgroups[i].deletedFlag = false;
    }

    for (unsigned int i = 0; i < allgroups.size(); i++) {
        PeakGroup& grp1 = allgroups[i];
        if (grp1.deletedFlag) continue;
        for (unsigned int j = i + 1; j < allgroups.size(); j++) {
            PeakGroup& grp2 = allgroups[j];
            if ( grp2.deletedFlag) continue;

            float rtoverlap = mzUtils::checkOverlap(grp1.minRt, grp1.maxRt, grp2.minRt, grp2.maxRt);
            float ppmdist = ppmDist(grp2.meanMz, grp1.meanMz);
            if ( ppmdist > precursorPPM ) break;

            if ( rtoverlap > mergeOverlap && ppmdist < precursorPPM) {
                if ( grp1.maxIntensity <= grp2.maxIntensity) {
                    grp1.deletedFlag = true;
                    //allgroups.erase(allgroups.begin()+i);
                    //i--;
                    break;
                } else if ( grp1.maxIntensity > grp2.maxIntensity) {
                    grp2.deletedFlag = true;
                    //allgroups.erase(allgroups.begin()+j);
                    //i--;
                    // break;
                }
            }
        }
    }

    int reducedGroupCount = 0;
    vector<PeakGroup> allgroups_;
    for (int i = 0; i < allgroups.size(); i++)
    {
        PeakGroup& grp1 = allgroups[i];
        if (!grp1.deletedFlag) {
            allgroups_.push_back(grp1);
            reducedGroupCount++;
        }
    }

    allgroups = allgroups_;
    cout << "reduceGroups(): finish with " << allgroups.size() << " groups." << endl;
}

//Peaks are automatically numbered by projectDB.writeGroupSqlite(), based on their order in the PeakGroup.peaks().
//EIC::groupPeaks() sorts these peaks within a peak group as a part of peak grouping.
void reNumberGroups() {

    int groupNum = 1;
    for (PeakGroup &g : allgroups) {

        g.groupId = groupNum++;

        for (auto &c : g.children) {
            c.groupId = groupNum++;
        }

    }

}

void loadProject(string projectFile) {
    cout << "Loading project " << projectFile;
    project = new ProjectDB(projectFile.c_str());
    project->loadSamples();
    project->doAlignment();
    samples = project->getSamples();

}

void writeReport(string setName) {
    if (writeReportFlag == false ) return;

    cout << "Writing report to " << setName << endl;

    reNumberGroups();

    if (isRunClustering) {

        //run clustering
        double maxRtDiff = 0.2;
        double minSampleCorrelation = 0.8;
        double minPeakShapeCorrelation = 0.9;

        PeakGroup::clusterGroups(allgroups, samples, maxRtDiff, minSampleCorrelation, minPeakShapeCorrelation, precursorPPM);
    }

    //create an output folder
    mzUtils::createDir(outputdir.c_str());

    //5% of time writeReport() spent up to this point

    QString tblName = QString(setName.c_str());

    if (isMzkitchenSearch) {
        tblName = QString("clamDB");
    } else if (isQQQSearch) {
        tblName = QString("QQQ Peak Detection");
    } else if (isPeptideStabilitySearch) {
        tblName = QString("Peptide Stability Search");
    }

    if (saveSqlLiteProject)  {

        //open project if need be
        if (project and project->isOpen()) {
            cout << "Overwriting previous search results" << endl;
            project->deleteSearchResults(setName.c_str());

            if (isSpecialSearch()) {
                project->deleteSearchResults(tblName);
                project->savePeakGroupsTableData(searchTableData);

                set<Compound*> compoundSet;
                for (auto& group : allgroups){
                    traverseAndAdd(group, compoundSet);
                    if (group.compound){
                        project->writeGroupSqlite(&group, 0, tblName, saveScanData);
                    } else {
                        project->writeGroupSqlite(&group, 0, setName.c_str(), saveScanData);
                    }
                }
                project->saveCompounds(compoundSet);
            } else {
                project->saveGroups(allgroups, setName.c_str(), saveScanData);

                set<Compound*> compoundSet;
                for (auto & group : allgroups) {
                    traverseAndAdd(group, compoundSet);
                }
                project->saveCompounds(compoundSet);
            }

        } else {
	    //create new sqlite database
            cout << "Creating new project file" << endl;
            string projectDBfilename = outputdir + setName + ".mzrollDB";

            qDebug() << "new project file:" << projectDBfilename.c_str();
            project = new ProjectDB(projectDBfilename.c_str());

            if ( project->isOpen() ) {
                project->deleteAll();
                Database::setDefaultSampleColors(samples);
				project->setSamples(samples);
                project->saveSamples(samples);

                //this is only necessary if RT alignment is actually performed
                //Issue 641: Explicitly save anchor points
                if (!sampleToUpdatedRts.empty()) {
                    project->saveAlignment(sampleToUpdatedRts);
                }

                if (isSpecialSearch()) {
                    set<Compound*> compoundSet;
                    for (auto& group : allgroups){
                        traverseAndAdd(group, compoundSet);
                        if (group.compound){
                            project->writeGroupSqlite(&group, 0, tblName, saveScanData);
                        } else {
                            project->writeGroupSqlite(&group, 0, setName.c_str(), saveScanData);
                        }
                    }
                    project->saveCompounds(compoundSet);
                } else {
                    project->saveGroups(allgroups, setName.c_str(), saveScanData);

                    set<Compound*> compoundSet;
                    for (auto & group : allgroups) {
                        traverseAndAdd(group, compoundSet);
                    }
                    project->saveCompounds(compoundSet);
                }

                project->savePeakGroupsTableData(searchTableData);

            } else {
                cout << "Failed to open project file" << endl;
			}
        }
    }

    cout << "writeReport() " << allgroups.size() << " groups " << endl;

    if (!isSpecialSearch()) {
        writeCSVReport(outputdir + setName + ".csv");
    } else if (isQQQCSVExport) {
        writeQQQReport(outputdir + setName + ".csv");
    } else if (isPeptideStabilitySearch) {
        writePeptideStabilityReport(outputdir + setName + ".csv");
    }

    //if(writeConsensusMS2) writeConsensusMS2Spectra(outputdir + setName + ".msp");
    //if(writeConsensusMS2) classConsensusMS2Spectra(outputdir + setName + "CLASS.msp");
}

void writeQQQReport(string filename) {
    ofstream groupReport;
    groupReport.open(filename.c_str());
    if (! groupReport.is_open()) return;

    string SEP = csvFileFieldSeperator;

    QStringList Header;
    Header << "label" << "metaGroupId"
           << "groupId"
           << "goodPeakCount"
           << "medMz"
           << "medRt"
           << "maxQuality"
           << "note"
           << "compound"
           << "compoundId"
           << "category"
           << "database"
           << "expectedRtDiff"
           << "ppmDiff"
           << "parent"
           << "ms2EventCount"
           << "fragNumIonsMatched"
           << "fragFracMatched"
           << "ticMatched"
           << "weightedDotProduct"
           << "hypergeomScore"
           << "mzFragError"
           << "spearmanRankCorrelation";

    for (unsigned int i = 0; i < samples.size(); i++) { Header << samples[i]->sampleName.c_str(); }

    foreach(QString h, Header)  groupReport << h.toStdString() << SEP;
    groupReport << "\n";

    //Only used for the report
    quantitationType = PeakGroup::SmoothedArea;

    for (int i = 0; i < allgroups.size(); i++ ) {

        PeakGroup groupOriginal = allgroups[i];

        PeakGroup groupCopy = groupOriginal;
        groupCopy.children.clear();

        PeakGroup* group = &groupCopy;

        if (group->isGroupLabeled('q')) {
            writeGroupInfoCSV(group, groupReport);
        }
    }

    groupReport.close();
}

void writePeptideStabilityReport(string filename) {

    //TODO: customize this report
    //curently duplicating writeQQQReport()

    ofstream groupReport;
    groupReport.open(filename.c_str());
    if (! groupReport.is_open()) return;

    string SEP = csvFileFieldSeperator;

    QStringList Header;
    Header << "label" << "metaGroupId"
           << "groupId"
           << "goodPeakCount"
           << "medMz"
           << "medRt"
           << "maxQuality"
           << "note"
           << "compound"
           << "compoundId"
           << "category"
           << "database"
           << "expectedRtDiff"
           << "ppmDiff"
           << "parent"
           << "ms2EventCount"
           << "fragNumIonsMatched"
           << "fragFracMatched"
           << "ticMatched"
           << "weightedDotProduct"
           << "hypergeomScore"
           << "mzFragError"
           << "spearmanRankCorrelation";

    for (unsigned int i = 0; i < samples.size(); i++) { Header << samples[i]->sampleName.c_str(); }

    foreach(QString h, Header)  groupReport << h.toStdString() << SEP;
    groupReport << "\n";

    //Only used for the report
    quantitationType = PeakGroup::SmoothedArea;

    for (int i = 0; i < allgroups.size(); i++ ) {

        PeakGroup groupOriginal = allgroups[i];

        PeakGroup groupCopy = groupOriginal;
        groupCopy.children.clear();

        PeakGroup* group = &groupCopy;

        if (group->isGroupLabeled('q')) {
            writeGroupInfoCSV(group, groupReport);
        }
    }

    groupReport.close();
}

void writeCSVReport(string filename) {
    ofstream groupReport;
    groupReport.open(filename.c_str());
    if (! groupReport.is_open()) return;

    string SEP = csvFileFieldSeperator;

    QStringList Header;
    Header << "label" << "metaGroupId"
           << "groupId"
           << "goodPeakCount"
           << "medMz"
           << "medRt"
           << "maxQuality"
           << "note"
           << "compound"
           << "compoundId"
           << "category"
           << "database"
           << "expectedRtDiff"
           << "ppmDiff"
           << "parent"
           << "ms2EventCount"
           << "fragNumIonsMatched"
           << "fragFracMatched"
           << "ticMatched"
           << "weightedDotProduct"
           << "hypergeomScore"
           << "mzFragError"
           << "spearmanRankCorrelation";

    for (unsigned int i = 0; i < samples.size(); i++) { Header << samples[i]->sampleName.c_str(); }

    foreach(QString h, Header)  groupReport << h.toStdString() << SEP;
    groupReport << "\n";

    for (int i = 0; i < allgroups.size(); i++ ) {
        PeakGroup* group = &allgroups[i];
        writeGroupInfoCSV(group, groupReport);
    }

    groupReport.close();
}

Peak* getHighestIntensityPeak(PeakGroup* grp) {
    Peak* highestIntensityPeak = 0;
    for (int i = 0; i < grp->peaks.size(); i++ ) {
        if (!highestIntensityPeak or grp->peaks[i].peakIntensity > highestIntensityPeak->peakIntensity) {
            highestIntensityPeak = &grp->peaks[i];
        }
    }
    return highestIntensityPeak;
}

bool isC12Parent(Scan* scan, float monoIsotopeMz) {
    if (!scan) return 0;

    const double C13_DELTA_MASS = 13.0033548378 - 12.0;
    for (int charge = 1; charge < 4; charge++) {
        int peakPos = scan->findHighestIntensityPos(monoIsotopeMz - C13_DELTA_MASS / charge, precursorPPM);
        if (peakPos != -1) {
            return false;
        }
    }

    return true;
}

bool isMonoisotope(PeakGroup* grp) {
    Peak* highestIntensityPeak = getHighestIntensityPeak(grp);
    if (! highestIntensityPeak) return false;

    int scanum = highestIntensityPeak->scan;
    Scan* s = highestIntensityPeak->getSample()->getScan(scanum);
    return isC12Parent(s, highestIntensityPeak->peakMz);
}

int getChargeStateFromMS1(PeakGroup* grp) {
    Peak* highestIntensityPeak = getHighestIntensityPeak(grp);
    int scanum = highestIntensityPeak->scan;
    float peakMz = highestIntensityPeak->peakMz;
    Scan* s = highestIntensityPeak->getSample()->getScan(scanum);

    int peakPos = 0;
    if (s) peakPos = s->findHighestIntensityPos(highestIntensityPeak->peakMz, precursorPPM);

    if (s and peakPos > 0 and peakPos < s->nobs() ) {
        float ppm = (0.005 / peakMz) * 1e6;
        vector<int> parentPeaks = s->assignCharges(ppm);
        return parentPeaks[peakPos];
    } else {
        return 0;
    }
}

void classConsensusMS2Spectra(string filename) {
    ofstream outstream;
    outstream.open(filename.c_str());
    if (! outstream.is_open()) return;

    map<string, Fragment>compoundClasses;

    for (int i = 0; i < allgroups.size(); i++ ) {
        PeakGroup* group = &allgroups[i];
        if (group->compound == NULL) continue; //skip unassinged compounds
        if (group->fragmentationPattern.nobs() == 0) continue; //skip compounds without fragmentation

        vector<string>nameParts; split(group->compound->category[0], ';', nameParts);
        if (nameParts.size() == 0) continue; //skip unassinged compound classes
        string compoundClass = nameParts[0];

        Fragment f = group->fragmentationPattern;
        f.addNeutralLosses();
        if (compoundClasses.count(compoundClass) == 0) {
            compoundClasses[compoundClass] = f;
        } else {
            compoundClasses[compoundClass].addFragment(&f);
        }
    }

    for (auto pair : compoundClasses) {
        string compoundClass = pair.first;
        Fragment& f = pair.second;
        if (f.brothers.size() < 10) continue;
        f.buildConsensus(productPpmTolr);

        cout << "Writing " << compoundClass << endl;
        f.printConsensusNIST(outstream, 0.75, productPpmTolr, f.group->compound, f.group->adduct);
    }
    outstream.close();
}

void writeConsensusMS2Spectra(string filename) {
    cout << "Writing consensus specta to " << filename <<  endl;


    ofstream outstream;
    outstream.open(filename.c_str());
    if (! outstream.is_open()) return;

    for (int i = 0; i < allgroups.size(); i++ ) {
        //Scan* x = allgroups[i].getAverageFragmenationScan(100);

        PeakGroup* group = &allgroups[i];
        if (WRITE_CONSENSUS_MAPPED_COMPOUNDS_ONLY and !group->compound) continue;
        group->fragmentationPattern.rt = group->meanRt;

        if (group->fragmentationPattern.nobs() > 0 ) {
            group->fragmentationPattern.printConsensusNIST(outstream, 0.01, productPpmTolr, group->compound, group->adduct);
        }
    }
    outstream.close();
}

void writeGroupInfoCSV(PeakGroup* group,  ofstream& groupReport) {

    if (! groupReport.is_open()) return;
    string SEP = csvFileFieldSeperator;
    PeakGroup::QType qtype = quantitationType;

    vector<float> yvalues = group->getOrderedIntensityVector(samples, qtype);

    groupReport << group->getPeakGroupLabel() << SEP
                << setprecision(7)
                << group->metaGroupId << SEP
                << group->groupId << SEP
                << group->goodPeakCount << SEP
                << group->meanMz << SEP
                << group->meanRt << SEP
                << group->maxQuality << SEP
                << group->tagString;

    string compoundName;
    string compoundCategory;
    string compoundID;
    string database;
    float  expectedRtDiff = 1000;
    float  ppmDist = 1000;

    if (group->compound) {
        Compound* c = group->compound;
        ppmDist = group->fragMatchScore.ppmError;
        expectedRtDiff = c->expectedRt - group->meanRt;
        compoundName = c->name;
        compoundID =  c->id;
        database = c->db;
        if (c->category.size()) compoundCategory = c->category[0];
    }

    groupReport << SEP << mzUtils::doubleQuoteString(compoundName);
    groupReport << SEP << mzUtils::doubleQuoteString(compoundID);
    groupReport << SEP << mzUtils::doubleQuoteString(compoundCategory);
    groupReport << SEP << mzUtils::doubleQuoteString(database);
    groupReport << SEP << expectedRtDiff;
    groupReport << SEP << ppmDist;

    if (group->parent) {
        groupReport << SEP << group->parent->meanMz;
    } else {
        groupReport << SEP << group->meanMz;
    }

    groupReport << SEP << group->ms2EventCount;
    groupReport << SEP << group->fragMatchScore.numMatches;
    groupReport << SEP << group->fragMatchScore.fractionMatched;
    groupReport << SEP << group->fragMatchScore.ticMatched;
    groupReport << SEP << group->fragMatchScore.weightedDotProduct;
    groupReport << SEP << group->fragMatchScore.hypergeomScore;
    groupReport << SEP << group->fragMatchScore.mzFragError;
    groupReport << SEP << group->fragMatchScore.spearmanRankCorrelation;

    for ( unsigned int j = 0; j < samples.size(); j++) groupReport << SEP <<  yvalues[j];
    groupReport << "\n";

    for (unsigned int k = 0; k < group->children.size(); k++) {
        writeGroupInfoCSV(&group->children[k], groupReport);
        //writePeakInfo(&group->children[k]);
    }
}

double get_wall_time() {
    struct timeval time;
    if (gettimeofday(&time, NULL)) {
        //  Handle error
        return 0;
    }
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

double get_cpu_time() {
    return (double)getTime() / CLOCKS_PER_SEC;
}

string possibleClasses(set<Compound*>matches) {
    map<string, int>names;
    for (Compound* cpd : matches) {
        if (cpd->category.size() == 0) continue;
        vector<string>nameParts;
        split(cpd->category[0], ';', nameParts);
        if (nameParts.size() > 0) names[nameParts[0]]++;
    }

    string classes;
    for (auto p : names) {
        classes += p.first + "[matched=" + integer2string(p.second) + " ] ";
    }
    return classes;
}

void matchFragmentation() {
    if (DB.compoundsDB.size() == 0) return;

    for (int i = 0; i < allgroups.size(); i++) {
        PeakGroup* g = &allgroups[i];
        if (g->ms2EventCount == 0) continue;

        //set<Compound*> matches = findSpeciesByMass(g->meanMz,precursorPPM);
        vector<MassCalculator::Match>matchesX = DB.findMatchingCompounds(g->meanMz, precursorPPM, ionizationMode);
        //vector<MassCalculator::Match>matchesS = DB.findMathchingCompoundsSLOW(g->meanMz,precursorPPM,ionizationMode);

        if (matchesX.size() == 0) continue;

        //g->fragmentationPattern.printFragment(productPpmTolr,10);

        Adduct* adduct = 0;
        Compound* bestHit = 0;
        FragmentationMatchScore bestScore;

        for (MassCalculator::Match m : matchesX ) {
            Compound* cpd = m.compoundLink;
            FragmentationMatchScore s = cpd->scoreCompoundHit(&g->fragmentationPattern, productPpmTolr, true);

            if (scoringScheme == "weightedDotProduct") {
                s.mergedScore = s.weightedDotProduct;
            } else {
                s.mergedScore = s.hypergeomScore;

            }
            s.ppmError = m.diff;

            if (s.numMatches == 0 ) continue;
            //cout << "\t"; cout << cpd->name << "\t" << cpd->precursorMz << "\tppmDiff=" << m.diff << "\t num=" << s.numMatches << "\t tic" << s.ticMatched <<  " s=" << s.spearmanRankCorrelation << endl;
            if (s.mergedScore > bestScore.mergedScore ) {
                bestScore = s;
                bestHit = cpd;
                adduct =  m.adductLink;
            }
        }

        if (bestHit) {
            g->compound = bestHit;
            g->adduct  = adduct;
            if (adduct) g->tagString = adduct->name;
            g->fragMatchScore = bestScore;

            cout << "\t BESTHIT"
                 << setprecision(6)
                 << g->meanMz << "\t"
                 << g->meanRt << "\t"
                 << g->maxIntensity << "\t"
                 << bestHit->name << "\t"
                 << bestScore.numMatches << "\t"
                 << bestScore.fractionMatched << "\t"
                 << bestScore.ticMatched << endl;

        }
    }
}

void loadSpectralLibraries(string methodsFolder) {
    QDir dir(methodsFolder.c_str());
    if (dir.exists()) {
        dir.setFilter(QDir::Files );
        QFileInfoList list = dir.entryInfoList();
        for (int i = 0; i < list.size(); ++i) {
            QFileInfo fileInfo = list.at(i);
            QString fileName = fileInfo.absoluteFilePath();

            vector<Compound*>compounds;
            if ( fileName.endsWith("msp", Qt::CaseInsensitive) || fileName.endsWith("sptxt", Qt::CaseInsensitive)) {
                compounds = DB.loadNISTLibrary(fileName);
            } else if ( fileName.endsWith("txt", Qt::CaseInsensitive)
                        || fileName.endsWith("csv", Qt::CaseInsensitive)
                        || fileName.endsWith("tsv", Qt::CaseInsensitive)) {
            }
            DB.compoundsDB.insert( DB.compoundsDB.end(), compounds.begin(), compounds.end());
        }
    }
    sort(DB.compoundsDB.begin(), DB.compoundsDB.end(), Compound::compMass);
}

/**
 * @brief standardsBasedAlignment
 * MS Issue 317
 *
 * Allow for more extreme RT swings by using standards as anchor points
 *
 * Issue 659: Refactor to use ExperimentAnchorPoints class.
 */
void anchorPointsBasedAlignment() {

    double startLoadingTime = getTime();

    ExperimentAnchorPoints experimentAnchorPoints = ExperimentAnchorPoints(
                samples,
                anchorPointsFile,
                standardsAlignment_precursorPPM,
                standardsAlignment_maxRtWindow,
                eic_smoothingWindow,
                standardsAlignment_minPeakIntensity,
                rtAlignmentAnchorPointReference
                );
    experimentAnchorPoints.compute(true, true); // (debug, isClean)
    sampleToUpdatedRts = experimentAnchorPoints.sampleToUpdatedRts;

    printf("Execution time (anchorPointsBasedAlignment()) : %f seconds \n", getTime() - startLoadingTime);
}

//Issue 739: Handle different types of mzkitchen-driven msp searches.
void mzkitchenSearch() {

    cout << "Performing mzkitchen msp search on identified peak groups." << endl;
    cout << setprecision(10);

    vector<Compound*> mzkitchenCompounds = DB.loadNISTLibrary(mzkitchenMspFile.c_str());

    sort(mzkitchenCompounds.begin(), mzkitchenCompounds.end(), [](const Compound* lhs, const Compound* rhs){
        if (lhs->precursorMz != rhs->precursorMz) {
            return lhs->precursorMz < rhs->precursorMz;
        }

        if (lhs->name != rhs->name){
            return lhs->name < rhs->name;
        }

        return lhs->adductString < rhs->adductString;
    });

    cout << "MSP spectral library \'" << mzkitchenMspFile << "\' contains " << mzkitchenCompounds.size() << " compounds." << endl;

    if (mzkitchenSearchType == "lipidSearch") {

        cout << "MzKitchenProcessor::matchLipids_LC() --> MzKitchenProcessor::assignBestLipidToGroup() for "
             << allgroups.size()
             << " lipids."
             << endl;

        MzKitchenProcessor::matchLipids_LC(allgroups, mzkitchenCompounds, lipidSearchParams, false);
        searchTableData.insert(make_pair("clamDB", QString(lipidSearchParams->encodeParams().c_str())));

    } else if (mzkitchenSearchType == "metaboliteSearch") {

        cout << "MzKitchenProcessor::matchMetabolites() --> MzKitchenProcessor::assignBestMetaboliteToGroup() for "
             << allgroups.size()
             << " metabolites."
             << endl;

        MzKitchenProcessor::matchMetabolites(allgroups, mzkitchenCompounds, metaboliteSearchParams, false);
        searchTableData.insert(make_pair("clamDB", QString(metaboliteSearchParams->encodeParams().c_str())));

    } else {
        cerr << "Unknown mzkitchenSearchType: '" << mzkitchenSearchType << "'. Exiting in error." << endl;
        abort();
    }

    if (isExtractIsotopes) {
        cout << "Extracting isotopes for peak groups." << endl;
    } else {
        cout << "Isotopes will not be extracted for peak groups." << endl;
    }

    unsigned int identifiedGroups = 0;
    for (auto& group : allgroups) {

        //Issue 751: If a compound matches, try to match the corresponding adduct (if loaded)
        //Note that this approach is OK b/c compounds are directly loaded from library files,
        //i.e. the associated adduct from the library is always used
        //(do not search existing library data against alternative adduct forms).
        if (group.compound) {
            group.searchTableName = "clamDB";
            identifiedGroups++;
            if (loadedAdducts.find(group.compound->adductString) != loadedAdducts.end()) {
                Adduct *adduct = loadedAdducts.at(group.compound->adductString);
                group.adduct = adduct;
            }
        }

        //Issue 751: Isotopes option (only for untargeted data)
        if (isExtractIsotopes) {
            group.pullIsotopes(isotopeParameters,
                               samples,
                               false //debug
                               );
            group.isotopeParameters = isotopeParameters;
        }
    }

    cout << "Identified " << identifiedGroups << " compounds in " << allgroups.size() << " peak groups." << endl;
}

bool isSpecialSearch() {
    return (isMzkitchenSearch || isQQQSearch || isPeptideStabilitySearch);
}

void traverseAndAdd(PeakGroup& group, set<Compound*>& compoundSet) {
    if (group.compound) compoundSet.insert(group.compound);
    for (auto& child : group.children) {
       traverseAndAdd(child, compoundSet);
    }
}

// Issue 815: Support compound-based approach
vector<mzSlice*> compoundLibrarySlices(string compoundLibraryFile, bool debug) {

    //initialize output
    vector<mzSlice*> slices{};

    QString fileName = QString(compoundLibraryFile.c_str());

    // [1] Generate compounds and compound ions from compoundLibraryFile
    vector<Compound*> compounds{};
    vector<CompoundIon*> compoundIons{};

    if ( fileName.endsWith("msp", Qt::CaseInsensitive) || fileName.endsWith("sptxt", Qt::CaseInsensitive)) {
        compounds = Database::loadNISTLibrary(fileName);
    } else if ( fileName.endsWith("csv", Qt::CaseInsensitive)) {
        compounds = Database::loadCompoundCSVFile(fileName, debug);
    } else {
        //not actually a compound library - single compound in encoded string
        compounds = CompoundUtils::fromExactMassAdducts(compoundLibraryFile, debug);
    }

    for (Compound* compound : compounds) {

        string adductString = compound->adductString;

        //skip over any compounds where adduct is not specified
        if (adductString.empty()) {
            continue;
        }

        Adduct *adduct = nullptr;
        if (loadedAdducts.find(adductString) == loadedAdducts.end()) {
            Adduct val = MassCalculator::parseAdductFromName(adductString);

            adduct = new Adduct();
            adduct->name = val.name;
            adduct->mass = val.mass;
            adduct->charge = val.charge;
            adduct->nmol = val.nmol;

            loadedAdducts.insert(make_pair(adductString, adduct));
        }

        adduct = loadedAdducts.at(adductString);

        CompoundIon *compoundIon = new CompoundIon(compound, adduct);

        compoundIons.push_back(compoundIon);
    }

    // [2] convert compound Ions to slices
    for (CompoundIon *compoundIon : compoundIons) {

        //by default, slices will span the entire gradient
        float rtmin = 0.0f;
        float rtmax = 1e38f;

        bool isHasExpectedRt = compoundIon->compound->expectedRt > 0;
        bool isHasRtRange = compoundIon->compound->expectedRtMin >= 0 && compoundIon->compound->expectedRtMax >= 0 && compoundIon->compound->expectedRtMax > compoundIon->compound->expectedRtMin;

        //only bind RT constraints when the compound has some RT information associated with it.
        if (isHasRtRange) {
            rtmin = compoundIon->compound->expectedRtMin;
            rtmax = compoundIon->compound->expectedRtMax;
        } else if (isHasExpectedRt){
            rtmin = compoundIon->compound->expectedRt - rtWindow;
            rtmax = compoundIon->compound->expectedRt + rtWindow;
        }

        float mzmin = compoundIon->compound->precursorMz - precursorPPM * compoundIon->compound->precursorMz / 1e6;
        float mzmax = compoundIon->compound->precursorMz + precursorPPM * compoundIon->compound->precursorMz / 1e6;

        mzSlice *slice = new mzSlice(mzmin, mzmax, rtmin, rtmax);
        slice->compound = compoundIon->compound;
        slice->adduct = compoundIon->adduct;
        slice->compoundVector = vector<Compound*>{};
        slice->compoundVector.push_back(compoundIon->compound);

        slices.push_back(slice);
    }

    return slices;
}
