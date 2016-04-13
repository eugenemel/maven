#ifndef PROJECTDB_H
#define PROJECTDB_H

#include <QtCore>
#include <QtSql>
#include <QDebug>
#include <QString>
#include "mzSample.h"
#include "database.h"

class Database;
extern Database DB;

class ProjectDB {

	public: 
            ProjectDB(QString dbfilename);

            QSqlDatabase sqlDB;

            vector<PeakGroup> allgroups;
            vector<mzSample*> samples;

            void clearLoadedPeakGroups() { allgroups.clear(); }
            void saveSamples(vector<mzSample *> &sampleSet);
            void saveGroups(vector<PeakGroup>   &allgroups, QString setName);
            void writeSearchResultsToDB();
            int writeGroupSqlite(PeakGroup* group, int parentGroupId, QString tableName);
            void loadPeakGroups(QString tableName);
            void loadGroupPeaks(PeakGroup* group);
            void deleteAll();
            void deletePeakGroup(PeakGroup* g, QString tableName);
            QStringList getSearchTableNames();


            void setSamples(vector<mzSample*>set) { samples = set; }
            bool isOpen() { return sqlDB.isOpen(); }
            QString databaseName() { return sqlDB.databaseName(); }

            bool openDatabaseConnection(QString dbname);
            bool closeDatabaseConnection();


};

#endif
