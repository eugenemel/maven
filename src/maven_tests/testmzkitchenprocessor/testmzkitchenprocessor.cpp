#include <fstream>
#include <vector>
#include <map>
#include <ctime>
#include <limits.h>
#include <sstream>

#include "mzSample.h"
#include "parallelMassSlicer.h"
#include "../maven/projectDB.h"

#include "../maven/classifierNeuralNet.h"
#include "../maven/database.h"
#include "./Fragment.h"
#include "pugixml.hpp"
#include <sys/time.h>

#include <QtCore>
#include <QCoreApplication>
#include <QPixmap>

#include <stdio.h>
#include <stdlib.h>

//fields
Database DB;
static string mzkitchenMspFile = "";
static string mzrollDBFile = "";

//functions
void processCLIArguments(int argc, char* argv[]);
void mzkitchenSearch();
PeakGroup* getPeakGroupFromDB(int groupId);

int main(int argc, char* argv[]) {
    processCLIArguments(argc, argv);
    mzkitchenSearch();
    cout << "Test Passed" << endl;
}

void processCLIArguments(int argc, char* argv[]){
    for (int i = 1; i < argc ; i++) {
        if (strcmp(argv[i], "--mzkitchenMspFile") == 0) {
            mzkitchenMspFile = argv[i+1];
        } else if (strcmp(argv[i], "--mzrollDBFile") == 0) {
            mzrollDBFile = argv[i+1];
        }
    }
}

void mzkitchenSearch() {
    cout << "Performing mzkitchen msp search on identified peak groups." << endl;
    cout << setprecision(10);

    vector<Compound*> mzkitchenCompounds = DB.loadNISTLibrary(mzkitchenMspFile.c_str());

    sort(mzkitchenCompounds.begin(), mzkitchenCompounds.end(), [](const Compound* lhs, const Compound* rhs){
        return lhs->precursorMz < rhs->precursorMz;
    });

    cout << "MSP spectral library \'" << mzkitchenMspFile << "\' contains " << mzkitchenCompounds.size() << " compounds." << endl;

//    for (auto compound : mzkitchenCompounds) {
//        cout << compound->name << ": mz=" << compound->precursorMz << endl;
//    }

    //Issue 633: Failure to ID bug
    PeakGroup *tmaoPG = getPeakGroupFromDB(50);
}

#include "../maven/projectDB.h"

PeakGroup* getPeakGroupFromDB(int groupId) {
    ProjectDB pdb = ProjectDB(QString(mzrollDBFile.c_str()));

    //pull value from peakgroup

    pdb.closeDatabaseConnection();
}
