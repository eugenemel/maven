#include <iostream>
#include <omp.h>

#include "proteinutils.h"

void processOptions(int argc, char *argv[]);
void printUsage();
void printArguments();
void debuggingTestCases();

void writeFastaFile(vector<FastaWritable*> entries, string outputFile, unsigned int seqLineMax);

//TODO: port this to Protein:: method
vector<ProteinFragment*> fragmentProtein(Protein* protein, vector<double>& fragMasses, double tolerance, bool debug);

using namespace std;

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

    //writeFastaFile(proteinFragments, outputFile, 87, true);
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

//TODO: move this to Protein::fragmentProtein(double tolerance)
vector<ProteinFragment*> fragmentProtein(Protein* protein, vector<double>& fragMasses, double tolerance, bool debug) {

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

                if (abs(m1-mass) < tolerance && i == 0) {
                    ProteinFragment *fragment = new ProteinFragment(protein, mass, m1, 0, (i-1));
                    fragmentProteins.push_back(fragment);
                    if (debug) cout << "(" << (i-1) << ", " << 0 << "): mass=" << mass << ", M1=" << m1 << "Da, delta=" << abs(m1-mass) << endl;
                }

                if (abs(m2Cut2-mass) < tolerance) {
                    ProteinFragment *fragment = new ProteinFragment(protein, mass, m2Cut2, i, (j-1));
                    fragmentProteins.push_back(fragment);
                    if (debug) cout << "(" << i << ", " << (j-1) << "): mass="<< mass << ", M2=" << m2Cut2 << " Da, delta=" << abs(m2Cut2-mass) << endl;
                }

                if (abs(m3-mass) < tolerance && i == 0) {
                    ProteinFragment *fragment = new ProteinFragment(protein, mass, m3, j, protein->seq.length()-1);
                    fragmentProteins.push_back(fragment);
                    if (debug) cout << "(" << i << ", " << (protein->seq.length()-1) << "): mass="<< mass << ", M3=" << m3 << " Da, delta=" << abs(m3-mass) << endl;
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

void writeFastaFile(vector<FastaWritable*> entries, string outputFile, unsigned int seqLineMax, bool debug) {
    ofstream outputFileStream;
    outputFileStream.open(outputFile);

    for (FastaWritable *entry : entries) {
        outputFileStream << ">" << entry->getHeader() << "\n";

        string::size_type N = entry->getSequence().size();
        string::size_type currentPos = 0;

        while (currentPos != string::npos) {

            if (debug) cout << "currentPos: " << currentPos << endl;

            if (currentPos+seqLineMax < N) {

                if (debug) cout << "currentPos+seqLineMax: " << (currentPos+seqLineMax) << endl;
                if (debug) cout << "substring: " << entry->getSequence().substr(currentPos, seqLineMax) << endl;

                outputFileStream << entry->getSequence().substr(currentPos, seqLineMax)
                                 << "\n";

                currentPos = currentPos + seqLineMax + 1;
            } else {
                string::size_type remainder = N-currentPos;
                outputFileStream << entry->getSequence().substr(currentPos, remainder) << "\n";

                if (debug) cout << "remainder: " << remainder;
                if (debug) cout << "substring: " << entry->getSequence().substr(currentPos, remainder) << endl;

                break;
            }
        }

        outputFileStream << "\n";
    }

    outputFileStream.close();
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
