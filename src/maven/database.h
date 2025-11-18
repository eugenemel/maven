#ifndef DATABASE_H
#define DATABASE_H

#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QRegExp>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QIcon>

#include "mzSample.h"
#include "mzUtils.h"
#include "mzMassCalculator.h"

class PeakGroupTag;

class Database {

    public:
        Database() {
            //empty constructor
        }

        Database(QString filename) {
            connect(filename);
            reloadAll();
        }
        //~Database() { closeAll(); }

		QSqlDatabase& getLigandDB() { return ligandDB; }
        void reloadAll();  //loads all tables
        void closeAll();
        bool connect(QString filename);
        bool disconnect();
    
        vector<Adduct*> loadAdducts(string filename);
        vector<Adduct*> defaultAdducts();

        void loadMethodsFolder(QString methodsFolder);
        int  loadCompoundsFile(QString filename);
        static vector<Compound*> loadCompoundCSVFile(QString fileName, bool debug=false);
        static vector<Compound*> loadNISTLibrary(QString fileName);

        void loadCompoundsSQL(QString databaseName,QSqlDatabase& dbConnection);
        void saveCompoundsSQL(vector<Compound*> &compoundSet, QSqlDatabase& dbConnection, bool isRespectCIDs=true);
        void deleteCompoundsSQL(QString dbName, QSqlDatabase& dbConnection);
        void deleteAllCompoundsSQL(QSqlDatabase& dbConnection);
        void unloadCompounds(QString databaseName);
        void unloadAllCompounds();

		multimap<string,Compound*> keywordSearch(string needle);
		vector<string>   getCompoundReactions(string compound_id);

		void addCompound(Compound*c);
        void loadReactions(string modelName);

        vector<Compound*> getCompoundsSubset(string database);
        map<string, Compound*> getCompoundsSubsetMap(string dbName);

        QStringList getDatabaseNames();
        QStringList getLoadedDatabaseNames() { return loadedDatabase.keys(); }
        int getLoadedDatabaseCount(QString databaseName){return loadedDatabase[databaseName];}
        map<string,int>   getChromotographyMethods();
        QStringList getAdductNames(QString dbName);

        Compound* findSpeciesById(string id, string db, bool attemptToLoadDB=true);
		Compound* findSpeciesByPrecursor(float precursorMz, float productMz,int polarity,double amuQ1, double amuQ3);
        set<Compound*> findSpeciesByMass(float mz, float ppm);
        vector<Compound*> findSpeciesByName(string name, string dbname, bool attemptToLoadDB=true);
        Compound* findSpeciesByNameAndAdduct(string name, string adduct, string dbname);

        vector<MassCalculator::Match> findMatchingCompounds(float mz, float ppm, float charge);
        vector<MassCalculator::Match> findMatchingCompoundsSLOW(float mz, float ppm, float charge);

        Adduct* findAdductByName(string id);

		void loadRetentionTimes(QString method);
		void saveRetentionTime(Compound* c, float rt, QString method);

        void saveValidation(Peak* p);

        void loadPeakGroupTags(string filename);

        vector<Adduct*> adductsDB;
        vector<Adduct*> fragmentsDB;
        vector<Compound*> compoundsDB;
        std::map<char, PeakGroupTag*> peakGroupTags;

        vector<PeakGroupTag*> getSupplementalPeakGroupTags();

        vector<Adduct*> availableAdducts{}; //based on file, not activated

        static void setDefaultSampleColors(
            vector<mzSample*>& samples,
            bool skipColoredSamples=true,
            bool setBlanksToRed=true);

      private:
		QSqlDatabase ligandDB;
        QMap<string,Compound*> compoundIdMap;
        QMap<QString,int> loadedDatabase;
};

class PeakGroupTag {

public:
    string tagName;
    char label;
    char hotkey;
    QIcon icon;
    string description;
    unsigned int displayOrder;

    PeakGroupTag(string tagName, char label, char hotKeyChar, unsigned int displayOrder, string iconName, string description) {
        this->tagName = tagName;
        this->label = label;
        this->hotkey = hotKeyChar;
        this->displayOrder = displayOrder;
        QString iconPath(":/images/");
        iconPath.append(iconName.c_str());
        this->icon = QIcon(iconPath);
        this->description = description;
    }
};

#endif

