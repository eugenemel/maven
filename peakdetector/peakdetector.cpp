
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <limits.h>
#include<sstream>

#include "mzSample.h"
#include "parallelMassSlicer.h"
#include "../mzroll/projectDB.h"
#include "options.h"

#include "../mzroll/classifierNeuralNet.h"
#include "../mzroll/database.h"
#include "./Fragment.h"
#include "pugixml.hpp"
#include <sys/time.h>

#include <QtCore>
#include <QCoreApplication>

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
int modVersion =   2;

Database DB;
QSqlDatabase projectDB;
ClassifierNeuralNet clsf;
vector <mzSample*> samples;
vector <Compound*> compoundsDB;
vector<PeakGroup> allgroups;
MassCalculator mcalc;

time_t startTime, curTime, stopTime;

vector<string>filenames;

bool alignSamplesFlag=true;
bool processAllSlices=true;
bool reduceGroupsFlag=true;
bool matchRtFlag = true;
bool writeReportFlag=true;
bool saveJsonEIC=false;
bool mustHaveMS2=false;
bool writeConsensusMS2=false;
bool pullIsotopesFlag=true;
bool samplesC13Labeled=true;
bool saveSqlLiteProject=true;
bool writePDFReport=false;
bool searchAdductsFlag=true;

string csvFileFieldSeperator="\t";
PeakGroup::QType quantitationType = PeakGroup::AreaTop;

string ligandDbFilename;
string methodsFolder = "./methods";
string fragmentsDbFilename = "FRAGMENTS.csv";
string adductDbFilename =  "ADDUCTS.csv";
string clsfModelFilename = "default.model";
string outputdir = "reports" + string(DIR_SEPARATOR_STR);
string nameSuffix = "peakdetector";

int  ionizationMode = -1;

//mass slice detection
int   rtStepSize=10;
float precursorPPM=5;
float avgScanTime=0.2;

//peak detection
int eic_smoothingWindow = 5;

//peak grouping across samples
float grouping_maxRtWindow=0.5;

//peak filtering criteria
int minGoodGroupCount=1;
float minSignalBlankRatio=5;
int minNoNoiseObs=5;
float minSignalBaseLineRatio=5;
float minGroupIntensity=1000;
float minQuality = 0.5;
int minPrecursorCharge=0;

//size of retention time window
float rtWindow=2;
int  eicMaxGroups=100;

//MS2 matching
float minFragmentMatchScore=2;
float productPpmTolr = 20;
string scoringScheme="ticMatch"; // ticMatch | spearmanRank

void processOptions(int argc, char* argv[]);
void loadSamples(vector<string>&filenames);
void printSettings();
void alignSamples();
void processSlices(vector<mzSlice*>&slices,string setName);
void processCompounds(vector<Compound*> set, string setName);
void processMassSlices(int limit=0);
void reduceGroups();
void writeReport(string setName);
void writeCSVReport(string filename);
void writeConsensusMS2Spectra(string filename);
void classConsensusMS2Spectra(string filename);
void pullIsotopes(PeakGroup* parentgroup);
void writeGroupInfoCSV(PeakGroup* group,  ofstream& groupReport);
void loadSpectralLibraries(string methodsFolder);

bool addPeakGroup(PeakGroup& grp);

int getChargeStateFromMS1(PeakGroup* grp);
bool isC12Parent(Scan* scan, float monoIsotopeMz);
bool isMonoisotope(PeakGroup* grp);

vector<mzSlice*> getSrmSlices();

void matchFragmentation();
void writeMS2SimilarityMatrix(string filename);

vector<EIC*> getEICs(float rtmin, float rtmax, PeakGroup& grp);

double get_wall_time();
double get_cpu_time(); 

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    double programStartTime = getTime();
	//read command line options
	processOptions(argc,argv);

    cerr << "peakdetector ver=" << majorVersion << "." << minorVersion << "." << modVersion << endl;

	//load classification model
    cerr << "Loading classifiation model: " << methodsFolder+ "/"+clsfModelFilename << endl;
    clsfModelFilename = methodsFolder + "/" + clsfModelFilename;

    if (fileExists(clsfModelFilename)) {
        clsf.loadModel(clsfModelFilename);
    } else {
        cerr << "Can't find peak classificaiton model:" << clsfModelFilename << endl;
        exit(-1);
    }

    loadSpectralLibraries(methodsFolder);

    if (DB.compoundsDB.size() and searchAdductsFlag) {
        //processCompounds(compoundsDB, "compounds");
        DB.adductsDB = DB.loadAdducts(methodsFolder + "/" + adductDbFilename);
    }

    //load files
    double startLoadingTime = getTime();
    loadSamples(filenames);
    printf("Execution time (Sample loading) : %f seconds \n", getTime() - startLoadingTime);
	if (samples.size() == 0 ) { cerr << "Exiting .. nothing to process " << endl;  exit(1); }

    //get retenation time resolution
	avgScanTime = samples[0]->getAverageFullScanTime();

    //ionization
    if(samples.size() > 0 ) ionizationMode = samples[0]->getPolarity();

    printSettings();

    //align samples
    if (samples.size() > 1 && alignSamplesFlag) { alignSamples(); }
    //procall all mass slices
    if (processAllSlices == true) {
        matchRtFlag = false;
        processMassSlices();
    }

    //cleanup
    delete_all(samples);
    samples.clear();
    allgroups.clear();

    printf("Total program execution time : %f seconds \n",getTime() - programStartTime);
	return(0);
}

