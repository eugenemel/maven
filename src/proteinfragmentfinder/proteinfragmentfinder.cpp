#include <iostream>
#include <omp.h>

#include "mzSample.h"

void processOptions(int argc, char *argv[]);
void printUsage();
void printArguments();

using namespace std;

static string versionNum = "0.0.1";
static string inputFile = "";
static string outputDir = "";
static double tolerance = 10.0; // Da
static vector<double> masses{};

int main(int argc, char *argv[]){
    processOptions(argc, argv);

    cout << "All Processes Completed Successfully!" << endl;
}

void processOptions(int argc, char* argv[]) {

    for (unsigned int i = 0; i < static_cast<unsigned int>(argc-1); i++) {
        if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--input") == 0) {
            inputFile = argv[i];
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--tolerance") == 0) {
            tolerance = stod(argv[i+1]);
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--outputDir") == 0) {
            outputDir = argv[i+1];
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--masses") == 0) {
            vector<string> massesStrings{};
            mzUtils::split(argv[i+1], ",", massesStrings);
            for (string massString : massesStrings) {
                try {
                    double mass = stod(massString);
                    masses.push_back(mass);
                } catch (exception e) {
                    //ignore badly formatted entries
                }
            }
        }
    }

    if (inputFile == "" || outputDir == "" || masses.empty()) {
        printUsage();
        exit(-1);
    }

    printArguments();
}

void printUsage() {
    cout << "Usage: proteinfragmentfinder <options>\n"
         << "\t-m [--masses] <comma-separated-list of masses>\n"
         << "\t\te.g. 10000,20000,3000\n"
         << "\t-i [--input] <fasta_file>\n"
         << "\t\tFasta file containing one or more protein sequences.\n"
         << "\t-o [--outputDir] <output_dir>\n"
         << "\t\tDirectory to write all output files to.\n"
         << "\t-t [--tolerance] <mass_tolerance>\n"
         << "\t\tMass tolerance information."
         << endl;
}

void printArguments() {
    cout << "=======================================================\n"
         << "proteinfragmentfinder version " << versionNum << ":"
         << "\tInput File:" << inputFile << "\n"
         << "\tMasses:" << endl;

    for (unsigned int i = 0; i < masses.size(); i++) {
        if (i > 0) cout << ", ";
        cout << masses[i];
    }
    cout << endl;

    cout << "\tTolerance:" << tolerance << "\n"
         << "\tOutput Directory:" << outputDir << "\n"
         << "======================================================="
         << endl;
}
