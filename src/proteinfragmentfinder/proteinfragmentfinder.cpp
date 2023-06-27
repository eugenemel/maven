#include <iostream>
#include <omp.h>

#include "mzSample.h"
#include "proteinutils.h"

void processOptions(int argc, char *argv[]);
void printUsage();
void printArguments();

//TODO: port this to Protein:: method
vector<ProteinFragment*> fragmentProtein(Protein* protein, vector<double>& fragMasses, double tolerance);

using namespace std;

static string versionNum = "0.0.1";
static string inputFile = "";
static string outputFile = "";
static double tolerance = 10.0; // Da
static vector<double> masses{};

int main(int argc, char *argv[]){
    processOptions(argc, argv);

    vector<Protein*> proteins = Protein::loadFastaFile(inputFile);

    vector<ProteinFragment*> proteinFragments{};

    for (auto p : proteins) {
        p->printSummary();

        //TODO: fragment proteins
        vector<ProteinFragment*> pFragments = fragmentProtein(p, masses, tolerance);

        //add results to all protein fragments
        proteinFragments.reserve(proteinFragments.size() + pFragments.size());
        proteinFragments.insert(proteinFragments.end(), pFragments.begin(), pFragments.end());

    }

    for (auto frag : proteinFragments) {
        cout << frag->getHeader() << endl;
    }

    //TODO: Fix this
    //FastaWritable::writeFastaFile(proteinFragments, outputFile);
    //Protein::writeFastaFile(proteinFragments, outputFile);

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
vector<ProteinFragment*> fragmentProtein(Protein* protein, vector<double>& fragMasses, double tolerance) {

    /**
     * Progressively examine the pieces of a protein made by cutting at two places,
     * 'cut1', from the C terminus, and 'cut2', from the N terminus.
     *
     * Once protein fragments exceed certain size ranges, stop examining possibilities
     * for a given set of cuts.
     *
     *       M1     M2   M3
     *  N ------|------|------------ C
     *          cut1   cut2
     *
     * Progression: cut1 starts at the N terminus, cut2 starts at the C terminus,
     * cut1 progressively increases to the right while cut2 progressively increases to the left.
     * So, M1 and M3 progressively increases, while M2 fluctuates, but tends to decrease as M1
     * increases.
     *
     * The reason for all of this complexity is to know when to avoid making unnecessary comparisons
     * (stop comparing)
     **/

    sort(fragMasses.begin(), fragMasses.end(), [](const double& lhs, const double& rhs){
        return lhs < rhs;
    });

    //initialize output
    vector<ProteinFragment*> fragmentProteins{};

    //initial state
    double m1 = 0.0;
    double m2 = protein->mw;
    double m2Cut2 = m2;
    double m3 = 0.0;

    for (unsigned long i = 0; i < protein->seq.length(); i++) {

        //update temp variables in preparation for iteration
        m2Cut2 = m2;

        for (unsigned long j = protein->seq.length()-1; j > i; j--) {

            //second cut affects m2 and m3 values
            m2Cut2 -= aaMasses.at(protein->seq[j]);
            m3 += aaMasses.at(protein->seq[j]);

            for (double mass : fragMasses) {

                if (abs(m1-mass) < tolerance) {
                    ProteinFragment *fragment = new ProteinFragment(protein, mass, m1, 0, i);
                    fragmentProteins.push_back(fragment);
                    cout << "(" << i << ", " << 0 << "): mass=" << mass << ", M1=" << m1 << "Da, delta=" << abs(m1-mass) << endl;
                }

                if (abs(m2Cut2-mass) < tolerance) {
                    ProteinFragment *fragment = new ProteinFragment(protein, mass, m2Cut2, i, j);
                    fragmentProteins.push_back(fragment);
                    cout << "(" << i << ", " << j << "): mass="<< mass << ", M2=" << m2Cut2 << " Da, delta=" << abs(m2Cut2-mass) << endl;
                }
                if (abs(m3-mass) < tolerance) {
                    ProteinFragment *fragment = new ProteinFragment(protein, mass, m3, j, protein->seq.length()-1);
                    fragmentProteins.push_back(fragment);
                    cout << "(" << i << ", " << j << "): mass="<< mass << ", M3=" << m3 << " Da, delta=" << abs(m3-mass) << endl;
                }
            }
        }

        //first cut affects m1 and m2 values
        m1 += aaMasses.at(protein->seq[i]);
        m2 -= aaMasses.at(protein->seq[i]);
        m3 = 0.0;
    }

    sort(fragmentProteins.begin(), fragmentProteins.end(), [](ProteinFragment* lhs, ProteinFragment* rhs){
        if (lhs->theoreticalMw == rhs->theoreticalMw) {
            return lhs->deltaMw < rhs->deltaMw;
        } else {
            return lhs->theoreticalMw < rhs->theoreticalMw;
        }
    });

    return fragmentProteins;
}