void alignSamples() { 
	cerr << "Aligning samples" << endl;
    bool _tmpMustHaveMS2=mustHaveMS2; mustHaveMS2=false;

    ParallelMassSlicer massSlices;
    massSlices.setSamples(samples);
    massSlices.algorithmB(10.0,1e6,5); //ppm, minIntensity, rtWindow in Scans
    sort(massSlices.slices.begin(), massSlices.slices.end(), mzSlice::compIntensity);
    processSlices(massSlices.slices,"alignment");

    cerr << "Aligning: " << allgroups.size() << endl;
    vector<PeakGroup*>agroups(allgroups.size());
    sort(allgroups.begin(),allgroups.end(),PeakGroup::compRt);

    for(unsigned int i=0; i < allgroups.size();i++) {
        agroups[i]= &allgroups[i];
        /*
        cerr << "Alignments: " << setprecision(5)
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
    mustHaveMS2=_tmpMustHaveMS2;
}

void processCompounds(vector<Compound*> set, string setName) { 
		cerr << "processCompounds: " << setName << endl;
		if (set.size() == 0 ) return;
		vector<mzSlice*>slices;
		for (unsigned int i=0; i < set.size();  i++ ) {
				Compound* c = set[i];
				mzSlice* slice = new mzSlice();
				slice->compound = c;

				double mass =c->mass;
				if (!c->formula.empty()) {
					float formula_mass =  mcalc.computeMass(c->formula,ionizationMode);
					if(formula_mass) mass=formula_mass;
				}

                slice->mzmin = mass - precursorPPM * mass/1e6;
                slice->mzmax = mass + precursorPPM * mass/1e6;
                //cerr << c->name << " " << slice->mzmin << " " << slice->mzmax << endl;

				slice->rtmin  = 0;
				slice->rtmax =  1e9;
				slice->ionCount = FLT_MAX;
				if ( matchRtFlag==true && c->expectedRt > 0 ) { 
                        slice->rtmin = c->expectedRt-rtWindow;
                        slice->rtmax = c->expectedRt+rtWindow;
				}
				slices.push_back(slice);
		}

		reduceGroupsFlag=false;
		processSlices(slices,setName);
		delete_all(slices);
}

vector<mzSlice*> getSrmSlices() {
	map<string,int>uniqSrms;
	vector<mzSlice*>slices;
	for(int i=0; i < samples.size(); i++) {
			for(int j=0; j < samples[i]->scans.size(); j++ ) {
				Scan* scan = samples[i]->scans[j];
				string filterLine = scan->filterLine;
				if ( uniqSrms.count(filterLine) == 0 ) {
						uniqSrms[filterLine]=1;
						slices.push_back( new mzSlice(filterLine) );
				}
			}
	}
	cerr << "getSrmSlices() " << slices.size() << endl;
	return slices;
}

void processMassSlices(int limit) { 

		cerr << "Process Mass Slices" << endl;
		if ( samples.size() > 0 ) avgScanTime = samples[0]->getAverageFullScanTime();

		ParallelMassSlicer  massSlices;
        massSlices.setSamples(samples);
		if(limit>0) massSlices.setMaxSlices(limit);
        double functionAlgorithmBTime = getTime();

        if (mustHaveMS2) {
            massSlices.algorithmD(precursorPPM,rtStepSize);
            printf("Execution time (AlgorithmD) : %f seconds \n",((float)getTime() - (float)functionAlgorithmBTime) );
        } else {
            massSlices.algorithmB(precursorPPM,minGroupIntensity,rtStepSize);
            printf("Execution time (AlgorithmB) : %f seconds \n",((float)getTime() - (float)functionAlgorithmBTime) );
        }

		sort(massSlices.slices.begin(), massSlices.slices.end(), mzSlice::compIntensity);

		vector<mzSlice*>goodslices;

		if (limit > 0) {
			goodslices.resize(limit);
			for(int i=0; i< massSlices.slices.size() && i<limit; i++ ) goodslices[i] = massSlices.slices[i];
		} else {
			goodslices.resize(massSlices.slices.size());
			for(int i=0; i< massSlices.slices.size(); i++ ) goodslices[i] = massSlices.slices[i];
		}

		if (massSlices.slices.size() == 0 ) { 
			goodslices = getSrmSlices(); 
		}

		if (goodslices.size() == 0 ) {
			cerr << "Quting! No mass slices found" << endl;
			return;
		}

        if (reduceGroupsFlag) reduceGroups();
		string setName = "allslices";
        double functionStartTime = getTime();
		processSlices(goodslices,nameSuffix);
        printf("Execution time (processSlices) : %f seconds \n",((float)getTime() - (float)functionStartTime) );
		delete_all(massSlices.slices); 
		massSlices.slices.clear();
		goodslices.clear();
}

void processSlices(vector<mzSlice*>&slices, string setName) { 
    if (slices.size() == 0 ) return;
    allgroups.clear();

    printSettings();
    //process KNOWNS

    startTime = time(NULL);
    int eicCounter=0;

    double EICPullTime = 0.0;
    double peakGroupingTime = 0.0;
        double midPhaseTime = 0.0;
        double convergenceTime = 0.0;
        double endPhaseTime = 0.0;

        double startForLoopTime = getTime();

//Creating openmp threads and static scheduling with chunk of size 128
#ifdef OMP_PARALLEL
#pragma omp parallel for ordered num_threads(8) private(eicCounter) schedule(static,128)
#endif
		for (int i=0; i < slices.size();  i++ ) {
                if (i % 1000==0)
                cerr <<setprecision(2) << "Processing slices: (TID "<< omp_get_thread_num() << ") " <<  i /(float) slices.size()*100 << "% groups=" << allgroups.size() << endl;

				mzSlice* slice = slices[i];
				double mzmin = slices[i]->mzmin;
				double mzmax = slices[i]->mzmax;
				double rtmin = slices[i]->rtmin;
				double rtmax = slices[i]->rtmax;
				//if ( mzmin < 125.0  || mzmax > 125.2 ) continue;
                Compound* compound = slices[i]->compound;

                //cerr << "\tEIC Slice\t" << setprecision(5) << mzmin <<  "-" << mzmax << " @ " << rtmin << "-" << rtmax << " I=" << slices[i]->ionCount;

				//PULL EICS
                double startEICPullTime = getTime();
				vector<EIC*>eics; float eicMaxIntensity=0;
		  		for ( unsigned int j=0; j < samples.size(); j++ ) {
					EIC* eic;
					if ( !slice->srmId.empty() ) {
						eic = samples[j]->getEIC(slice->srmId);
					} else {
						eic = samples[j]->getEIC(mzmin,mzmax,rtmin,rtmax,1);
					}
					eics.push_back(eic);
					eicCounter++;
					if ( eic->maxIntensity > eicMaxIntensity) eicMaxIntensity=eic->maxIntensity;
				}
                EICPullTime += getTime() - startEICPullTime;

                double startMidPhaseTime = getTime();
				//optimization.. skip over eics with low intensities
				if ( eicMaxIntensity < minGroupIntensity)  {  delete_all(eics); continue; } 

				//compute peak positions
				for (unsigned int i=0; i<eics.size(); i++ ) {
					eics[i]->getPeakPositions(eic_smoothingWindow);
				}

                vector <PeakGroup> peakgroups =
				EIC::groupPeaks(eics,eic_smoothingWindow,grouping_maxRtWindow);

                //sort peaks by intensity
                sort(peakgroups.begin(), peakgroups.end(), PeakGroup::compIntensity);
                midPhaseTime += getTime() - startMidPhaseTime;

                double startPeakGroupingTime = getTime();
                vector<PeakGroup*> groupsToAppend;
        for(unsigned int j=0; j < peakgroups.size(); j++ ) {
            PeakGroup& group = peakgroups[j];

            if ( compound ) {
                group.compound = compound;
                //group.tagString = compound->name;
            }

            if( clsf.hasModel()) { clsf.classify(&group); group.groupStatistics(); }
            if (clsf.hasModel() && group.goodPeakCount < minGoodGroupCount) continue;
            if (clsf.hasModel() && group.maxQuality < minQuality) continue;

            if (group.maxNoNoiseObs < minNoNoiseObs) continue;
            if (group.maxSignalBaselineRatio < minSignalBaseLineRatio) continue;
            if (group.maxIntensity < minGroupIntensity ) continue;
            if (getChargeStateFromMS1(&group) < minPrecursorCharge) continue;

            //build consensus ms2 specta
            vector<Scan*>ms2events = group.getFragmenationEvents();
            if(ms2events.size()) {
                sort(ms2events.begin(),ms2events.end(),Scan::compIntensity);
                Fragment f = Fragment(ms2events[0],0.01,1,1024);
                for(Scan* s : ms2events) {  f.addFragment(new Fragment(s,0,0.01,1024)); }
                f.buildConsensus(productPpmTolr);
                group.fragmentationPattern = f.consensus;
                group.ms2EventCount = ms2events.size();
            }

            if (mustHaveMS2 and group.ms2EventCount == 0 ) continue;
            if (compound) group.compound = compound;
            if (!slice->srmId.empty()) group.srmId=slice->srmId;

            if(compound && pullIsotopesFlag && !group.isIsotope()) {
                pullIsotopes(&group);
            }

            group.groupRank = 1000;
            if (matchRtFlag && compound != NULL && compound->expectedRt>0) {
                float rtDiff =  abs(compound->expectedRt - (group.meanRt));
                group.expectedRtDiff = rtDiff;
                group.groupRank = rtDiff*rtDiff*(1.1-group.maxQuality)*(1/log(group.maxIntensity+1));
                if (group.expectedRtDiff > rtWindow ) continue;
            } else {
                group.groupRank = (1.1-group.maxQuality)*(1/log(group.maxIntensity+1));
            }

            groupsToAppend.push_back(&group);
            //addPeakGroup(group);
        }

        int appendCount=0;
        std::sort(groupsToAppend.begin(), groupsToAppend.end(),PeakGroup::compRankPtr);
        for(int j=0; j<groupsToAppend.size(); j++) {
            if(appendCount >= eicMaxGroups) break;
            PeakGroup* group = groupsToAppend[j];

            bool ok = addPeakGroup(*group);
            if (ok) {
                appendCount++;
            } else if (!ok and compound != NULL ) { //force insert when processing compounds.. even if duplicated
                appendCount++;
                allgroups.push_back(*group);
            }
        }
        peakGroupingTime += getTime() - startPeakGroupingTime;
        double startEndPhaseTime = getTime();
        delete_all(eics);
        eics.clear();
        endPhaseTime += getTime() - startEndPhaseTime;
    }



    printf("Execution time (for loop)     : %f seconds \n", getTime()- startForLoopTime);
    printf("\tExecution time (EICPulling)      : %f seconds \n", EICPullTime);
    printf("\tExecution time (ConvergenceTime) : %f seconds \n", convergenceTime);
    printf("\tExecution time (MidPhaseTime)    : %f seconds \n", midPhaseTime);
    printf("\tExecution time (PeakGrouping)    : %f seconds \n", peakGroupingTime);
    printf("\tExecution time (EndPhaseTime)    : %f seconds \n", endPhaseTime);
#ifdef OMP_PARALLEL
    printf("\tParallelisation (PeakGrouping)   : True\n");
#endif

    double startWriteReportTime = getTime();
    writeReport(setName);
    printf("Execution time (Write report) : %f seconds \n",getTime() - startWriteReportTime);
}

vector<EIC*> getEICs(float rtmin, float rtmax, PeakGroup& grp) { 
		vector<EIC*>eics;
		for(int i=0; i < grp.peaks.size(); i++ ) {
				float mzmin  = grp.meanMz-0.2;
				float mzmax  = grp.meanMz+0.2;
				//cerr <<setprecision(5) << "getEICs: mz:" << mzmin << "-" << mzmax << " rt:" << rtmin << "-" << rtmax << endl;

		  		for ( unsigned int j=0; j < samples.size(); j++ ) {
					if ( !grp.srmId.empty() ) {
						EIC* eic = samples[j]->getEIC(grp.srmId);
						eics.push_back(eic);
					} else {
						EIC* eic = samples[j]->getEIC(mzmin,mzmax,rtmin,rtmax,1);
						eics.push_back(eic);
					}
				}
		}
		return(eics);
}

void processOptions(int argc, char* argv[]) {

		//command line options
        const char * optv[] = {
                                "a?alignSamples",
                                "b?minGoodGroupCount <int>",
                                "c?matchRtFlag <int>",
                                "d?db <sting>",
                                "e?processAllSlices",
                                "g?grouping_maxRtWindow <float>",
                                "h?help",
                                "i?minGroupIntensity <float>",
                                "k?productPpmTolr <float>",
                                "m?methodsFolder <string>",
                                "n?eicMaxGroups <int>",
                                "o?outputdir <string>",
                                "p?ppmMerge <float>",
                                "q?minQuality <float>",
                                "r?rtStepSize <float>",
                                "s?nameSuffix <string>",
                                "t?scoringScheme <string>",
                                "v?ver",
                                "w?minPeakWidth <int>",
                                "x?minPrecursorCharge <int>",
                                "y?eicSmoothingWindow <int>",
                                "z?minSignalBaseLineRatio <float>",
                                "2?mustHaveMS2",
				NULL
		};

		//parse input options
		Options  opts(*argv, optv);
		OptArgvIter  iter(--argc, ++argv);
		const char * optarg;

        while( const char optchar = opts(iter, optarg) ) {
            switch (optchar) {
            case 'a' : alignSamplesFlag=atoi(optarg); break;
            case 'b' : minGoodGroupCount=atoi(optarg); break;
            case 'c' : rtWindow=atof(optarg); matchRtFlag=true; if(rtWindow==0) matchRtFlag=false; break;
            case 'd' : ligandDbFilename = optarg; break;
            case 'e' : processAllSlices=true; break;
            case 'g' : grouping_maxRtWindow=atof(optarg); break;
            case 'h' : opts.usage(cerr, "files ..."); exit(0); break;
            case 'i' : minGroupIntensity=atof(optarg); break;
            case 'k' : productPpmTolr=atof(optarg); break;
            case 'm' : methodsFolder = optarg; break;
            case 'n' : eicMaxGroups=atoi(optarg); break;
            case 'o' : outputdir = optarg + string(DIR_SEPARATOR_STR); break;
            case 'p' : precursorPPM=atof(optarg); break;
            case 'q' : minQuality=atof(optarg); break;
            case 'r' : rtStepSize=atof(optarg); break;
            case 's' : nameSuffix=string(optarg); break;
            case 't' : scoringScheme=string(optarg); break;
            case 'w' : minNoNoiseObs=atoi(optarg); break;
            case 'x' : minPrecursorCharge=atoi(optarg); break;
            case 'y' : eic_smoothingWindow=atoi(optarg); break;
            case 'z':  minSignalBaseLineRatio=atof(optarg); break;
            case '2':  mustHaveMS2=true; writeConsensusMS2=true; break;
            default : break;
            }
        }


        cerr << "#Command:\t";
		for (int i=0 ; i<argc ;i++)  cerr << argv[i] << " "; 
		cerr << endl;

       	if (iter.index() < argc) {
          for (int i = iter.index() ; i < argc ; i++) filenames.push_back(argv[i]); 
        }

}

void printSettings() {
        cerr << "#Ligand Database file\t" << ligandDbFilename << endl;
        cerr << "#Classification Model\t" << clsfModelFilename << endl;

        cerr << "#Output folder=" <<  outputdir << endl;
        cerr << "#ionizationMode=" << ionizationMode << endl;
        
        cerr << "#rtStepSize=" << rtStepSize << endl;
        cerr << "#ppmMerge="  << precursorPPM << endl;
        cerr << "#avgScanTime=" << avgScanTime << endl;

        //peak detection
        cerr << "#eic_smoothingWindow=" << eic_smoothingWindow << endl;

        //peak grouping across samples
        cerr << "#grouping_maxRtWindow=" << grouping_maxRtWindow << endl;

        //peak filtering criteria
        cerr << "#minGoodGroupCount=" << minGoodGroupCount << endl;
        cerr << "#minSignalBlankRatio=" << minSignalBlankRatio << endl;
        cerr << "#minPeakWidth=" << minNoNoiseObs << endl;
        cerr << "#minSignalBaseLineRatio=" << minSignalBaseLineRatio << endl;
        cerr << "#minGroupIntensity=" << minGroupIntensity << endl;


    for (unsigned int i=0;i <filenames.size(); i++) cerr << "#Sample" << i << " :" << filenames[i] << endl;
	cerr << endl;
}

void loadSamples(vector<string>&filenames) {
		cerr << "Loading samples" << endl;
        	int sampleOrder=0;

#ifdef OMP_PARALLEL
		#pragma omp parallel for ordered num_threads(4) schedule(static)  
#endif
		for (unsigned int i=0; i < filenames.size(); i++ ) {
//          cout << "Thread # " << omp_get_thread_num() << endl;
			cerr << "Loading " << filenames[i] << endl;
			mzSample* sample = new mzSample();
            sample->loadSample(filenames[i].c_str());

            if ( sample->scans.size() >= 1 ) {
                    sample->setSampleOrder(sampleOrder++);
					samples.push_back(sample);
					sample->summary();
				} else {
				if(sample != NULL) { 
					delete sample; 
					sample = NULL; 
				}
			}
		}
		cerr << "loadSamples done: loaded " << samples.size() << " samples\n";
}

bool addPeakGroup(PeakGroup& grp1) { 


	for(unsigned int i=0; i<allgroups.size(); i++) {
		PeakGroup& grp2 = allgroups[i];
		float rtoverlap = mzUtils::checkOverlap(grp1.minRt, grp1.maxRt, grp2.minRt, grp2.maxRt );
        if ( rtoverlap > 0.9 && ppmDist(grp1.meanMz, grp2.meanMz) < precursorPPM &&
						grp1.maxIntensity < grp2.maxIntensity) {
			return false;
		}
	}

	//cerr << "\t\t accepting " << grp1.meanMz << "@" << grp1.meanRt;
#ifdef OMP_PARALLEL
                    #pragma omp critical
#endif
	allgroups.push_back(grp1); 
#ifdef OMP_PARALLEL
                    #pragma omp end critical
#endif
	return true;
}

void reduceGroups() {
	sort(allgroups.begin(),allgroups.end(), PeakGroup::compMz);
	cerr << "reduceGroups(): " << allgroups.size();

	//init deleteFlag 
	for(unsigned int i=0; i<allgroups.size(); i++) {
			allgroups[i].deletedFlag=false;
	}

	for(unsigned int i=0; i<allgroups.size(); i++) {
		PeakGroup& grp1 = allgroups[i];
        if(grp1.deletedFlag) continue;
		for(unsigned int j=i+1; j<allgroups.size(); j++) {
			PeakGroup& grp2 = allgroups[j];
            if( grp2.deletedFlag) continue;

			float rtoverlap = mzUtils::checkOverlap(grp1.minRt, grp1.maxRt, grp2.minRt, grp2.maxRt );
			float ppmdist = ppmDist(grp2.meanMz, grp1.meanMz);
            if ( ppmdist > precursorPPM ) break;

            if ( rtoverlap > 0.8 && ppmdist < precursorPPM) {
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
    for(int i=0; i <allgroups.size(); i++)
    {
        PeakGroup& grp1 = allgroups[i];
        if(grp1.deletedFlag == false)
        {
            allgroups_.push_back(grp1);
            reducedGroupCount++;
        }
    }
    printf("Reduced count of groups : %d \n",  reducedGroupCount);
    allgroups = allgroups_;
	cerr << " done final group count(): " << allgroups.size() << endl;
}

void reNumberGroups() {

    int groupNum=1;
    int metaGroupNum=0;
    for(PeakGroup &g : allgroups) {
        g.groupId=groupNum++;
        g.metaGroupId = metaGroupNum;
        for(auto &c : g.children) {
            c.groupId= groupNum++;
            g.metaGroupId = metaGroupNum;
        }
    }
}

void writeReport(string setName) {
    if (writeReportFlag == false ) return;

    printf("\nProfiling writeReport...\n");

    reNumberGroups();
    matchFragmentation();

    //run clustering
    double maxRtDiff = 0.2;
    double minSampleCorrelation= 0.8;
    double minPeakShapeCorrelation=0.9;

    PeakGroup::clusterGroups(allgroups,samples,maxRtDiff,minSampleCorrelation,minPeakShapeCorrelation,precursorPPM);

    //create an output folder
    mzUtils::createDir(outputdir.c_str());
    string projectDBfilenane=outputdir + setName + ".mzrollDB";

    if (saveSqlLiteProject)  {
        ProjectDB project = ProjectDB(projectDBfilenane.c_str());

       if ( project.isOpen() ) {
            project.deleteAll();
            project.saveSamples(samples);
            project.saveGroups(allgroups,setName.c_str());
        }
    }

    cerr << "writeReport() " << allgroups.size() << " groups ";
    writeCSVReport   (outputdir + setName + ".tsv");
    //writeMS2SimilarityMatrix(outputdir + setName + ".fragMatrix.tsv");
    if(writeConsensusMS2) writeConsensusMS2Spectra(outputdir + setName + ".msp");
    //if(writeConsensusMS2) classConsensusMS2Spectra(outputdir + setName + "CLASS.msp");
}

void writeCSVReport( string filename) {
    ofstream groupReport;
    groupReport.open(filename.c_str());
    if(! groupReport.is_open()) return;

    string SEP = csvFileFieldSeperator;

    QStringList Header;
    Header << "label"<< "metaGroupId"
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

    for(unsigned int i=0; i< samples.size(); i++) { Header << samples[i]->sampleName.c_str(); }

    foreach(QString h, Header)  groupReport << h.toStdString() << SEP;
    groupReport << endl;

    for (int i=0; i < allgroups.size(); i++ ) {
        PeakGroup* group = &allgroups[i];
        writeGroupInfoCSV(group,groupReport);
    }

    groupReport.close();
}

Peak* getHighestIntensityPeak(PeakGroup* grp) {
    Peak* highestIntensityPeak=0;
    for(int i=0; i < grp->peaks.size(); i++ ) {
        if (!highestIntensityPeak or grp->peaks[i].peakIntensity > highestIntensityPeak->peakIntensity) {
           highestIntensityPeak = &grp->peaks[i];
        }
    }
    return highestIntensityPeak;
}

bool isC12Parent(Scan* scan, float monoIsotopeMz) {
    if(!scan) return 0;

    const double C13_DELTA_MASS = 13.0033548378-12.0;
    for(int charge=1; charge < 4; charge++) {
        int peakPos=scan->findHighestIntensityPos(monoIsotopeMz-C13_DELTA_MASS/charge,precursorPPM);
        if (peakPos != -1) {
            return false;
        }
    }

    return true;
}

bool isMonoisotope(PeakGroup* grp) {
    Peak* highestIntensityPeak=getHighestIntensityPeak(grp);
    if(! highestIntensityPeak) return false;

    int scanum = highestIntensityPeak->scan;
    Scan* s = highestIntensityPeak->getSample()->getScan(scanum);
    return isC12Parent(s,highestIntensityPeak->peakMz);
}

int getChargeStateFromMS1(PeakGroup* grp) {
    Peak* highestIntensityPeak=getHighestIntensityPeak(grp);
    int scanum = highestIntensityPeak->scan;
    float peakMz =highestIntensityPeak->peakMz;
    Scan* s = highestIntensityPeak->getSample()->getScan(scanum);

    int peakPos=0;
    if (s) peakPos = s->findHighestIntensityPos(highestIntensityPeak->peakMz,precursorPPM);

    if (s and peakPos > 0 and peakPos < s->nobs() ) {
        float ppm = (0.005/peakMz)*1e6;
        vector<int> parentPeaks = s->assignCharges(ppm);
        return parentPeaks[peakPos];
    } else {
        return 0;
    }
}

void classConsensusMS2Spectra(string filename) {
        ofstream outstream;
        outstream.open(filename.c_str());
        if(! outstream.is_open()) return;

         map<string,Fragment>compoundClasses;

        for (int i=0; i < allgroups.size(); i++ ) {
            PeakGroup* group = &allgroups[i];
            if (group->compound == NULL) continue; //skip unassinged compounds
            if (group->fragmentationPattern.nobs() == 0) continue; //skip compounds without fragmentation

            vector<string>nameParts; split(group->compound->category[0], ';',nameParts);
            if (nameParts.size() ==0) continue; //skip unassinged compound classes
            string compoundClass = nameParts[0];

            Fragment f = group->fragmentationPattern;
            f.addNeutralLosses();
            if (compoundClasses.count(compoundClass) == 0) {
                compoundClasses[compoundClass] = f;
            } else {
                compoundClasses[compoundClass].addFragment(&f);
            }
        }

        for(auto pair: compoundClasses) {
                string compoundClass = pair.first;
                Fragment& f = pair.second;
                if(f.brothers.size() < 10) continue;
                f.buildConsensus(productPpmTolr);

                cerr << "Writing " << compoundClass << endl;
                f.printConsensusNIST(outstream,0.75,productPpmTolr,f.group->compound);
        }
        outstream.close();
}

void writeConsensusMS2Spectra(string filename) {
        cerr << "Writing consensus specta to " << filename <<  endl;

        ofstream outstream;
        outstream.open(filename.c_str());
        if(! outstream.is_open()) return;

        for (int i=0; i < allgroups.size(); i++ ) {
            //Scan* x = allgroups[i].getAverageFragmenationScan(100);
            PeakGroup* group = &allgroups[i];
            if (group->fragmentationPattern.nobs() > 0 ) {
                group->fragmentationPattern.printConsensusNIST(outstream,0.01,productPpmTolr,group->compound);
            }
        }
        outstream.close();
}

void pullIsotopes(PeakGroup* parentgroup) {

    if(parentgroup == NULL) return;
    if(parentgroup->compound == NULL ) return;
    if(parentgroup->compound->formula.empty() == true) return;
    if ( samples.size() == 0 ) return;

    float ppm = precursorPPM;
    double maxIsotopeScanDiff = 10;
    double maxNaturalAbundanceErr = 100;
    double minIsotopicCorrelation=0.1;
    bool C13Labeled=true;
    bool N15Labeled=false;
    bool S34Labeled=false;
    bool D2Labeled=false;
    int eic_smoothingAlgorithm = 0;

    string formula = parentgroup->compound->formula;
    vector<Isotope> masslist = mcalc.computeIsotopes(formula,ionizationMode);

    map<string,PeakGroup>isotopes;
    map<string,PeakGroup>::iterator itr2;
    for ( unsigned int s=0; s < samples.size(); s++ ) {
        mzSample* sample = samples[s];
        for (int k=0; k< masslist.size(); k++) {
            Isotope& x= masslist[k];
            string isotopeName = x.name;
            double isotopeMass = x.mass;
            double expectedAbundance = x.abundance;
            float mzmin = isotopeMass-isotopeMass/1e6*ppm;
            float mzmax = isotopeMass+isotopeMass/1e6*ppm;
            float rt =  parentgroup->medianRt();
            float rtmin = parentgroup->minRt;
            float rtmax = parentgroup->maxRt;


            Peak* parentPeak =  parentgroup->getPeak(sample);
            if ( parentPeak ) rt  =   parentPeak->rt;
            if ( parentPeak ) rtmin = parentPeak->rtmin;
            if ( parentPeak ) rtmax = parentPeak->rtmax;

            float isotopePeakIntensity=0;
            float parentPeakIntensity=0;

            if ( parentPeak ) {
                parentPeakIntensity=parentPeak->peakIntensity;
                int scannum = parentPeak->getScan()->scannum;
                for(int i= scannum-3; i < scannum+3; i++ ) {
                    Scan* s = sample->getScan(i);

                    //look for isotopic mass in the same spectrum
                    vector<int> matches = s->findMatchingMzs(mzmin,mzmax);

                    for(int i=0; i < matches.size(); i++ ) {
                        int pos = matches[i];
                        if ( s->intensity[pos] > isotopePeakIntensity ) {
                            isotopePeakIntensity = s->intensity[pos];
                            rt = s->rt;
                        }
                    }
                }
            }
            //if(isotopePeakIntensity==0) continue;

            //natural abundance check
            if (    (x.C13 > 0    && C13Labeled==false)
                    || (x.N15 > 0 && N15Labeled==false)
                    || (x.S34 > 0 && S34Labeled==false )
                    || (x.H2 > 0  && D2Labeled==false )

                    ) {
                if (expectedAbundance < 1e-8) continue;
                if (expectedAbundance * parentPeakIntensity < 1) continue;
                float observedAbundance = isotopePeakIntensity/(parentPeakIntensity+isotopePeakIntensity);
                float naturalAbundanceError = abs(observedAbundance-expectedAbundance)/expectedAbundance*100;

                //cerr << isotopeName << endl;
                //cerr << "Expected isotopeAbundance=" << expectedAbundance;
                //cerr << " Observed isotopeAbundance=" << observedAbundance;
                //cerr << " Error="     << naturalAbundanceError << endl;

                if (naturalAbundanceError > maxNaturalAbundanceErr )  continue;
            }


            float w = maxIsotopeScanDiff*avgScanTime;
            double c = sample->correlation(isotopeMass,parentgroup->meanMz,ppm, rtmin-w,rtmax+w);
            if (c < minIsotopicCorrelation)  continue;

            //cerr << "pullIsotopes: " << isotopeMass << " " << rtmin-w << " " <<  rtmin+w << " c=" << c << endl;

            EIC* eic=NULL;
            for( int i=0; i<maxIsotopeScanDiff; i++ ) {
                float window=i*avgScanTime;
                eic = sample->getEIC(mzmin,mzmax,rtmin-window,rtmax+window,1);
                eic->setSmootherType((EIC::SmootherType) eic_smoothingAlgorithm);
                eic->getPeakPositions(eic_smoothingWindow);
                if ( eic->peaks.size() == 0 ) continue;
                if ( eic->peaks.size() > 1 ) break;
            }
            if (!eic) continue;

            Peak* nearestPeak=NULL; float d=FLT_MAX;
            for(int i=0; i < eic->peaks.size(); i++ ) {
                Peak& x = eic->peaks[i];
                float dist = abs(x.rt - rt);
                if ( dist > maxIsotopeScanDiff*avgScanTime) continue;
                if ( dist < d ) { d=dist; nearestPeak = &x; }
            }

            if (nearestPeak) {
                if (isotopes.count(isotopeName)==0) {
                    PeakGroup g;
                    g.meanMz=isotopeMass;
                    g.tagString=isotopeName;
                    g.expectedAbundance=expectedAbundance;
                    g.isotopeC13count= x.C13;
                    g.groupRank = 0;
                    isotopes[isotopeName] = g;
                }
                isotopes[isotopeName].addPeak(*nearestPeak);
            }
            delete(eic);
        }
    }

    parentgroup->children.clear();
    for(itr2 = isotopes.begin(); itr2 != isotopes.end(); itr2++ ) {
        string isotopeName = (*itr2).first;
        PeakGroup& child = (*itr2).second;
        child.tagString = isotopeName;
        child.metaGroupId = parentgroup->metaGroupId;
        //child.groupId = parentgroup->groupId;
        child.compound = parentgroup->compound;
        child.parent = parentgroup;
        child.setType(PeakGroup::IsotopeType);
        child.groupStatistics();
        if (clsf.hasModel()) { clsf.classify(&child); child.groupStatistics(); }
        parentgroup->addChild(child);
        //cerr << " add: " << isotopeName << " " <<  child.peaks.size() << " " << isotopes.size() << endl;
    }
   // cerr << "pullIsotopes done..: " << parentgroup->children.size();
    /*
            //if ((float) isotope.maxIntensity/parentgroup->maxIntensity > 3*ab[isotopeName]/ab["C12 PARENT"]) continue;
        */
}

void writeGroupInfoCSV(PeakGroup* group,  ofstream& groupReport) {

    if(! groupReport.is_open()) return;
    string SEP = csvFileFieldSeperator;
    PeakGroup::QType qtype = quantitationType;
    vector<float> yvalues = group->getOrderedIntensityVector(samples,qtype);

    char label[2];
    sprintf(label,"%c",group->label);

    groupReport << label << SEP
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
    float  expectedRtDiff=1000;
    float  ppmDist=1000;

    if ( group->compound != NULL) {
        Compound* c = group->compound;
        ppmDist = group->fragMatchScore.ppmError;
        expectedRtDiff = c->expectedRt-group->meanRt;
        compoundName = c->name;
        compoundID =  c->id;
        database = c->db;
        if (c->category.size()) compoundCategory = c->category[0];
    }

    groupReport << SEP << compoundName;
    groupReport << SEP << compoundID;
    groupReport << SEP << compoundCategory;
    groupReport << SEP << database;
    groupReport << SEP << expectedRtDiff;
    groupReport << SEP << ppmDist;

    if ( group->parent != NULL ) {
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

    for( unsigned int j=0; j < samples.size(); j++) groupReport << SEP <<  yvalues[j];
    groupReport << endl;

    for (unsigned int k=0; k < group->children.size(); k++) {
        writeGroupInfoCSV(&group->children[k],groupReport);
        //writePeakInfo(&group->children[k]);
    }
}

double get_wall_time(){
    struct timeval time;
    if (gettimeofday(&time,NULL)){
        //  Handle error
        return 0;
    }
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

double get_cpu_time(){
    return (double)getTime() / CLOCKS_PER_SEC;
}

string possibleClasses(set<Compound*>matches) {
    map<string,int>names;
    for(Compound* cpd : matches) {
        if(cpd->category.size() == 0) continue;
        vector<string>nameParts;
        split(cpd->category[0], ';',nameParts);
        if (nameParts.size()>0) names[nameParts[0]]++;
    }

    string classes;
    for(auto p: names) {
        classes += p.first + "[matched=" + integer2string(p.second) + " ] ";
    }
    return classes;
}

void writeMS2SimilarityMatrix(string filename) {
   vector<Fragment> allFragments;

   ofstream groupReport;
   groupReport.open(filename.c_str());
   if(! groupReport.is_open()) return;

   for (int i=0; i< allgroups.size(); i++) {
        PeakGroup* g = &allgroups[i];
        if (g->fragmentationPattern.nobs() == 0) continue;
        Fragment f = g->fragmentationPattern;
        f.addNeutralLosses();
        allFragments.push_back(f);
    }

    for (int i=0; i< allFragments.size(); i++) {
        Fragment& f1 = allFragments[i];
        for (int j=i; j< allFragments.size(); j++) {
            Fragment& f2 = allFragments[j];
            FragmentationMatchScore score = f1.scoreMatch( &f2, productPpmTolr);

            //print matrix
            if(score.numMatches > 2 ) {
                string id1,id2;
                if (f1.group->hasCompoundLink()) id1 = f1.group->compound->name;
                if (f2.group->hasCompoundLink()) id2 = f2.group->compound->name;

                groupReport
                     << f1.group->groupId  << "\t" << f2.group->groupId << "\t"
                     << abs(f1.group->meanMz - f2.group->meanMz) << "\t"
                     << abs(f1.group->meanRt - f2.group->meanRt) << "\t"
                     << score.numMatches <<  "\t"
                     << score.ticMatched << "\t"
                     << score.spearmanRankCorrelation << "\t"
                     << id1 << "\t" << id2 << endl;
            }
        }
    }
}

void matchFragmentation() {
        if(DB.compoundsDB.size() == 0) return;

        for (int i=0; i< allgroups.size(); i++) {
            PeakGroup* g = &allgroups[i];
            if(g->ms2EventCount == 0) continue;

            //set<Compound*> matches = findSpeciesByMass(g->meanMz,precursorPPM);
            vector<MassCalculator::Match>matchesX = DB.findMathchingCompounds(g->meanMz,precursorPPM,ionizationMode);
            //vector<MassCalculator::Match>matchesS = DB.findMathchingCompoundsSLOW(g->meanMz,precursorPPM,ionizationMode);

            if(matchesX.size() == 0) continue;

            g->fragmentationPattern.printFragment(productPpmTolr,10);
            Adduct* adduct=0;
            Compound* bestHit=0;
            FragmentationMatchScore bestScore;

            for(MassCalculator::Match m: matchesX ) {
                Compound* cpd = m.compoundLink;
                FragmentationMatchScore s = cpd->scoreCompoundHit(&g->fragmentationPattern,productPpmTolr,true);

                if (scoringScheme == "weightedDotProduct") {
                    s.mergedScore = s.weightedDotProduct;
                } else {
                    s.mergedScore = s.hypergeomScore;

                }
                s.ppmError = m.diff;

                if (s.numMatches == 0 ) continue;
                cerr << "\t"; cerr << cpd->name << "\t" << cpd->precursorMz << "\tppmDiff=" << m.diff << "\t num=" << s.numMatches << "\t tic" << s.ticMatched <<  " s=" << s.spearmanRankCorrelation << endl;
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

                cerr << "\t BESTHIT"
                     << setprecision(6)
                     << g->meanMz << "\t"
                     << g->meanRt << "\t"
                     << g->maxIntensity << "\t"
                     << bestHit->name << "\t"
                     << bestScore.numMatches << "\t"
                     << bestScore.fractionMatched << "\t"
                     << bestScore.ticMatched << endl;

            }
            cerr << "-------" << endl;
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
            if ( fileName.endsWith("msp",Qt::CaseInsensitive) || fileName.endsWith("sptxt",Qt::CaseInsensitive)) {
                compounds = DB.loadNISTLibrary(fileName);
            } else {
                compounds = DB.loadCompoundCSVFile(fileName);
            }

            DB.compoundsDB.insert( DB.compoundsDB.end(), compounds.begin(),compounds.end());
        }
    }

    sort(DB.compoundsDB.begin(),DB.compoundsDB.end(), Compound::compMass);
}
