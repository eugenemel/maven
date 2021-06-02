#ifndef BACKGROUND_PEAK_UPDATE_H
#define BACKGROUND_PEAK_UPDATE_H

#include "stable.h"
#include "mzSample.h"
#include "mainwindow.h"
#include "database.h"
#include "csvreports.h"
#include <iostream>


class MainWindow;
class Database;
class TableDockWidget;
class EIC;

extern Database DB;

class BackgroundPeakUpdate : public QThread
{
  Q_OBJECT

public:

  BackgroundPeakUpdate(QWidget*);
  ~BackgroundPeakUpdate();
  void setRunFunction(QString functionName);
  void setCompounds(vector<Compound*>set) { compounds=set; }
  void setSlices(vector<mzSlice*>set)     { _slices=set;   }
  void setPeakGroup(PeakGroup* p)         { _group = p;     }
  void setSamples(vector<mzSample*>&set);
  void setMainWindow(MainWindow* mw)	{ mainwindow = mw; }
  void setOutputDir(QString outdir)		{ outputdir = outdir.toStdString() + string(DIR_SEPARATOR_STR); }
  void setMaxGroupCount(int x) 			{ limitGroupCount=x; }
  void stop() { _stopped=true; }
  bool stopped() { return _stopped; }
  
  bool writeCSVFlag;
  bool alignSamplesFlag;
  bool keepFoundGroups;
  bool processMassSlicesFlag;
  bool pullIsotopesFlag;
  bool showProgressFlag;
  bool matchRtFlag;
  bool checkConvergence;
  bool featureMatchRtFlag;

  int ionizationMode; //default ionization mode used by mass spec

  //isotopes
  float isotopeMzTolr;

  //mass slicing parameters
  float rtStepSize;
  float avgScanTime;
  float ppmMerge;   //for Peaks search only

  //peaks compound matching
  float featureCompoundMatchMzTolerance;
  float featureCompoundMatchRtTolerance;

  //peak detection
  float eic_smoothingWindow; //smoothing window
  int  eic_smoothingAlgorithm;
  float eic_ppmWindow;
  int baseline_smoothingWindow;
  int baseline_dropTopX;

  //peak filtering
   int 	 minGoodPeakCount;
   float minSignalBlankRatio;
   float minNoNoiseObs;
   float minSignalBaseLineRatio;
   float minGroupIntensity;
   float minQuality;

  //eic window around compound
   float compoundPPMWindow;
   float compoundRTWindow;
   int   eicMaxGroups;

   //grouping of peaks across samples
   float grouping_maxRtWindow;		//do no group peaks that are greater than differ more than X in retention time

   PeakGroupCompoundMatchingPolicy peakGroupCompoundMatchingPolicy;

   //stop looking for groups if group count is greater than X
    int limitGroupCount;

    bool excludeIsotopicPeaks;

    //MS2 matching
    bool mustHaveMS2=false;
    bool compoundMustHaveMs2=false;

    bool searchProton=false;    //consider +/-PROTON when matching fragmentation
    float minFragmentMatchScore=0;
    float minNumFragments=0;
    float minNumDiagnosticFragments=0;
    float productPpmTolr = 20;
    QString scoringScheme="dotProduct"; // ticMatch | spearmanRank | dotProduct
    QString compoundDatabase;

   //triple quad compound matching
   float amuQ1;
   float amuQ3;

   //matching options
   bool isRequireMatchingAdduct;
   bool isRetainUnmatchedCompounds;
   bool isClusterPeakGroups;

   Classifier* clsf;


//CLASS FUNCTIONS
 static vector<EIC*> pullEICs(mzSlice* slice,
                              std::vector<mzSample*>&samples,
                              int peakDetect,
                              int smoothingWindow,
                              int smoothingAlgorithm,
                              float amuQ1,
                              float amuQ3,
                              int baselineSmoothingWindow,
                              int baselineDropTopX,
                              string scanFilterString="",
                              pair<float, float> mzKey = make_pair(0.0f,0.0f)
                              );

signals:
	void updateProgressBar(QString,int,int);
    void newPeakGroup(PeakGroup* group, bool add, bool isDeletePeakGroupPtr);

protected:
  void run(void);

private:
  string runFunction;
  string outputdir;

  MassCalculator mcalc;
  MainWindow *mainwindow;
  PeakGroup* _group;
  vector<mzSample*>samples;
  vector<Compound*>compounds;
  vector<mzSlice*> _slices;

  void pullIsotopes(PeakGroup *group);
  void processSlices(void);
  void processSlices(vector<mzSlice*>&slices,string setName);
  void processSlice(mzSlice& slice);
  void processCompounds(vector<Compound*> set, string setName);
  void processCompoundSlices(vector<mzSlice*>&slices, string setName);
  void processSRMTransitions(vector<SRMTransition*>& transitions);
  vector<tuple<float, Compound*, Adduct*>> prepareCompoundDatabase(vector<Compound*> set);
  void computePeaks();
  void processMassSlices();
  void findPeaksOrbi(void);
  void findPeaksQQQ(void);
  bool addPeakGroup(PeakGroup& group);
  vector<PeakGroup>allgroups;
  void cleanup();
  void printSettings();
  void matchFragmentation(PeakGroup* g, vector<tuple<float, Compound*, Adduct*>>& searchableDatabase);
  bool sliceHasMS2Event(mzSlice* slice);

  private:
  volatile bool _stopped;

};

#endif


