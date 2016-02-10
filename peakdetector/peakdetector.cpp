#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <limits.h>
#include<sstream>
#include "Fragment.h"

#include "mzSample.h"
#include "mzMassSlicer.h"
#include "parallelMassSlicer.h"
#include "../mzroll/projectDB.h"
#include "options.h"
//include "omp.h"

#include "../mzroll/classifierNeuralNet.h"
#include "../libmzorbi/MersenneTwister.h"
#include "./Fragment.h"
#include "pugixml.hpp"
#include <sys/time.h>


#include <QtCore>
#include <QCoreApplication>

using namespace std;
#ifdef OMP_PARALLEL
	#define getTime() omp_get_wtime()
#else 
	#define getTime() get_wall_time()
	#define omp_get_thread_num()  1
#endif

//variables
int majorVersion = 1;
int minorVersion = 1;
int modVersion =   2;


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
float fragmentMatchPPMTolr=10;
float productAmuToll = 0.01;


void processOptions(int argc, char* argv[]);
void loadSamples(vector<string>&filenames);
int loadCompoundCSVFile(string filename);
int loadNISTLibrary(QString fileName);
void printSettings();
void alignSamples();
void processSlices(vector<mzSlice*>&slices,string setName);
void processCompounds(vector<Compound*> set, string setName);
void processMassSlices(int limit=0);
void reduceGroups();
void writeReport(string setName);
void writeCSVReport(string filename);
void writeQEInclusionList(string filename);
void writeConsensusMS2Spectra(string filename);
void classConsensusMS2Spectra(string filename);
void pullIsotopes(PeakGroup* parentgroup);
void writeGroupInfoCSV(PeakGroup* group,  ofstream& groupReport);

vector<Adduct*> adductsDB;

bool addPeakGroup(PeakGroup& grp);

int getChargeStateFromMS1(PeakGroup* grp);
bool isC12Parent(Scan* scan, float monoIsotopeMz);
bool isMonoisotope(PeakGroup* grp);

vector<mzSlice*> getSrmSlices();
string cleanSampleName( string sampleName);

void matchFragmentation();
void writeMS2SimilarityMatrix(string filename);

void computeAdducts();
void loadAdducts(string filename);

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


	//load compound list
	if (!ligandDbFilename.empty()) {
        //processAllSlices=false;
             if(ligandDbFilename.find("csv") !=std::string::npos )  loadCompoundCSVFile(ligandDbFilename);
        else if(ligandDbFilename.find("txt") !=std::string::npos )  loadCompoundCSVFile(ligandDbFilename);
        else if(ligandDbFilename.find("tab") !=std::string::npos )  loadCompoundCSVFile(ligandDbFilename);
        else if(ligandDbFilename.find("msp") !=std::string::npos )  loadNISTLibrary(ligandDbFilename.c_str());
        cerr << "Loaded " << compoundsDB.size() << " compounds." << endl;
	}

    //process compound list
    if (compoundsDB.size() and searchAdductsFlag) {
        //processCompounds(compoundsDB, "compounds");
        loadAdducts(methodsFolder + "/" + adductDbFilename);
        computeAdducts();
    }

    //load files
    double startLoadingTime = getTime();
    loadSamples(filenames);
    printf("Execution time (Sample loading) : %f seconds \n", getTime() - startLoadingTime);
	if (samples.size() == 0 ) { cerr << "Exiting .. nothing to process " << endl;  exit(1); }

	//get retenation time resoluution
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
    int converged=0;
    int eicCounter=0;
    int foundGroups=0;

    double EICPullTime = 0.0;
    double peakGroupingTime = 0.0;
        double midPhaseTime = 0.0;
        double convergenceTime = 0.0;
        double endPhaseTime = 0.0;

        double startForLoopTime = getTime();

