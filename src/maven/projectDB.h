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
            void saveCompounds(vector<PeakGroup> &allgroups);
            void saveCompounds(set<Compound*>&compounds);
            void writeSearchResultsToDB();
            int writeGroupSqlite(PeakGroup* group, int parentGroupId, QString tableName);
            void loadPeakGroups(QString tableName);
            void loadGroupPeaks(PeakGroup* group);
            void deleteAll();
            void deleteGroups();
            void deleteSearchResults(QString searchTable);
            void deletePeakGroup(PeakGroup* g, QString tableName);
            void assignSampleIds();
            void saveAlignment();
            QStringList getSearchTableNames();


            void setSamples(vector<mzSample*>set) { samples = set; }
            bool isOpen() { return sqlDB.isOpen(); }
            QString databaseName() { return sqlDB.databaseName(); }

            bool openDatabaseConnection(QString dbname);
            bool closeDatabaseConnection();

	    string getScanSigniture(Scan* scan, int limitSize);
	    void saveScans(vector<mzSample *> &sampleSet);

	    mzSample* getSampleById( int sampleId);
	    void doAlignment();

	    QString  projectPath();
	    QString  projectName();
        static QString locateSample(QString filepath, QStringList pathlist);

	    void loadSamples();
	    vector<mzSample*> getSamples() { return samples; }
};

#endif
