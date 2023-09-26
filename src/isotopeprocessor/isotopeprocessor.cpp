#include <iostream>
// #include <omp.h>

#include "isotopicenvelopeutils.h"
#include "../maven/projectDB.h"

using namespace std;

// comand line inputs
string projectFile = "";
string sampleDir = "";
string outputDir = "";
string adductsFile = "../../src/maven_core/bin/methods/ADDUCTS.csv";
string configInfo = ""; // can either be encoded string of parameters, or a file containing parameters.

//transient maven data structures
Database DB; // this declaration is necessary for ProjectDB

//persistent maven data structures
ProjectDB* project = nullptr;
vector<PeakGroup*> seedPeakGroups{};
IsotopeParameters params;

//function declarations
void processOptions(int argc, char* argv[]);
void printUsage();

void loadSeedPeakGroups();

int main(int argc, char *argv[]){
    processOptions(argc, argv);

    project->loadSamples(QString(sampleDir.c_str())); //single-threaded, ensure that IDs match

    loadSeedPeakGroups();

    vector<IsotopicEnvelopeGroup> isotopicEnvelopeGroups{};

    for (unsigned int i = 0; i < seedPeakGroups.size(); i++) {

        PeakGroup *group = seedPeakGroups[i];
        Compound *compound = group->compound;
        Adduct *adduct = group->adduct;

        //dummy test: 12C, 13C*1, 13C*2, 13C*3
        vector<Isotope> theoreticalIsotopes = MassCalculator::computeIsotopes(
                    compound->formula,
                    adduct,
                    3,
                    true, //isUse13C,
                    false, // isUse15N,
                    false, // isUse34S
                    false // isUse2H
                    );

        //Combine isotopes based on parameters (e.g, resolving power)
        vector<Isotope> isotopes = IsotopicEnvelopeAdjuster::condenseTheoreticalIsotopes(
            theoreticalIsotopes,
            params,
            false
            );

        IsotopicEnvelopeGroup isotopicEnvelopeGroup = IsotopicEnvelopeExtractor::extractEnvelopes(
            compound,
            adduct,
            group,
            theoreticalIsotopes,
            project->samples,
            params,
            false
            );

        isotopicEnvelopeGroup.print();
        isotopicEnvelopeGroups.push_back(isotopicEnvelopeGroup);

        //Update the children of the peakgroup to match this value
        isotopicEnvelopeGroup.setIsotopesToChildrenPeakGroups();

    }

    vector<PeakGroup> envelopeExtractedPeakGroups(seedPeakGroups.size());
    transform(seedPeakGroups.begin(), seedPeakGroups.end(), envelopeExtractedPeakGroups.begin(), [](PeakGroup* pg){return *pg;});

    //save results
    project->deleteGroups(false);
    project->saveGroups(envelopeExtractedPeakGroups, "Isotope Processor");
    project->saveSamples(project->samples);

    //save parameters
    string isotopeParameters = params.encodeParams();
    QString key = QString("isotopic_envelopes");
    QString value = QString(isotopeParameters.c_str());
    map<QString, QString> parametersData{make_pair(key, value)};
    project->savePeakGroupsTableData(parametersData);

    cout << "All Processes Completed Successfully!" << endl;
}

void processOptions(int argc, char* argv[]) {

    for (unsigned int i = 0; i < static_cast<unsigned int>(argc-1); i++) {
        if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) {
            configInfo = argv[i+1];
            if (configInfo.find(".ini") != std::string::npos) {
                // TODO: parse configuration file
            } else {
                params = IsotopeParameters::decode(configInfo);
            }
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--mzrollDB") == 0) {
            projectFile = argv[i+1];
            project = new ProjectDB(QString(projectFile.c_str()));
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--sampleDir") == 0) {
            sampleDir = argv[i+1];
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

    if (sampleDir == "") {
        cerr << "Required --sampleDir argument no provided. Exiting." << endl;
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
         << "\t\tisotopic peak groups will be addd as new 'isotope processor' table of results.\n"
         << "\t-s [--sampleDir] <sample_dir>\n"
         << "\t\tDirectory containing sample files referenced in mzrollDB file.\n"
         << "\t\tWhether or not the argument is provided, envelopes will be saved into the original mzrollDB file.\n"
         << "\t-a [--adductsFile] <adducts_file>\n"
         << "\t\tFile containing adducts information, usually ADDUCTS.csv\n"
         << endl;
}

/**
 * @brief loadSeedPeakGroups
 * Currently, all peak groups that are marked as 'good' or are in the Bookmarks table are slated
 * for re-extraction.  However, this could be generalized/adjusted as needed.
 */
void loadSeedPeakGroups() {

    QSqlQuery query(project->sqlDB);
    query.exec("select DISTINCT groupId, compoundName, adductName from peakgroups where peakgroups.compoundId != '' "
               "AND (peakgroups.label LIKE '%g%' OR peakgroups.searchTableName == 'Bookmarks')"
               "AND peakgroups.type == 0;"); // type == 0 is the default type. type == 4 is the isotope type, which we ignore.

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

       //check information
       if (!adduct || !compound) {
           cout << "groupId=" << groupId
                << ", compoundName=" << compoundName
                << ", adductName=" << adductName
                << endl;
           cerr << "Missing Compound or Adduct information! exiting." << endl;
           abort();
       }

       PeakGroup *group = new PeakGroup();
       group->groupId = groupId;
       group->peaks = groupPeaks;
       group->compound = compound;
       group->adduct = adduct;
       group->groupStatistics();

       cout << group->compound->name << " "
            << group->adduct->name << " "
            << group->peakCount() << " peaks, peaks[0] *sample: "
            << group->peaks.at(0).sample
            << endl;

       seedPeakGroups[i] = group;

    }
}
