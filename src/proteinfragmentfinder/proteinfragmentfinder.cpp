#include <iostream>
#include <omp.h>

#include "proteinutils.h"

void processOptions(int argc, char *argv[]);
void printUsage();
void printArguments();
void debuggingTestCases();

static string versionNum = "0.0.1";
static string inputFile = "";
static string outputFile = "";
static double tolerance = 10.0; // Da
static vector<double> masses{};

int main(int argc, char *argv[]){
    processOptions(argc, argv);

    vector<Protein*> proteins = ProteinUtils::loadFastaFile(inputFile);

    vector<FastaWritable*> proteinFragments{};

    for (auto p : proteins) {
        p->printSummary();

        vector<ProteinFragment*> pFragments = p->fragmentProtein(masses, tolerance, false);

        //add results to all protein fragments
        proteinFragments.reserve(proteinFragments.size() + pFragments.size());
        proteinFragments.insert(proteinFragments.end(), pFragments.begin(), pFragments.end());

    }

    ProteinUtils::writeFastaFile(proteinFragments, outputFile);

    // debuggingTestCases();

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

void debuggingTestCases() {

    //test case 1
    Protein *protein = new Protein("dummy", "PEPTIDER");

    for (unsigned int i = 0; i < protein->getSequence().size(); i++) {
        cout << "i=" << i << ": "
             << protein->getSequence().substr(i)  << ": "
             << ProteinUtils::getProteinMass(protein->getSequence().substr(i)) << " Da."
             << endl;
    }


    vector<double> masses{
        ProteinUtils::getProteinMass("PEP"), // from beginning
        ProteinUtils::getProteinMass("TIDE"), //middle
        ProteinUtils::getProteinMass("PTIDER"), //to end
    };

    vector<ProteinFragment*> fragments = protein->fragmentProtein(masses, 1, true);

    for (auto frag : fragments) {
        cout << frag->getHeader()  << ": " << frag->getSequence() << endl;
    }

    //test case 2

    Protein *protein2 = new Protein("dummy", "PEPTIDERPEPTIDER");

    vector<double> masses2{
        ProteinUtils::getProteinMass("PEP"), // from beginning 2x
        ProteinUtils::getProteinMass("TIDE"), //middle 2x
        ProteinUtils::getProteinMass("PTIDER"), //to end 2x
        ProteinUtils::getProteinMass("PEPTIDER"), // from beginning
        ProteinUtils::getProteinMass("TIDERPEPTI"), //middle
        ProteinUtils::getProteinMass("TIDERPEPTIDER"), //to end
        ProteinUtils::getProteinMass("PEPTIDERPEPTIDE"), //to end
        ProteinUtils::getProteinMass("EPTIDERPEPTIDER"), //to end
    };

    vector<ProteinFragment*> fragments2 = protein2->fragmentProtein(masses2, 1, true);

    for (auto frag : fragments2) {
        cout << frag->getHeader()  << ": " << frag->getSequence() << endl;
    }

}
