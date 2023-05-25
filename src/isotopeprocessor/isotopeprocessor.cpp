#include <iostream>
#include <omp.h>

#include "isotopicenvelopeutils.h"
#include "../maven/projectDB.h"

using namespace std;

// static variables - CL inputs
Database DB; // this declaration is necessary for ProjectDB
static ProjectDB* project = nullptr;
static vector<mzSample*> samples{};
static string sampleDir = "";
static string outputDir = "";

//function declarations
void processOptions(int argc, char* argv[]);
void printUsage();

int main(int argc, char *argv[]){
    processOptions(argc, argv);

    cout << "All Processes Completed Successfully!" << endl;
}

void processOptions(int argc, char* argv[]) {

    IsotopeProcessorOptions& options = IsotopeProcessorOptions::instance();

    for (unsigned int i = 0; i < static_cast<unsigned int>(argc-1); i++) {
        if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) {
            options.setOptions(argv[i+1]);
        } else if (strcmp(argv[i], "-m") || strcmp(argv[i], "--mzrollDB") == 0) {
            project = new ProjectDB(QString(argv[i+1]));
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--sampleDir") == 0) {
            sampleDir = argv[i+1];
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--outputDir") == 0) {
            outputDir = argv[i+1];
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
         << endl;
}
