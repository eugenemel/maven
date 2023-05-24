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
#include "../maven/projectDB.h"
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
static string mzrollDBFile = "";
static string sampleDir = "";
static int groupIdOfInterest = -1;

//functions
void processCLIArguments(int argc, char* argv[]);

int main(int argc, char* argv[]) {
    processCLIArguments(argc, argv);

    cout << "Started importing mzrolldB file: '" << mzrollDBFile << "'..." << endl;

    ProjectDB* selectedProject = new ProjectDB(QString(mzrollDBFile.c_str()));

    vector<mzSample*> samples{};

    QSqlQuery query(selectedProject->sqlDB);
    query.exec("select * from samples");
    while (query.next()) {
        QString sname   = query.value("name").toString();
        QString fullPath = QString(sampleDir.c_str()).append("/").append(sname);

        mzSample *sample = new mzSample();
        sample->loadSample(fullPath.toStdString().c_str());

        cout << "Loaded Sample: '" << sname.toStdString() << "'" << endl;
    }

    cout << "Finished importing mzrolldB file: '" << mzrollDBFile << "'" << endl;

    cout << "Test Passed" << endl;
}

void processCLIArguments(int argc, char* argv[]){
    for (int i = 1; i < argc ; i++) {
        if (strcmp(argv[i], "--mzrollDBFile") == 0) {
            mzrollDBFile = argv[i+1];
        } else if (strcmp(argv[i], "--groupId") == 0) {
            groupIdOfInterest = stoi(argv[i+1]);
        } else if (strcmp(argv[i], "--sampleDir") == 0) {
            sampleDir = argv[i+1];
        }
    }
}
