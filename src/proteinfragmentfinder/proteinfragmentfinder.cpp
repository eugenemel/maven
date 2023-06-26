#include <iostream>
#include <omp.h>

#include "mzSample.h"
#include "proteinutils.h"

void processOptions(int argc, char *argv[]);
void printUsage();
void printArguments();

//TODO: port this to Protein:: method
vector<Protein*> fragmentProtein(Protein* protein, double tolerance);

using namespace std;

static string versionNum = "0.0.1";
static string inputFile = "";
static string outputFile = "";
static double tolerance = 10.0; // Da
static vector<double> masses{};

int main(int argc, char *argv[]){
    processOptions(argc, argv);

    vector<Protein*> proteins = Protein::loadFastaFile(inputFile);

    vector<Protein*> proteinFragments{};

    for (auto p : proteins) {
        p->printSummary();

        //TODO: fragment proteins
        vector<Protein*> pFragments = fragmentProtein(p, tolerance);

        //add results to all protein fragments
        proteinFragments.reserve(proteinFragments.size() + pFragments.size());
        proteinFragments.insert(proteinFragments.end(), pFragments.begin(), pFragments.end());

    }

    Protein::writeFastaFile(proteinFragments, outputFile);

    cout << "All Processes Completed Successfully!" << endl;
}

void processOptions(int argc, char* argv[]) {

    for (unsigned int i = 0; i < static_cast<unsigned int>(argc-1); i++) {
        if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--input") == 0) {
            inputFile = argv[i+1];
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--tolerance") == 0) {
            tolerance = stod(argv[i+1]);
        } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--outputFile") == 0) {
            outputFile = argv[i+1];
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

    if (inputFile == "" || outputFile == "" || masses.empty()) {
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
         << "\t-o [--outputFile] <output_file>\n"
         << "\t\tDirectory to write all output files to.\n"
         << "\t-t [--tolerance] <mass_tolerance>\n"
         << "\t\tMass tolerance information."
         << endl;
}

void printArguments() {
    cout << "=======================================================\n"
         << "proteinfragmentfinder version " << versionNum << ":\n"
         << "\tInput File:" << inputFile << "\n"
         << "\tMasses: ";

    for (unsigned int i = 0; i < masses.size(); i++) {
        if (i > 0) cout << ", ";
        cout << masses[i] << " Da";
    }
    cout << endl;

    cout << "\tTolerance: " << tolerance << " Da\n"
         << "\tOutput Directory:" << outputFile << "\n"
         << "======================================================="
         << endl;
}

//TODO: move this to Protein::fragmentProtein(double tolerance)
vector<Protein*> fragmentProtein(Protein* protein, double tolerance) {
    unsigned long cut1 = 0;
    unsigned long cut2 = protein->seq.length();

    vector<Protein*> fragmentProteins{};

    /**
     * Progressively examine the pieces of a protein made by cutting at two places,
     * 'cut1', from the C terminus, and 'cut2', from the N terminus.
     *
     * Once protein fragments exceed certain size ranges, stop examining possibilities
     * for a given set of cuts.
     *
     *
     **/
    for (unsigned long i = cut1; i < protein->seq.length(); i++) {
        for (unsigned long j = cut2; j > 0; j--) {
            //TODO
        }
    }

    return fragmentProteins;
}