//Creating openmp threads and static scheduling with chunk of size 128
#ifdef OMP_PARALLEL
#pragma omp parallel for ordered num_threads(8) private(converged, eicCounter, foundGroups) schedule(static,128)
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
                group.tagString = compound->name;
            }

            if( clsf.hasModel()) { clsf.classify(&group); group.groupStatistics(); }
            if (clsf.hasModel() && group.goodPeakCount < minGoodGroupCount) continue;
            if (clsf.hasModel() && group.maxQuality < minQuality) continue;

            if (group.maxNoNoiseObs < minNoNoiseObs) continue;
            if (group.maxSignalBaselineRatio < minSignalBaseLineRatio) continue;
            if (group.maxIntensity < minGroupIntensity ) continue;
            if (getChargeStateFromMS1(&group) < minPrecursorCharge) continue;

            if (mustHaveMS2) {
                vector<Scan*>ms2events = group.getFragmenationEvents();
                //group.MS2Count= ms2events.size();
                //cerr << "MS2eventCOunt: " << ms2events.size() << endl;
                if(ms2events.size() == 0) continue;
            }

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
                                "k?productAmuToll <float>",
                                "m?methodsFolder <string>",
                                "n?eicMaxGroups <int>",
                                "o?outputdir <string>",
                                "p?ppmMerge <float>",
                                "q?minQuality <float>",
                                "r?rtStepSize <float>",
                                "s?nameSuffix <string>",
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
            case 'k' : productAmuToll=atof(optarg); break;
            case 'm' : methodsFolder = optarg; break;
            case 'n' : eicMaxGroups=atoi(optarg); break;
            case 'o' : outputdir = optarg + string(DIR_SEPARATOR_STR); break;
            case 'p' : precursorPPM=atof(optarg); break;
            case 'q' : minQuality=atof(optarg); break;
            case 'r' : rtStepSize=atof(optarg); break;
            case 's' : nameSuffix=string(optarg); break;
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

#ifdef OMP_PARALLEL
		#pragma omp parallel for ordered num_threads(4) schedule(static)  
