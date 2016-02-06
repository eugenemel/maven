#ifndef PROJECTDB_H
#define PROJECTDB_H

#include <QtCore>
#include <QtSql>
#include <QDebug>
#include <QString>
#include "mzSample.h"

class ProjectDB {

	public: 
            QSqlDatabase projectDB;
            void saveSamples(vector<mzSample *> &sampleSet);
            void saveGroups(vector<PeakGroup>   &allgroups);
            void writeSearchResultsToDB();
            void writeGroupSqlite(PeakGroup* g);

            ProjectDB( QString dbfilename) { openProjectDB(dbfilename); }

            void openProjectDB(QString fileName) {
                projectDB = QSqlDatabase::addDatabase("QSQLITE", "projectDB");
                projectDB.setDatabaseName(fileName);
                projectDB.open();
                if (!projectDB.isOpen()) { qDebug()  << "Failed to open " + fileName; }

            }
};

#endif
