#ifndef PROJECTDB_H
#define PROJECTDB_H

#include <QtCore>
#include <QtSql>
#include <QDebug>
#include <QString>
#include "mzSample.h"
#include "database.h"
#include "directinfusionprocessor.h"
#include <memory>
#include "isotopicenvelopeutils.h"

class Database;
extern Database DB;

struct mzrollDBMatch;

class ProjectDB {

	public: 
            ProjectDB(QString dbfilename);

            //primarily used for test code
            static PeakGroup* getPeakGroupFromDB(int groupId, QString dbfilename);

            QSqlDatabase sqlDB;

            vector<PeakGroup> allgroups;
            vector<mzSample*> samples;

            //both bookmarks and rumsDB can have connections to matches table.
            map<int, shared_ptr<mzrollDBMatch>> topMatch = {};
            multimap<int, shared_ptr<mzrollDBMatch>> allMatches = {};
            map<int, int> rumsDBOldToNewGroupIDs = {};
            map<int, int> bookmarksOldToNewGroupIDs = {};
            map<string, shared_ptr<DirectInfusionSearchParameters>> diSearchParameters = {};
            map<string, shared_ptr<PeaksSearchParameters>> peaksSearchParameters = {};
            map<string, string> genericSearchParameters = {};

            //Issue 641: explicitly save anchor points, when they are available.
            map<mzSample*, vector<pair<float, float>>> sampleToUpdatedRts{};

            //UI adjustments
            unordered_map<string, string> quantTypeMap{}; //<original, renamed>
            unordered_map<string, string> quantTypeInverseMap{}; //<renamed, original>

            //Issue 768: if peakgroup-associated MS2 scans need to be explicitly saved in a dedicated table
            bool isSaveMs2Scans = false;

            void clearLoadedPeakGroups() { allgroups.clear(); }
            void saveSamples(vector<mzSample *> &sampleSet);
            void saveGroups(vector<PeakGroup>   &allgroups, QString setName, bool isSaveMs2Scans=false);
            void saveCompounds(vector<PeakGroup> &allgroups);
            void saveCompounds(set<Compound*>&compounds);
            void saveMatchTable();
            void saveUiTable();
            void savePeakGroupsTableData(map<QString, QString> searchTableData);
            void writeSearchResultsToDB();
            int writeGroupSqlite(PeakGroup* group, int parentGroupId, QString tableName, bool isSaveMs2Scans=false);

            void loadPeakGroups(QString tableName, QString rumsDBLibrary, bool isAttemptToLoadDB=true, const map<int, vector<Peak>>& peakGroupMap={}, Classifier *classifier=nullptr);
            void loadGroupPeaks(PeakGroup* group);
            map<int, vector<Peak>> getAllPeaks(vector<int> groupIds = vector<int>(0));
            vector<Peak> getPeaks(int groupId);
            void loadMatchTable();
            void loadSearchParams();
            void loadUIOptions();

            void deleteAll();
            void deleteGroups(bool isDeleteSearchParams = true);
            void deleteSearchResults(QString searchTable);
            void deletePeakGroup(PeakGroup* g, QString tableName);
            void assignSampleIds();
            void saveAlignment();
            void saveAlignment(map<mzSample*, vector<pair<float, float>>>& sampleToUpdatedRts);
            QStringList getSearchTableNames();
            QStringList getProjectFileDatabaseNames();

            void setSamples(vector<mzSample*>set) { samples = set; }
            bool isOpen() { return sqlDB.isOpen(); }
            QString databaseName() { return sqlDB.databaseName(); }

            bool openDatabaseConnection(QString dbname);
            bool closeDatabaseConnection();

        string getScanSignature(Scan* scan, int limitSize);
	    void saveScans(vector<mzSample *> &sampleSet);
        void saveGroupScans(vector<Scan*>& scans, int groupId);

	    mzSample* getSampleById( int sampleId);
	    void doAlignment();

	    QString  projectPath();
	    QString  projectName();
        static QString locateSample(QString filepath, QStringList pathlist);

        void loadSamples(QString sampleDir="");
	    vector<mzSample*> getSamples() { return samples; }

        void addPeaksTableColumn(QString columnName, QString columnType="real", QString numericDefault="0");
        void alterPeaksTable();

        void alterPeakGroupsTable();
};

struct mzrollDBMatch {
    int groupId;
    int ionId;
    string compoundName;
    string adductName;
    int summarizationLevel;
    string originalCompoundName;
    float score;
    bool is_match;

    mzrollDBMatch(int groupIdVal,
                  int ionIdVal,
                  string compoundNameVal,
                  string adductNameVal,
                  int summarizationLevelVal,
                  string originalCompoundNameVal,
                  float scoreVal,
                  bool is_matchVal) {

        groupId = groupIdVal;
        ionId = ionIdVal;
        compoundName = compoundNameVal;
        adductName = adductNameVal;
        summarizationLevel = summarizationLevelVal;
        originalCompoundName = originalCompoundNameVal;
        score = scoreVal;
        is_match = is_matchVal;
    }

    int isMatchAsInt() {return is_match ? 1: 0;}
};

typedef multimap<int, shared_ptr<mzrollDBMatch>>::iterator matchIterator;

#endif
