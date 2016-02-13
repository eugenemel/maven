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
            QSqlDatabase projectDB;
            QString projectFilename;

            vector<PeakGroup> allgroups;
            vector<mzSample*> samples;

            void saveSamples(vector<mzSample *> &sampleSet);
            void saveGroups(vector<PeakGroup>   &allgroups, QString setName);
            void writeSearchResultsToDB();
            void writeGroupSqlite(PeakGroup* group);
            void loadPeakGroups(QString tableName);
            void loadGroupPeaks(PeakGroup* group);
            void deleteAll();

            ProjectDB(QString dbfilename) {
                projectFilename = dbfilename;
                open();
            }

            void setSamples(vector<mzSample*>set) {
                samples = set;
            }

            bool open() {
                projectDB = QSqlDatabase::addDatabase("QSQLITE", "projectDB");
                projectDB.setDatabaseName(projectFilename);
                projectDB.open();

               if (!projectDB.isOpen()) {
                    qDebug()  << "Failed to open project" + projectFilename;
                    return false;
                } else {
                    return true;
                }
            }

            bool close() {
                if (projectFilename.isEmpty()) {
                    //projectDB.close();
                    projectFilename = QString();
                }
                return true;
            }


};

#endif