#endif
        int sampleOrder=0;
		for (unsigned int i=0; i < filenames.size(); i++ ) {
//          cout << "Thread # " << omp_get_thread_num() << endl;
			cerr << "Loading " << filenames[i] << endl;
			mzSample* sample = new mzSample();
			sample->loadSample(filenames[i].c_str());
			sample->sampleName = cleanSampleName(filenames[i]);

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

string cleanSampleName( string sampleName) { 
		unsigned int pos =sampleName.find_last_of("/");
		if (pos != std::string::npos) { 
				sampleName=sampleName.substr(pos+1, sampleName.length());
				
		}

		pos=sampleName.find_last_of("\\");
		if (pos != std::string::npos) { 
				sampleName=sampleName.substr(pos+1, sampleName.length());
		}
		return sampleName;
		
}

void loadAdducts(string filename) {
    ifstream myfile(filename.c_str());
    if (! myfile.is_open()) return;

    string line;
    while ( getline(myfile,line) ) {
        if (line.empty()) continue;
        if (line[0] == '#') continue;

        vector<string>fields;
        mzUtils::split(line,',', fields);

        //ionization

        if(fields.size() < 2 ) continue;
        string name=fields[0];
        int nmol=string2float(fields[1]);
        int charge=string2float(fields[2]);
        float mass=string2float(fields[3]);

        //if ( SIGN(charge) != SIGN(ionizationMode) ) continue;
        if ( name.empty() || nmol < 0 ) continue;
        cerr << "#ADDUCT: " << name << endl;
        Adduct* a = new Adduct();
        a->nmol = nmol;
        a->name = name;
        a->mass = mass;
        a->charge = charge;
        a->isParent = false;
        if (abs(abs(a->mass)-HMASS)< 0.01) a->isParent=true;
        adductsDB.push_back(a);
    }
    cerr << "loadAdducts() " << filename << " count=" << adductsDB.size() << endl;
    myfile.close();
}

void computeAdducts() {
    MassCalculator mcalc;
    for(Compound* c: compoundsDB) {
        float parentMass = mcalc.computeNeutralMass(c->formula);
        for(Adduct* a: adductsDB ) {
            if ( SIGN(a->charge) != SIGN(ionizationMode) ) continue;
            float adductMass=a->computeAdductMass(parentMass);
            Compound* compoundAdduct = new Compound(c->id, c->name + a->name, c->formula, ionizationMode);
            compoundAdduct->mass = compoundAdduct->precursorMz = adductMass;
            compoundAdduct->category = c->category;
            compoundAdduct->fragment_intensity = c->fragment_intensity;
            compoundAdduct->fragment_mzs = c->fragment_mzs;
            compoundsDB.push_back(compoundAdduct);
            //cerr << c->name << " " << a->name << " " << adductMass << endl;
        }
     }

    //resort compound DB, this allows for binary search
    sort(compoundsDB.begin(),compoundsDB.end(), Compound::compMass);
}


int loadCompoundCSVFile(string filename){

    ifstream myfile(filename.c_str());
    if (! myfile.is_open()) {
        cerr << "Can't open " << filename << endl;
        exit(-1);
    }

    string line;
    string dbname = mzUtils::cleanFilename(filename);
    int loadCount=0;
    int lineCount=0;
    map<string, int>header;
    vector<string> headers;
    MassCalculator mcalc;

    //assume that files are tab delimited, unless  ".csv", then comma delimited
    char sep='\t';
    if(filename.find(".csv") != -1 || filename.find(".CSV") != -1) sep=',';

    cerr << filename << " sep=" << sep << endl;
    while ( getline(myfile,line) ) {
        if (!line.empty() && line[0] == '#') continue;
        if (lineCount == 0 and line.find("\t") != std::string::npos) { sep ='\t'; }

        //trim spaces on the left
        line.erase(line.find_last_not_of(" \n\r\t")+1);
        lineCount++;

        vector<string>fields;
        mzUtils::split(line, sep, fields);

        for(unsigned int i=0; i < fields.size(); i++ ) {
            int n = fields[i].length();
            if (n>2 && fields[i][0] == '"' && fields[i][n-1] == '"') {
                fields[i]= fields[i].substr(1,n-2);
            }
            if (n>2 && fields[i][0] == '\'' && fields[i][n-1] == '\'') {
                fields[i]= fields[i].substr(1,n-2);
            }
        }

        if (lineCount==1) {
            headers = fields;
            for(unsigned int i=0; i < fields.size(); i++ ) {
                fields[i]=makeLowerCase(fields[i]);
                header[ fields[i] ] = i;
            }
            continue;
        }

        string id, name, formula;
        float rt=0;
        float mz=0;
        float charge=0;
        float collisionenergy=0;
        float precursormz=0;
        float productmz=0;
        int N=fields.size();
        vector<string>categorylist;

        if ( header.count("mz") && header["mz"]<N)  mz = string2float(fields[ header["mz"]]);
        if ( header.count("rt") && header["rt"]<N)  rt = string2float(fields[ header["rt"]]);
        if ( header.count("expectedrt") && header["expectedrt"]<N) rt = string2float(fields[ header["expectedrt"]]);
        if ( header.count("charge")&& header["charge"]<N) charge = string2float(fields[ header["charge"]]);
        if ( header.count("formula")&& header["formala"]<N) formula = fields[ header["formula"] ];
        if ( header.count("id")&& header["id"]<N) 	 id = fields[ header["id"] ];
        if ( header.count("name")&& header["name"]<N) 	 name = fields[ header["name"] ];
        if ( header.count("compound")&& header["compound"]<N) 	 name = fields[ header["compound"] ];

        if ( header.count("precursormz") && header["precursormz"]<N) precursormz=string2float(fields[ header["precursormz"]]);
        if ( header.count("productmz") && header["productmz"]<N)  productmz = string2float(fields[header["productmz"]]);
        if ( header.count("collisionenergy") && header["collisionenergy"]<N) collisionenergy=string2float(fields[ header["collisionenergy"]]);

        if ( header.count("Q1") && header["Q1"]<N) precursormz=string2float(fields[ header["Q1"]]);
        if ( header.count("Q3") && header["Q3"]<N)  productmz = string2float(fields[header["Q3"]]);
        if ( header.count("CE") && header["CE"]<N) collisionenergy=string2float(fields[ header["CE"]]);

        //cerr << lineCount << " " << endl;
        //for(int i=0; i<headers.size(); i++) cerr << headers[i] << ", ";
        //cerr << "   -> category=" << header.count("category") << endl;
        if ( header.count("category") && header["category"]<N) {
            string catstring=fields[header["category"]];
            if (!catstring.empty()) {
                mzUtils::split(catstring,';', categorylist);
                if(categorylist.size() == 0) categorylist.push_back(catstring);
                //cerr << catstring << " ListSize=" << categorylist.size() << endl;
            }
         }

        if ( header.count("polarity") && header["polarity"] <N)  {
            string x = fields[ header["polarity"]];
            if ( x == "+" ) {
                charge = 1;
            } else if ( x == "-" ) {
                charge = -1;
            } else  {
                charge = string2float(x);
            }

        }



        //cerr << "Loading: " << id << " " << formula << "mz=" << mz << " rt=" << rt << " charge=" << charge << endl;

        if (mz == 0) mz=precursormz;
        if (id.empty()&& !name.empty()) id=name;
        if (id.empty() && name.empty()) id="cmpd:" + integer2string(loadCount);

        if ( mz > 0 || ! formula.empty() ) {
            Compound* compound = new Compound(id,name,formula,charge);
            compound->expectedRt = rt;
            if (mz == 0) mz = mcalc.computeMass(formula,charge);
            compound->mass = mz;
            compound->db = dbname;
            compound->expectedRt=rt;
            compound->precursorMz=precursormz;
            compound->productMz=productmz;
            compound->collisionEnergy=collisionenergy;
            for(int i=0; i < categorylist.size(); i++) compound->category.push_back(categorylist[i]);
            compoundsDB.push_back(compound);
            loadCount++;
        }
    }

    sort(compoundsDB.begin(),compoundsDB.end(), Compound::compMass);
    cerr << "Loading compound list from file:" << dbname << " Found: " << loadCount << " compounds" << endl;
    myfile.close();
    return loadCount;
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
    int metaGroupNum=1;
    for(PeakGroup &g : allgroups) {
        g.groupId=groupNum++;
        g.metaGroupId = metaGroupNum++;
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

    //create an output folder
    mzUtils::createDir(outputdir.c_str());
    string projectDBfilenane=outputdir + setName + ".mzrollDB";

    if (saveSqlLiteProject)  {
        ProjectDB project = ProjectDB(projectDBfilenane.c_str());

        if ( project.open() ) {
            project.saveSamples(samples);
            project.saveGroups(allgroups);
        }
    }

    cerr << "writeReport() " << allgroups.size() << " groups ";
    writeCSVReport   (outputdir + setName + ".tsv");
    //writeMS2SimilarityMatrix(outputdir + setName + ".fragMatrix.tsv");
    //writeQEInclusionList(outputdir + setName + ".txt");
    if(writeConsensusMS2) writeConsensusMS2Spectra(outputdir + setName + ".msp");
    if(writeConsensusMS2) classConsensusMS2Spectra(outputdir + setName + "CLASS.msp");
}

void writeCSVReport( string filename) {
    ofstream groupReport;
    groupReport.open(filename.c_str());
    if(! groupReport.is_open()) return;

    int metaGroupId=0;
    string SEP = csvFileFieldSeperator;

    QStringList Header;
    Header << "label"<< "metaGroupId"<< "groupId"<< "goodPeakCount"
                        <<"medMz" << "medRt"<<"maxQuality"
                        <<"note"<<"compound" <<"compoundId" << "category"
                        <<"expectedRtDiff"<<"ppmDiff"<<"parent"
                        <<"ms2EventCount"
                        <<"fragNumIonsMatched"
                        <<"fragFracMatched"
                        <<"ticMatched"
                        <<"dotProduct"
                        <<"mzFragError"
                        <<"spearmanRankCorrelation";

    for(unsigned int i=0; i< samples.size(); i++) { Header << samples[i]->sampleName.c_str(); }

    foreach(QString h, Header)  groupReport << h.toStdString() << SEP;
    groupReport << endl;

    for (int i=0; i < allgroups.size(); i++ ) {
        PeakGroup* group = &allgroups[i];
        writeGroupInfoCSV(group,groupReport);
    }

    groupReport.close();
}

void writeSampleListXML(xml_node& parent ) {
     xml_node samplesset = parent.append_child();
     samplesset.set_name("samples");

     for(int i=0; i < samples.size(); i++ ) {
  	xml_node _sample = samplesset.append_child();
        _sample.set_name("sample");
        _sample.append_attribute("name") = samples[i]->sampleName.c_str();
        _sample.append_attribute("filename") = samples[i]->fileName.c_str();
        _sample.append_attribute("sampleOrder") = i;
        _sample.append_attribute("setName") = "A";
        _sample.append_attribute("sampleName") = samples[i]->sampleName.c_str();
      }
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
    float peakMz =highestIntensityPeak->peakMz;
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

         map<string,Fragment*>compoundClasses;

        for (int i=0; i < allgroups.size(); i++ ) {
            PeakGroup* group = &allgroups[i];
            if (group->compound == NULL) continue; //skip unassinged compounds

            vector<string>nameParts; split(group->compound->category[0], ';',nameParts);
            if (nameParts.size() ==0) continue; //skip unassinged compound classes
            string compoundClass = nameParts[0];

            vector<Scan*>ms2events =group->getFragmenationEvents();
            if (ms2events.size() == 0) continue; //skip groups wihtout MS2 events
            cerr << "add.." << compoundClass << " " << ms2events.size() << endl;

            Scan* bestScan = 0;
            for(Scan* s : ms2events) { if(!bestScan or s->totalIntensity() > bestScan->totalIntensity()) bestScan=s; }

            Fragment* f = 0;
            if (compoundClasses.count(compoundClass) == 0) {
                f = new Fragment(bestScan,0.01,1,1024);
                f->addNeutralLosses();
                f->group = group;
                compoundClasses[compoundClass] = f;
                continue;
            } else {
                Fragment* x = new Fragment(bestScan,0.01,1,1024);
                x->addNeutralLosses();
                f = compoundClasses[compoundClass];
                f->addFragment(x);
            }

        }

        for(auto pair: compoundClasses) {
                string compoundClass = pair.first;
                Fragment* f = pair.second;
                if(f->brothers.size() < 10) continue;
                f->buildConsensus(productAmuToll);

                cerr << "Writing " << compoundClass << endl;
                f->printConsensusNIST(outstream,0.75,productAmuToll,f->group->compound);
                //f->printMzList();
                //f->printConsensusNIST(outstream,0.01,productAmuToll);
                delete(f);
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
            vector<Scan*>ms2events =group->getFragmenationEvents();
            if (ms2events.size() > 0) {
                Fragment* f = new Fragment(ms2events[0],0.01,1,1024);
                for(Scan* s : ms2events) {  f->addFragment(new Fragment(s,0,0.01,1024)); }
                f->buildConsensus(productAmuToll);
                f->printConsensusNIST(outstream,0.01,productAmuToll,group->compound);
                delete(f);
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
        child.setType(PeakGroup::Isotope);
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

    string tagString = group->srmId + group->tagString;
    tagString = mzUtils::substituteInQuotedString(tagString,"\",'","---");

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
                << tagString;

    string compoundName;
    string compoundCategory;
    string compoundID;
    float  expectedRtDiff=1000;
    float  ppmDist=1000;

    if ( group->compound != NULL) {
        Compound* c = group->compound;
        compoundName = mzUtils::substituteInQuotedString(c->name,"\",'","_");
        if (c->category.size()) compoundCategory = c->category[0];
        compoundID =  c->id;
        double mass =c->mass;

        if (c->precursorMz > 0 ) {
            ppmDist = mzUtils::ppmDist(c->precursorMz,group->meanMz);
        } else if (!c->formula.empty()) {
            float formula_mass =  mcalc.computeMass(c->formula,ionizationMode);
            if(formula_mass) mass=formula_mass;
            ppmDist = mzUtils::ppmDist(mass,(double) group->meanMz);
        }
        expectedRtDiff = c->expectedRt-group->meanRt;
    }

    groupReport << SEP << compoundName;
    groupReport << SEP << compoundID;
    groupReport << SEP << compoundCategory;
    groupReport << SEP << expectedRtDiff;
    groupReport << SEP << ppmDist;

    if ( group->parent != NULL ) {
        groupReport << SEP << group->parent->meanMz;
    } else {
        groupReport << SEP << group->meanMz;
    }

    vector<Scan*>ms2events =group->getFragmenationEvents();
    groupReport << SEP << ms2events.size();
    groupReport << SEP << group->fragMatchScore.numMatches;
    groupReport << SEP << group->fragMatchScore.fractionMatched;
    groupReport << SEP << group->fragMatchScore.ticMatched;
    groupReport << SEP << group->fragMatchScore.dotProduct;
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

int loadNISTLibrary(QString fileName) {
    qDebug() << "Loading NIST Libary: " << fileName;
    QFile data(fileName);

    if (!data.open(QFile::ReadOnly) ) {
        qDebug() << "Can't open " << fileName; exit(-1);
        return 0;
    }

    string dbfilename = fileName.toStdString();
    string dbname = mzUtils::cleanFilename(dbfilename);

   QTextStream stream(&data);

   /* sample
   Name: DGDG 8:0; [M-H]-; DGDG(2:0/6:0)
   MW: 555.22888
   PRECURSORMZ: 555.22888
   Comment: Parent=555.22888 Mz_exact=555.22888 ; DGDG 8:0; [M-H]-; DGDG(2:0/6:0); C23H40O15
   Num Peaks: 2
   115.07586 999 "sn2 FA"
   59.01330 999 "sn1 FA"
   */
  //Comment: Spec=Consensus Parent=417.2120056 Protein="Michrom_PTD_gi1351907" collisionEnergy=25 AvgMVH=35.73300171 AvgRt=21.34076614 MinRt=20.54099846 MaxRt=22.80683327 StdRt=0.9019529997 SampleName=BSA_run_130202213057.mzXML ConsensusSize=5

   QRegExp whiteSpace("\\s+");
   QRegExp formulaMatch("(C\\d+H\\d+\\S*)");
   QRegExp retentionTimeMatch("AvgRt\\=(\\S+)");

   int charge=0;
   QString line;
   QString name, comment,formula,id, category,smileString;
   double retentionTime;
   double mw=0;
   double precursor=0;
   int peaks=0;
   double logP = 0;
   vector<float>mzs;
   vector<float>intest;

   int compoundCount=0;
   MassCalculator mcalc;

    do {
        line = stream.readLine();

        if(line.startsWith("Name:",Qt::CaseInsensitive) && !name.isEmpty()) {
            if (!name.isEmpty()) { //insert new compound
               Compound* cpd = new Compound( name.toStdString(), name.toStdString(), formula.toStdString(), charge);

               if (precursor>0) { cpd->mass=precursor; cpd->precursorMz=precursor; }
               else if (mw>0) { cpd->mass=mw; cpd->precursorMz=precursor; }
               else if(!formula.isEmpty()) { cpd->mass = cpd->precursorMz = mcalc.computeMass(formula.toStdString(),ionizationMode); }

               if(!id.isEmpty()) cpd->id = id.toStdString();
               cpd->db=dbname;
               cpd->fragment_mzs = mzs;
               cpd->fragment_intensity = intest;
               cpd->expectedRt=retentionTime;
               cpd->category.push_back(category.toStdString());
               cpd->logP = logP;
               cpd->smileString = smileString.toStdString();
               compoundsDB.push_back(cpd);

            }

            //reset for the next record
           name = comment = formula = category=id=smileString=QString();
           mw=precursor=0;
           retentionTime=0;
           logP=0;
           peaks=0;
           mzs.clear();
           intest.clear();
        }

         if(line.startsWith("Name:",Qt::CaseInsensitive)) {
             name = line.mid(5,line.length()).simplified();
         } else if (line.startsWith("MW:",Qt::CaseInsensitive)) {
             mw = line.mid(3,line.length()).simplified().toDouble();
         } else if (line.startsWith("ID:",Qt::CaseInsensitive)) {
             id = line.mid(3,line.length()).simplified();
         } else if (line.startsWith("LOGP:",Qt::CaseInsensitive)) {
            logP = line.mid(5,line.length()).simplified().toDouble();
         } else if (line.startsWith("SMILE:",Qt::CaseInsensitive)) {
             smileString = line.mid(7,line.length()).simplified();
         } else if (line.startsWith("PRECURSORMZ:",Qt::CaseInsensitive)) {
             precursor = line.mid(13,line.length()).simplified().toDouble();
         } else if (line.startsWith("FORMULA:",Qt::CaseInsensitive)) {
             formula = line.mid(9,line.length()).simplified();
             formula.replace("\"","",Qt::CaseInsensitive);
         } else if (line.startsWith("CATEGORY:",Qt::CaseInsensitive)) {
             category = line.mid(10,line.length()).simplified();
         } else if (line.startsWith("Comment:",Qt::CaseInsensitive)) {
             comment = line.mid(8,line.length()).simplified();
             if (comment.contains(formulaMatch)){
                 formula=formulaMatch.capturedTexts().at(1);
                 //qDebug() << "Formula=" << formula;
             }
            if (comment.contains(retentionTimeMatch)){
                 retentionTime=retentionTimeMatch.capturedTexts().at(1).simplified().toDouble();
                 //qDebug() << "retentionTime=" << retentionTimeString;
             }
         } else if (line.startsWith("Num Peaks:",Qt::CaseInsensitive) || line.startsWith("NumPeaks:",Qt::CaseInsensitive)) {
             peaks = 1;
         } else if ( peaks ) {
             QStringList mzintpair = line.split(whiteSpace);
             if( mzintpair.size() >=2 ) {
                 bool ok=false; bool ook=false;
                 float mz = mzintpair.at(0).toDouble(&ok);
                 float ints = mzintpair.at(1).toDouble(&ook);
                 if (ok && ook && mz >= 0 && ints >= 0) {
                     mzs.push_back(mz);
                     intest.push_back(ints);
                 }
             }
         }

    } while (!line.isNull());

    sort(compoundsDB.begin(),compoundsDB.end(), Compound::compMass);
    return compoundCount;
}


set<Compound*> findSpeciesByMass(float mz, float ppm) {
        set<Compound*>uniqset;
        Compound x("find", "", "",0);
        x.mass = mz-(mz/1e6*ppm);;
        vector<Compound*>::iterator itr = lower_bound( compoundsDB.begin(),compoundsDB.end(),  &x, Compound::compMass );

        for(;itr != compoundsDB.end(); itr++ ) {
            Compound* c = *itr;
            if (c->mass > mz+0.5) break;

            if ( mzUtils::ppmDist(c->mass,mz) < ppm ) {
                if (uniqset.count(c)) continue;
                uniqset.insert(c);
            }
        }
        return uniqset;
}

FragmentationMatchScore scoreCompoundHit(Compound* cpd, Fragment* f) {
        FragmentationMatchScore s;
        if (!cpd or cpd->fragment_mzs.size() == 0) return s;

        Fragment t;
        t.precursorMz = cpd->precursorMz;
        t.mzs = cpd->fragment_mzs;
        t.intensity_array = cpd->fragment_intensity;

        int N = t.mzs.size();
        for(int i=0; i<N;i++) {
            t.mzs.push_back( t.mzs[i] + PROTON);
            t.intensity_array.push_back( t.intensity_array[i] );
            t.mzs.push_back( t.mzs[i] - PROTON );
            t.intensity_array.push_back( t.intensity_array[i] );
        }

        t.sortByIntensity();
        s = t.scoreMatch(f,productAmuToll);

        if(s.numMatches>0)  {
            vector<int>ranks = Fragment::compareRanks(&t,f,productAmuToll);
            for(int i=0; i< ranks.size(); i++ ) if(ranks[i]>0) cerr << t.mzs[i] << " "; cerr << endl;
        }
        return s;
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
        vector<Scan*>ms2events =g->getFragmenationEvents();
        if(ms2events.size() == 0 ) continue;

        Fragment* f = new Fragment(ms2events[0],0.01,1,1024);
        for(Scan* s : ms2events) {  f->addFragment(new Fragment(s,0,0.01,1024)); }
        f->buildConsensus(productAmuToll);

        //resize consensus to top 10  peaks
        f->consensus->mzs.resize(10);
        f->consensus->intensity_array.resize(10);

        f->consensus->group = g;

        //neutral losses .. will not work with double charged
        float parentMass = g->meanMz;
        vector<float> nL, nLI;
        Fragment* z = f->consensus;

        for(int i=0; i < z->mzs.size(); i++) {
            float nLMass = parentMass- (z->mzs[i]);
            float nLIntensity = z->intensity_array[i];
            if( nLMass > 0 ) {
                nL.push_back(nLMass);
                nLI.push_back(nLIntensity);
            }
        }

        for(int i=0; i < nL.size(); i++ ) {
            z->mzs.push_back(nL[i]);
            z->intensity_array.push_back(nLI[i]);
        }

        z->printFragment(productAmuToll,10);
        allFragments.push_back(z);
        delete(f);
    }

    for (int i=0; i< allFragments.size(); i++) {
        Fragment& f1 = allFragments[i];
        for (int j=i; j< allFragments.size(); j++) {
            Fragment& f2 = allFragments[j];
            FragmentationMatchScore score = f1.scoreMatch( &f2, productAmuToll);

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
        if(compoundsDB.size() == 0) return;

        for (int i=0; i< allgroups.size(); i++) {
            PeakGroup* g = &allgroups[i];
            set<Compound*> matches = findSpeciesByMass (g->meanMz,precursorPPM);
           if(matches.size() == 0) continue;

            vector<Scan*>ms2events =g->getFragmenationEvents();
            string possibleClass = possibleClasses(matches);

            cerr << g->meanMz << " compoundMatches=" << matches.size()
                              << " msCount=" << ms2events.size()
                              << " possibleClasses=" << possibleClass << endl;

            g->tagString = possibleClass;

            if(ms2events.size() == 0) continue;

            Fragment* f = new Fragment(ms2events[0],0.01,1,1024);
            for(Scan* s : ms2events) {  f->addFragment(new Fragment(s,0,0.01,1024)); }
            f->buildConsensus(productAmuToll);

            //cerr << "Consensus: " << endl;
            f->consensus->printFragment(productAmuToll,10);
            Compound* bestHit=0;
            FragmentationMatchScore bestScore;

            for(Compound* cpd : matches) {
                FragmentationMatchScore s = scoreCompoundHit(cpd,f);
                s.mergedScore = s.ticMatched;
                if (s.numMatches == 0 ) continue;
                cerr << "\t"; cerr << cpd->name << "\t" << cpd->precursorMz << "\t num=" << s.numMatches << "\t tic" << s.ticMatched <<  " s=" << s.mergedScore << endl;
                if (s.mergedScore > bestScore.mergedScore ) {
                    //for(auto f: cpd->fragment_mzs) cerr << f << " "; cerr << endl;
                    bestScore = s;
                    bestHit = cpd;
                }
            }

            if (bestHit) {
                g->compound = bestHit;
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
            delete(f);
            cerr << "-------" << endl;
       }
}
