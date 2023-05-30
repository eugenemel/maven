#include <iostream>
#include <omp.h>

#include "isotopicenvelopeutils.h"
#include "../maven/projectDB.h"

using namespace std;

// comand line inputs
static string projectFile = "";
static string sampleDir = "";
static string outputDir = "";
static string adductsFile = "../../src/maven_core/bin/methods/ADDUCTS.csv";

//transient maven data structures
Database DB; // this declaration is necessary for ProjectDB

//persistent maven data structures
static ProjectDB* project = nullptr;
static vector<mzSample*> samples{};
static vector<PeakGroup*> seedPeakGroups{};
static vector<Isotope> isotopes{};

//function declarations
void processOptions(int argc, char* argv[]);
void printUsage();

void loadSamples();
void loadSeedPeakGroups();
void loadIsotopes();

int main(int argc, char *argv[]){
    processOptions(argc, argv);

    loadSamples();
    loadSeedPeakGroups();
    loadIsotopes();

    cout << "All Processes Completed Successfully!" << endl;
}

void processOptions(int argc, char* argv[]) {

    IsotopeProcessorOptions& options = IsotopeProcessorOptions::instance();

    for (unsigned int i = 0; i < static_cast<unsigned int>(argc-1); i++) {
        if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) {
            options.setOptions(argv[i+1]);
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--mzrollDB") == 0) {
            projectFile = argv[i+1];
            project = new ProjectDB(QString(projectFile.c_str()));
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--sampleDir") == 0) {
            sampleDir = argv[i+1];
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--outputDir") == 0) {
            outputDir = argv[i+1];
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--adductsFile") == 0) {
            adductsFile = argv[i+1];
            DB.adductsDB = Adduct::loadAdducts(adductsFile);
        }
    }

    if (!project) {
        cerr << "Required --mzrollDB argument not provided. Exiting." << endl;
        printUsage();
        exit(-1);
    }
}

void printUsage() {
    cout << "Usage: isotopeprocessor <options>\n"
         << "\t-c [--config] <config_file>\n"
         << "\t\tConfiguration file with algorithm options.\n"
         << "\t-m [--mzrollDB] <mzrolldb_file>\n"
         << "\t\tmzrollDB file containing peakgroups for envelope extraction.\n"
         << "\t-s [--sampleDir] <sample_dir>\n"
         << "\t\tDirectory containing sample files referenced in mzrollDB file.\n"
         << "\t-o [--outputDir] <output_dir>\n"
         << "\t\tDirectory to write all output files to.\n"
         << "\t-a [--adductsFile] <adducts_file>\n"
         << "\t\tFile containing adducts information, usually ADDUCTS.csv\n"
         << endl;
}

void loadSamples() {
    //TODO
}

void loadSeedPeakGroups() {

    QSqlQuery query(project->sqlDB);
    query.exec("select DISTINCT groupId, compoundName, adductName from peakgroups where peakgroups.compoundId != '' AND (peakgroups.label LIKE '%g%' OR peakgroups.searchTableName == 'Bookmarks');");

    vector<int> groupIds{};
    vector<string> compoundNames{};
    vector<string> adductStrings{};

    while(query.next()) {
        groupIds.push_back(query.value("groupId").toInt());
        compoundNames.push_back(query.value("compoundName").toString().toStdString());
        adductStrings.push_back(query.value("adductName").toString().toStdString());
    }

    project->alterPeaksTable();
    map<int, vector<Peak>> peaks = project->getAllPeaks(groupIds);

    DB.connect(QString(projectFile.c_str()));
    DB.loadCompoundsSQL("ALL", project->sqlDB);
    map<string, Compound*> compounds = DB.getCompoundsSubsetMap("ALL");

    seedPeakGroups = vector<PeakGroup*>(groupIds.size());

    for (unsigned int i = 0; i < groupIds.size(); i++) {

       int groupId = groupIds.at(i);
       string compoundName = compoundNames.at(i);
       string adductName = adductStrings.at(i);

       vector<Peak> groupPeaks = peaks.at(groupId);
       string compoundId = compoundName + " " + adductName;
       Compound *compound = compounds.at(compoundId);
       Adduct *adduct = DB.findAdductByName(adductName);

//       //debugging
//       cout << "groupId=" << groupId
//            << ", compoundName=" << compoundName
//            << ", adductName=" << adductName
//            << ", # peaks=" << groupPeaks.size()
//            << ", *Compound=" << compound
//            << ", *Adduct=" << adduct
//            << endl;

       //check information
       if (!adduct || !compound) {
           cout << "groupId=" << groupId
                << ", compoundName=" << compoundName
                << ", adductName=" << adductName
                << endl;
           cerr << "Missing Compound or Adduct information! exiting." << endl;
           abort();
       }

       //PeakGroup *group = new PeakGroup();

    }
}

void loadIsotopes() {
    //TODO
}
