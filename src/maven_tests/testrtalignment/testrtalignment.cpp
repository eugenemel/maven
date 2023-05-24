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

    QString mzrollDBFileQ = QString(mzrollDBFile.c_str());

    /*
     * Get PG info
     */
    PeakGroup *histidineIS = ProjectDB::getPeakGroupFromDB(groupIdOfInterest, mzrollDBFileQ);

    float rtmin = histidineIS->peaks.at(0).rtmin;
    float rt = histidineIS->peaks.at(0).rt;
    float rtmax =  histidineIS->peaks.at(0).rtmax;

    cout << "PG #" << groupIdOfInterest
         << ": mz=" << histidineIS->meanMz
         << ": #peaks= " << histidineIS->peakCount()
         << ", rt=" << rt
         << ", rt range=" << rtmin << " - " << rtmax
         << endl;

    cout << "Started importing mzrolldB file: '" << mzrollDBFile << "'..." << endl;

    /*
     * Taken from ProjectDockWidget::loadProjectSQLITE()
     */

    ProjectDB* selectedProject = new ProjectDB(mzrollDBFileQ);

    vector<mzSample*> samples{};
    map<int, mzSample*> sampleById{};

    QSqlQuery query(selectedProject->sqlDB);
    query.exec("select * from samples");
    while (query.next()) {

        QString sname   = query.value("name").toString();
        int sampleId  =   query.value("sampleID").toInt();
        int sampleOrder  = query.value("sampleOrder").toInt();

        QString fullPath = QString(sampleDir.c_str()).append("/").append(sname);

        mzSample *sample = new mzSample();
        sample->loadSample(fullPath.toStdString().c_str());
        sample->setSampleId(sampleId);
        sample->setSampleOrder(sampleOrder);

        sampleById.insert(make_pair(sampleId, sample));
        samples.push_back(sample);

        cout << "Loaded Sample: '" << sname.toStdString() << "'" << endl;
    }


    /*
     * Taken from ProjectDockWidget::doAlignment()
     */
    query.exec("select S.name, S.sampleId, R.* from rt_update_key R, samples S where S.sampleId = R.sampleId ORDER BY sampleId, rt");

    AlignmentSegment* lastSegment = nullptr;
    Aligner aligner;
    aligner.setSamples(samples);

     int segCount=0;
     while (query.next()) {

        string sampleName = query.value("name").toString().toStdString();
        int sampleId =   query.value("sampleId").toString().toInt();
        mzSample* sample = sampleById.at(sampleId);
        if (!sample) continue;
        segCount++;

        AlignmentSegment* seg = new AlignmentSegment();
        seg->sampleName   = sampleName;
        seg->seg_start = 0;
        seg->seg_end   = query.value("rt").toString().toFloat();
        seg->new_start = 0;
        seg->new_end   = query.value("rt_update").toString().toFloat();

       if (lastSegment and lastSegment->sampleName == seg->sampleName) {
           seg->seg_start = lastSegment->seg_end;
           seg->new_start = lastSegment->new_end;
       } else {
            cout << endl;
       }

        //debugging
        cout << sample->sampleName << "\t" << seg->seg_end << "\t" << seg->new_end << endl;

        aligner.addSegment(sample->sampleName,seg);
        lastSegment = seg;
       }

     aligner.doSegmentedAligment();

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
