#include "projectDB.h"


 ProjectDB::ProjectDB(QString dbfilename) {
                openDatabaseConnection(dbfilename);
            }

void ProjectDB::saveGroups(vector<PeakGroup>& allgroups, QString setName) {
    for(unsigned int i=0; i < allgroups.size(); i++ ) {
        writeGroupSqlite(&allgroups[i],0,setName);
    }
}

void ProjectDB::saveCompounds(set<Compound*>& compoundSet) {
    //save seen compounds in to project database
    std::vector<Compound*>compounds(compoundSet.begin(), compoundSet.end());
    DB.saveCompoundsSQL(compounds,sqlDB);
}

void ProjectDB::saveCompounds(vector<PeakGroup>& allgroups) {
    set<Compound*> seenCompounds;

    //find linked compounds
    for(unsigned int i=0; i < allgroups.size(); i++ ) {
        Compound* c = allgroups[i].compound;
        if(c) seenCompounds.insert(c);
    }
    saveCompounds(seenCompounds);
}


void ProjectDB::deleteAll() {
    QSqlQuery query(sqlDB);
    query.exec("drop table IF EXISTS compounds");
    query.exec("drop table IF EXISTS samples");
    query.exec("drop table IF EXISTS peakgroups");
    query.exec("drop table IF EXISTS peaks");
    query.exec("drop table IF EXISTS rt_update_key");
    query.exec("drop table IF EXISTS matches");
    query.exec("drop table IF EXISTS search_params"); //Issue 197
    query.exec("drop table IF EXISTS ui"); //Issue 525
}

void ProjectDB::deleteGroups() {
    QSqlQuery query(sqlDB);
    query.exec("drop table peakgroups");
    query.exec("drop table peaks");
    query.exec("drop table IF EXISTS search_params"); //Issue 197
}


void ProjectDB::deleteSearchResults(QString searchTable) {
    QSqlQuery query(sqlDB);
    query.prepare("delete from peaks where groupId in (select groupId from peakgroups where searchTableName = ?");
    query.addBindValue(searchTable);
    query.exec();

    query.prepare("delete from peakgroups where searchTableName = ?");
    query.addBindValue(searchTable);
    query.exec();
}

void ProjectDB::assignSampleIds() {
    int maxSampleId = -1;
    for(mzSample* s: samples) if (s->getSampleId()>maxSampleId) maxSampleId=s->getSampleId();
    if (maxSampleId == -1) maxSampleId=0;
    for(mzSample* s: samples) {
        if (s->getSampleId() == -1)  s->setSampleId(maxSampleId++);
        cerr << "asssignSampleIds" << s->sampleName << " \t" << s->sampleId << endl;
   }
}

string ProjectDB::getScanSigniture(Scan* scan, int limitSize=200) {
    stringstream SIG;
    map<int,bool>seen;

    int mz_count=0;
    for(int pos: scan->intensityOrderDesc() ) {
        float mzround = (int) scan->mz[pos];
        int peakIntensity = (int) scan->intensity[pos];

        if(! seen.count(mzround)) {
            SIG << "[" << setprecision(9) << scan->mz[pos] << "," << peakIntensity << "]";
            seen[mzround]=true;
        }

        if (mz_count++ >= limitSize) break;
    }

    return SIG.str();
}


void ProjectDB::saveScans(vector<mzSample *> &sampleSet) {
    QSqlQuery query0(sqlDB);

	query0.exec("begin transaction");
    query0.exec("drop table scans");

    if(!query0.exec("CREATE TABLE IF NOT EXISTS scans(\
				id INTEGER PRIMARY KEY   AUTOINCREMENT,\
				sampleId     int       NOT NULL,\
				scan    int       NOT NULL,\
				fileSeekStart  int       NOT NULL,\
				fileSeekEnd    int       NOT NULL,\
				mslevel        int       NOT NULL,\
				rt             real       NOT NULL,\
				precursorMz       real       NOT NULL,\
				precursorCharge   int       NOT NULL,\
				precursorIc real  NOT NULL,\
				precursorPurity real, \
				minmz   real  NOT NULL,\
				maxmz   real  NOT NULL,\
				data VARCHAR(100000))" ))  qDebug() << "Ho... " << query0.lastError();

        QSqlQuery query1(sqlDB);
        query1.prepare("insert into scans values(NULL,?,?,?,?,?,?,?,?,?,?,?,?,?)");

		int ms2count=0;
		float ppm = 20;
        for(mzSample* s : sampleSet) {
				cerr << "Saving scans for sample: " << s->sampleName  << " #scans=" << s->scans.size() << endl;
				for (int i=0; i < s->scans.size(); i++ ) {
					if (i % 1000 == 0) cerr << "...";

					Scan* scan = s->scans[i];
					if (scan->mslevel == 1) continue;

					string scanData = getScanSigniture(scan,2000);
					ms2count++;

                    query1.addBindValue( s->getSampleId());
					query1.addBindValue( scan->scannum);
					query1.addBindValue( -1);
					query1.addBindValue( -1);
					query1.addBindValue( scan->mslevel);
					query1.addBindValue( scan->rt);
					query1.addBindValue( scan->precursorMz);
					query1.addBindValue( scan->precursorCharge);
					query1.addBindValue( scan->totalIntensity());
					query1.addBindValue( scan->getPrecursorPurity(ppm));
					query1.addBindValue( scan->minMz());
					query1.addBindValue( scan->maxMz());
					query1.addBindValue( scanData.c_str());

					if(!query1.exec())  qDebug() << query1.lastError();
				}
				cerr << "#ms2=" << ms2count << endl;
        }
    query0.exec("end transaction");
}


void ProjectDB::saveSamples(vector<mzSample *> &sampleSet) {
    QSqlQuery query0(sqlDB);

    this->assignSampleIds();

    query0.exec("begin transaction");
    if(!query0.exec("create table IF NOT EXISTS samples(\
                    sampleId integer primary key AUTOINCREMENT,\
                    name varchar(254),\
                    filename varchar(254),\
                    setName varchar(254),\
                    sampleOrder int,\
                    isSelected  int,\
                    color_red real,\
                    color_green real,\
                    color_blue real,\
                    color_alpha real,\
                    norml_const real, \
                    transform_a0 real,\
                    transform_a1 real,\
                    transform_a2 real,\
                    transform_a4 real,\
                    transform_a5 real \
                    )"))  qDebug() << "Ho... " << query0.lastError();

        QSqlQuery query1(sqlDB);
        query1.prepare("replace into samples values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");

        for(mzSample* s : sampleSet) {
            query1.bindValue( 0, s->getSampleId() );
            query1.bindValue( 1, QString(s->getSampleName().c_str()) );
            query1.bindValue( 2, QString(s->fileName.c_str()));
            query1.bindValue( 3, QString(s->getSetName().c_str()));

            query1.bindValue( 4, s->getSampleOrder() );
            query1.bindValue( 5, s->isSelected );

            query1.bindValue( 6, s->color[0] );
            query1.bindValue( 7, s->color[1] );
            query1.bindValue( 8, s->color[2] );
            query1.bindValue( 9, s->color[3] );
            query1.bindValue( 10, s->getNormalizationConstant());

            query1.bindValue( 11,  0);
            query1.bindValue( 12,  0);
            query1.bindValue( 13,  0);
            query1.bindValue( 14,  0);
            query1.bindValue( 15,  0);

            if(!query1.exec())  qDebug() << query1.lastError();
        }
    query0.exec("end transaction");
}

QStringList ProjectDB::getSearchTableNames() {
    QSqlQuery query(sqlDB);
    query.prepare("SELECT distinct searchTableName from peakgroups");
    if (!query.exec())  qDebug() << query.lastError();

    qDebug() << "Search Table Names:";
    QStringList dbnames;
    while (query.next()){
       QString tableName = query.value(0).toString();
       dbnames << tableName;
       qDebug() << "entry=" << tableName;
    }

    qDebug() << "End Search Table Names.";

    return dbnames;
}


void ProjectDB::deletePeakGroup(PeakGroup* g, QString tableName) {
     if (!g) return;

     vector<int>selectedGroups;
     selectedGroups.push_back(g->groupId);
     for(const PeakGroup& child: g->children)  selectedGroups.push_back(child.groupId);

     if(selectedGroups.size() == 0) return;

     QSqlQuery query0(sqlDB);
     query0.exec("begin transaction");
     QSqlQuery query1(sqlDB);
     query1.prepare("delete from ?     where groupId = ?");

     QSqlQuery query2(sqlDB);
     query2.prepare("delete from peaks where groupId = ?");

     for(int id: selectedGroups) {
            query1.addBindValue(tableName);
            query1.addBindValue(id);
            query2.addBindValue(id);

            if( query1.exec() ) qDebug() << query1.lastError();
            if( query2.exec() ) qDebug() << query2.lastError();
    }
    query0.exec("commit");
}

int ProjectDB::writeGroupSqlite(PeakGroup* g, int parentGroupId, QString tableName) {

     if (!g) return -1;
     if (g->deletedFlag) return -1; // skip deleted groups

     QSqlQuery query0(sqlDB);
     query0.exec("begin transaction");

     QString TABLESQL= QString("create table IF NOT EXISTS peakgroups (\
                        groupId integer primary key AUTOINCREMENT,\
                        parentGroupId integer,\
                        tagString varchar(254),\
                        metaGroupId integer,\
                        expectedRtDiff real,\
                        \
                        groupRank int,\
                        label varchar(10),\
                        type integer,\
                        srmId varchar(254),\
                        ms2EventCount int,\
                        \
                        ms2Score real,\
                        adductName varchar(32),\
                        compoundId varchar(254),\
                        compoundName varchar(254),\
                        compoundDB varchar(254),\
                        \
                        searchTableName varchar(254),\
                        displayName varchar(254),\
                        \
                        srmPrecursorMz real,\
                        srmProductMz real,\
                        \
                        isotopicIndex integer,\
                        isotopeParameters TEXT\
                        )");

     if(!query0.exec(TABLESQL)) qDebug() << query0.lastError();

     QString INSERTSQL = QString("insert into peakgroups\
                                 (groupId,parentGroupId,tagString,metaGroupId,expectedRtDiff, \
                                    groupRank,label,type,srmId,ms2EventCount, \
                                    ms2Score,adductName,compoundId,compoundName,compoundDB,\
                                    searchTableName,displayName,\
                                    srmPrecursorMz,srmProductMz,\
                                    isotopicIndex,isotopeParameters\
                                  )\
                                    \
                                 values\
                                 (?,?,?,?,?,\
                                    ?,?,?,?,?,\
                                    ?,?,?,?,?,\
                                    ?,?,\
                                    ?,?,\
                                    ?,?\
                                  )\
                                 ");

     //cerr << "inserting .. " << g->groupId << endl;
	 QSqlQuery query1(sqlDB);
            query1.prepare(INSERTSQL);

            //Issue 424
            if (g->savedGroupId == -1) { // group does not have previously saved ID value, rely on autoincrement
                query1.addBindValue(QVariant(QVariant::Int)); //equivalent to nullptr, triggers autoincrement
            } else {
                query1.addBindValue(g->savedGroupId);
            }

            query1.addBindValue(parentGroupId);
            query1.addBindValue(QString(g->tagString.c_str()));
            query1.addBindValue(g->metaGroupId);
            query1.addBindValue(g->expectedRtDiff);
            query1.addBindValue(g->groupRank);
            query1.addBindValue(QString(g->getPeakGroupLabel().c_str()));
            query1.addBindValue(g->type());
            query1.addBindValue(QString(g->srmId.c_str()));
            query1.addBindValue(g->ms2EventCount);
            query1.addBindValue(g->fragMatchScore.mergedScore);

        if (g->adduct) {
            query1.addBindValue(QString(g->adduct->name.c_str()) );
        } else if (g->compound && !g->compound->adductString.empty()){
            query1.addBindValue(QString(g->compound->adductString.c_str()));
        } else {
            query1.addBindValue(QString());
        }

        QString compoundName;
        if (!g->importedCompoundName.empty()){
              compoundName = QString(g->importedCompoundName.c_str());
        } else if (g->compound) {
              compoundName = QString(g->compound->name.c_str());
        }

        if (g->compound) {
            query1.addBindValue(QString(g->compound->id.c_str()) );
            query1.addBindValue(compoundName);
            query1.addBindValue(QString(g->compound->db.c_str()) );
        } else{
            query1.addBindValue(QString());
            query1.addBindValue(compoundName);
            query1.addBindValue(QString());
        }

        query1.addBindValue(tableName);

        if (!g->displayName.empty()){
            query1.addBindValue(QString(g->displayName.c_str()) );
        } else {
            query1.addBindValue(QString());
        }

        query1.addBindValue(g->srmPrecursorMz);
        query1.addBindValue(g->srmProductMz);

        //Issue 380
        query1.addBindValue(g->isotopicIndex);

        //Issue 402
        if (g->isotopeParameters.isotopeParametersType != IsotopeParametersType::INVALID) {
            query1.addBindValue(QString(g->isotopeParameters.encodeParams().c_str()));
        } else {
            query1.addBindValue(QString(""));
        }

     if(! query1.exec() ) {
        qDebug() << query1.lastError();
     }
     int lastInsertGroupId = query1.lastInsertId().toString().toInt();

     if (tableName == "rumsDB" || tableName== "clamDB") {
         rumsDBOldToNewGroupIDs.insert(make_pair(g->groupId, lastInsertGroupId));
     } else if (tableName == "Bookmarks") {
         bookmarksOldToNewGroupIDs.insert(make_pair(g->groupId, lastInsertGroupId));
     }

     QSqlQuery query2(sqlDB);
     if(!query2.exec("create table IF NOT EXISTS peaks( \
                    peakId integer primary key AUTOINCREMENT, \
                    groupId int,\
                    sampleId int, \
                    pos int, \
					minpos int, \
					maxpos int, \
                    rt real, \
                    rtmin real, \
                    rtmax real, \
                    mzmin real, \
                    mzmax real, \
                    scan int,	\
					minscan int, \
					maxscan int,\
                    peakArea real,\
                    peakAreaCorrected real,\
                    peakAreaTop real,\
                    peakAreaFractional real,\
                    peakRank real,\
                    peakIntensity real,\
                    peakBaseLineLevel real,\
                    peakMz real,\
                    medianMz real,\
                    baseMz real,\
                    quality real,\
					width int,\
                    gaussFitSigma real,\
                    gaussFitR2 real,\
					noNoiseObs int,\
                    noNoiseFraction real,\
                    symmetry real,\
                    signalBaselineRatio real,\
                    groupOverlap real,\
                    groupOverlapFrac real,\
                    localMaxFlag real,\
                    fromBlankSample int,\
                    label int)"))  qDebug() << query2.lastError();
			
	 		QSqlQuery query3(sqlDB); 
            query3.prepare("insert into peaks values(NULL,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
		
			for(int j=0; j < g->peaks.size(); j++ ) { 
					Peak& p = g->peaks[j];

                    if(!p.getSample()) {

                        string errMsg = "ProjectDB::writeGroupSqlite(): peak sample (peak.getSample()) is null. Illegal memory access of peak.getSample()->getSampleId(). Exiting program.";
                        cerr << errMsg << endl;

                        QString msg = QString("ProjectDB::writeGroupSqlite(): peak sample (peak.getSample()) is null. Illegal memory access of peak.getSample()->getSampleId(). Exiting program.");

                        qDebug() << msg;
                        abort();
                    }

                    query3.addBindValue(QString::number(lastInsertGroupId));
                    query3.addBindValue(p.getSample()->getSampleId()); //Issue 62: the sampleID is not set if the file is empty. Accessing this field causes a crash.
                    query3.addBindValue(p.pos);
                    query3.addBindValue(p.minpos);
                    query3.addBindValue(p.maxpos);
                    query3.addBindValue(p.rt);
                    query3.addBindValue(p.rtmin);
                    query3.addBindValue(p.rtmax);
                    query3.addBindValue(p.mzmin);
                    query3.addBindValue(p.mzmax);
                    query3.addBindValue((int) p.scan);
                    query3.addBindValue((int) p.minscan);
                    query3.addBindValue((int) p.maxscan);
                    query3.addBindValue(p.peakArea);
                    query3.addBindValue(p.peakAreaCorrected);
                    query3.addBindValue(p.peakAreaTop);
                    query3.addBindValue(p.peakAreaFractional);
                    query3.addBindValue(p.peakRank);
                    query3.addBindValue(p.peakIntensity);;
                    query3.addBindValue(p.peakBaseLineLevel);
                    query3.addBindValue(p.peakMz);
                    query3.addBindValue(p.medianMz);
                    query3.addBindValue(p.baseMz);
                    query3.addBindValue(p.quality);
                    query3.addBindValue((int) p.width);
                    query3.addBindValue(p.gaussFitSigma);
                    query3.addBindValue(p.gaussFitR2);
                    query3.addBindValue((int) p.noNoiseObs);
                    query3.addBindValue(p.noNoiseFraction);
                    query3.addBindValue(p.symmetry);
                    query3.addBindValue(p.signalBaselineRatio);
                    query3.addBindValue(p.groupOverlap);
                    query3.addBindValue(p.groupOverlapFrac);
                    query3.addBindValue(p.localMaxFlag);
                    query3.addBindValue(p.fromBlankSample);
					query3.addBindValue(QString::number(p.label));
    				if(!query3.exec())  qDebug() << query3.lastError();
		}

     //Issue 546: featurization table
     if (!g->compounds.empty()) {

        QSqlQuery query4(sqlDB);
        if(!query4.exec("create table IF NOT EXISTS peakgroupmatch( \
                       matchId integer primary key AUTOINCREMENT, \
                       groupId int,\
                       compoundId varchar(254),\
                       ppmError real,\
                       rtError real,\
                       \
                       numMatches int,\
                       fractionMatched real,\
                       spearmanRankCorrelation real,\
                       ticMatched real,\
                       mzFragError real,\
                       \
                       dotProduct real,\
                       hypergeomScore real,\
                       mvhScore real,\
                       weightedDotProduct real,\
                       \
                       )"))  qDebug() << query4.lastError();

        QSqlQuery query5(sqlDB);
        query5.prepare("insert into peakgroupmatch values("
                       "NULL,?,?,?,?,"
                       "?,?,?,?,?,"
                       "?,?,?,?,?,"
                       "?,?,?,?,?)"
                       );

        for (pair<Compound*, FragmentationMatchScore> pair : g->compounds) {

             //
             query5.addBindValue(QString::number(lastInsertGroupId));
             query5.addBindValue(QString(pair.first->id.c_str()));
             query5.addBindValue(pair.second.ppmError);

             float rtError = -1.0f;
             if (pair.first->expectedRt > 0) {
                rtError = abs(pair.first->expectedRt - g->medianRt());
             }
             query5.addBindValue(rtError);

             //

             query5.addBindValue(pair.second.numMatches);
             query5.addBindValue(pair.second.fractionMatched);
             query5.addBindValue(pair.second.spearmanRankCorrelation);
             query5.addBindValue(pair.second.ticMatched);
             query5.addBindValue(pair.second.mzFragError);

             //

             query5.addBindValue(pair.second.dotProduct);
             query5.addBindValue(pair.second.hypergeomScore);
             query5.addBindValue(pair.second.mvhScore);
             query5.addBindValue(pair.second.weightedDotProduct);

             if(!query5.exec())  qDebug() << query5.lastError();
        }
     }

        if ( g->childCount() ) {
           for(unsigned int i=0; i < g->children.size(); i++ ) {
                PeakGroup* child = &(g->children[i]); 

                QString searchTableName = child->searchTableName.empty() ? tableName : QString(child->searchTableName.c_str());

                //Issue 143: children of a bookmarked peak group should also be bookmarked
                if (tableName == "Bookmarks") {
                    searchTableName = tableName;
                }

                writeGroupSqlite(child, lastInsertGroupId, searchTableName);
            }
        }

    query0.exec("end transaction");
    //qDebug() << "writeGroupSQL: groupId" << lastInsertGroupId << " table=" << tableName;
    return lastInsertGroupId;
}

        //Issue 283: Read in all peaks to allow for faster retrieval later
        map<int, vector<Peak>> ProjectDB::getAllPeaks() {

                map<int, vector<Peak>> allPeaks{};
                unsigned long peaksCounter = 0;

                QSqlQuery query(sqlDB);
                query.prepare("select * from peaks inner join samples on peaks.sampleid = samples.sampleId;");
                query.exec();

                while (query.next()) {
                Peak p;
                p.pos = query.value("pos").toString().toInt();
                p.minpos = query.value("minpos").toString().toInt();
                p.maxpos = query.value("maxpos").toString().toInt();
                p.rt = query.value("rt").toString().toDouble();
                p.rtmin = query.value("rtmin").toString().toDouble();
                p.rtmax = query.value("rtmax").toString().toDouble();
                p.mzmin = query.value("mzmin").toString().toDouble();
                p.mzmax = query.value("mzmax").toString().toDouble();
                p.scan = query.value("scan").toString().toInt();
                p.minscan = query.value("minscan").toString().toInt();
                p.maxscan = query.value("maxscan").toString().toInt();
                p.peakArea = query.value("peakArea").toString().toDouble();
                p.peakAreaCorrected = query.value("peakAreaCorrected").toString().toDouble();
                p.peakAreaTop = query.value("peakAreaTop").toString().toDouble();
                p.peakAreaFractional = query.value("peakAreaFractional").toString().toDouble();
                p.peakRank = query.value("peakRank").toString().toDouble();
                p.peakIntensity = query.value("peakIntensity").toString().toDouble();
                p.peakBaseLineLevel = query.value("peakBaseLineLevel").toString().toDouble();
                p.peakMz = query.value("peakMz").toString().toDouble();
                p.medianMz = query.value("medianMz").toString().toDouble();
                p.baseMz = query.value("baseMz").toString().toDouble();
                p.quality = query.value("quality").toString().toDouble();
                p.width = query.value("width").toString().toInt();
                p.gaussFitSigma = query.value("gaussFitSigma").toString().toDouble();
                p.gaussFitR2 = query.value("gaussFitR2").toString().toDouble();
                p.groupNum= query.value("groupId").toString().toInt();
                p.noNoiseObs = query.value("noNoiseObs").toString().toInt();
                p.noNoiseFraction = query.value("noNoiseFraction").toString().toDouble();
                p.symmetry = query.value("symmetry").toString().toDouble();
                p.signalBaselineRatio = query.value("signalBaselineRatio").toString().toDouble();
                p.groupOverlap = query.value("groupOverlap").toString().toDouble();
                p.groupOverlapFrac = query.value("groupOverlapFrac").toString().toDouble();
                p.localMaxFlag = query.value("localMaxFlag").toString().toInt();
                p.fromBlankSample = query.value("fromBlankSample").toString().toInt();
                p.label = query.value("label").toString().toInt();

                string sampleName = query.value("name").toString().toStdString();

                for(int i=0; i< samples.size(); i++ ) {
                    if (samples[i]->sampleName == sampleName ) { p.setSample(samples[i]); break;}
                }

                if (allPeaks.find(p.groupNum) == allPeaks.end()) {
                    allPeaks.insert(make_pair(p.groupNum, vector<Peak>()));
                }
                allPeaks[p.groupNum].push_back(p);
                peaksCounter++;
            }

                cerr << "ProjectDB::getAllPeaks(): loaded " << peaksCounter << " peaks for " << allPeaks.size() << " groups." << endl;

                return allPeaks;
        }


void ProjectDB::loadPeakGroups(QString tableName, QString rumsDBLibrary, bool isAttemptToLoadDB, const map<int, vector<Peak>>& peakGroupMap, Classifier *classifier) {

        QSqlQuery queryCheckCols(sqlDB);

        QString strCheckCols = QString();
        strCheckCols.append("pragma table_info(");
        strCheckCols.append(tableName);
        strCheckCols.append(");");

        if (!queryCheckCols.exec(strCheckCols)) {
            qDebug() << "Ho..." << queryCheckCols.lastError();
        }

        bool isHasDisplayName = false;
        bool isHasSrmPrecursorMz = false;
        bool isHasSrmProductMz = false;
        bool isHasIsotopicIndex = false;
        bool isHasIsotopeParameters = false;

        while (queryCheckCols.next()) {
            if ("displayName" == queryCheckCols.value(1).toString()) {
                isHasDisplayName = true;
            } else if ("srmPrecursorMz" == queryCheckCols.value(1).toString()) {
                isHasSrmPrecursorMz = true;
            } else if ("srmProductMz" == queryCheckCols.value(1).toString()) {
                isHasSrmProductMz = true;
            } else if ("isotopicIndex" == queryCheckCols.value(1).toString()) {
                isHasIsotopicIndex = true;
            } else if ("isotopeParameters" == queryCheckCols.value(1).toString()) {
                isHasIsotopeParameters = true;
            }
        }

        qDebug() << "ProjectDB::loadPeakGroups(): "
                 << "isHasDisplayName? " << isHasDisplayName
                 << "isHasSrmPrecursorMz? " << isHasSrmPrecursorMz
                 << "isHasSrmProductMz? " << isHasSrmProductMz
                 << "isHasIsotopicIndex? " << isHasIsotopicIndex
                 << "isHasIsotopeParameters? " << isHasIsotopeParameters;

        if (!isHasDisplayName) {
            QSqlQuery queryAdjustPeakGroupsTable(sqlDB);

            QString strAdjustPeakGroupsTable = QString();
            strAdjustPeakGroupsTable.append("ALTER TABLE ");
            strAdjustPeakGroupsTable.append(tableName);
            strAdjustPeakGroupsTable.append(" ADD displayName VARCHAR(254) ");

            if (!queryAdjustPeakGroupsTable.exec(strAdjustPeakGroupsTable)){
                qDebug() << "Ho..." <<queryCheckCols.lastError();
            }
        }

        if (!isHasSrmPrecursorMz) {
            QSqlQuery queryAddSrmPrecursor(sqlDB);

            QString strAddSrmPrecursor = QString("ALTER TABLE ");
            strAddSrmPrecursor.append(tableName);
            strAddSrmPrecursor.append(" ADD srmPrecursorMz REAL DEFAULT 0");
            if (!queryAddSrmPrecursor.exec(strAddSrmPrecursor)) {
                qDebug() << "Ho..." << queryAddSrmPrecursor.lastError();
            }
        }

        if (!isHasSrmProductMz) {
            QSqlQuery queryAddSrmProduct(sqlDB);

            QString strAddSrmProduct = QString("ALTER TABLE ");
            strAddSrmProduct.append(tableName);
            strAddSrmProduct.append(" ADD srmProductMz REAL DEFAULT 0");
            if (!queryAddSrmProduct.exec(strAddSrmProduct)) {
                qDebug() << "Ho..." << queryAddSrmProduct.lastError();
            }
        }

        if (!isHasIsotopicIndex) {
            QSqlQuery queryAddIsotopicIndex(sqlDB);

            QString strAddIsotopicIndex = QString("ALTER TABLE ");
            strAddIsotopicIndex.append(tableName);
            strAddIsotopicIndex.append(" ADD isotopicIndex INTEGER DEFAULT 0");
            if (!queryAddIsotopicIndex.exec(strAddIsotopicIndex)) {
                qDebug() << "Ho..." << queryAddIsotopicIndex.lastError();
            }
        }

        if (!isHasIsotopeParameters) {
            QSqlQuery queryAddIsotopeParameters(sqlDB);

            QString strAddIsotopeParameters = QString();
            strAddIsotopeParameters.append("ALTER TABLE ");
            strAddIsotopeParameters.append(tableName);
            strAddIsotopeParameters.append(" ADD isotopeParameters TEXT DEFAULT ''");

            if (!queryAddIsotopeParameters.exec(strAddIsotopeParameters)){
                qDebug() << "Ho..." <<queryCheckCols.lastError();
            }
        }

     QSqlQuery query(sqlDB);
     query.exec("create index if not exists peak_group_ids on peaks(groupId)");
     query.exec("select * from " + tableName );

     //Issue 62: NEW CODE
     vector<pair<PeakGroup, int>> peakGroupChildren;

     QStringList databaseNames = DB.getDatabaseNames();

     QStringList projectFileDatabaseNames = getProjectFileDatabaseNames();

     while (query.next()) {

        PeakGroup g;
        g.groupId = query.value("groupId").toInt();
        int parentGroupId = query.value("parentGroupId").toInt();
        g.tagString = query.value("tagString").toString().toStdString();
        g.metaGroupId = query.value("metaGroupId").toInt();
        g.expectedRtDiff = query.value("expectedRtDiff").toDouble();
        g.groupRank = query.value("groupRank").toInt();
        g.srmPrecursorMz = static_cast<float>(query.value("srmPrecursorMz").toDouble());
        g.srmProductMz = static_cast<float>(query.value("srmProductMz").toDouble());
        g.isotopicIndex = query.value("isotopicIndex").toInt();

        QVariant label = query.value("label");
        if (label.toString().size() > 0) {

            QByteArray bytes = label.toString().toLatin1().data();
            for (auto &b : bytes) {
                g.labels.push_back(b);
            }
        }

        string displayName = query.value("displayName").toString().toStdString();
        if (!displayName.empty()) {
            g.displayName = displayName;
        }

        g.ms2EventCount = query.value("ms2EventCount").toString().toInt();
        g.fragMatchScore.mergedScore = query.value("ms2Score").toString().toDouble();
        g.setType( (PeakGroup::GroupType) query.value("type").toString().toInt());
        g.searchTableName = query.value("searchTableName").toString().toStdString();

        string compoundId = query.value("compoundId").toString().toStdString();
        string compoundDB = query.value("compoundDB").toString().toStdString();
        string compoundName = query.value("compoundName").toString().toStdString();
        string adductName = query.value("adductName").toString().toStdString();

        g.compoundId = compoundId;
        g.compoundDb = compoundDB;

        g.importedCompoundName = compoundName;

        string srmId = query.value("srmId").toString().toStdString();
        if (!srmId.empty()) g.setSrmId(srmId);

        if (!adductName.empty()) {
             Adduct *dbAdduct =DB.findAdductByName(adductName);

            if (dbAdduct) {
                g.adduct = dbAdduct;
            } else {
                for (Adduct *adduct : DB.availableAdducts) {
                    if (adduct && adduct->name == adductName) {
                        g.adduct = adduct;
                        DB.adductsDB.push_back(adduct);
                        break;
                    }
                }
            }

        }

        //Issue 402: inflate saved IsotopeParameters, if applicable
        string encodedIsotopeParameters = query.value("isotopeParameters").toString().toStdString();
        if (!encodedIsotopeParameters.empty()){

            IsotopeParameters decodedIsotopeParameters = IsotopeParameters::decode(encodedIsotopeParameters);

            if (!decodedIsotopeParameters.adductName.empty()) {
                decodedIsotopeParameters.adduct = DB.findAdductByName(decodedIsotopeParameters.adductName);
            }

            //TODO: no validation checking around encoded Classifier information
            decodedIsotopeParameters.clsf = classifier;

            g.isotopeParameters = decodedIsotopeParameters;
        }

        if (!compoundId.empty()){

            Compound *compound = nullptr;
            if (databaseNames.contains(QString(compoundDB.c_str()))) {
                compound = DB.findSpeciesById(compoundId, compoundDB, isAttemptToLoadDB);
            }

            //Issue 92: fall back to rumsDB table if could not find compound the normal way.
            //Issue 363: Support clamDB table name as well as rumsDB
            if (!compound && (g.searchTableName == "rumsDB" || g.searchTableName == "clamDB") && !rumsDBLibrary.isEmpty()) {
                compound = DB.findSpeciesById(compoundId, rumsDBLibrary.toStdString(), isAttemptToLoadDB);

                //Issue 271: to facillitate proper disconnect / reconnect from tabledockwidget
                if (compound){
                    g.compoundDb = rumsDBLibrary.toStdString();
                }
            }

            //Issue 190: fall back to data stored within the file
            if (!compound && projectFileDatabaseNames.contains(QString(compoundDB.c_str()))) {
                compound = DB.findSpeciesById(compoundId, compoundDB, isAttemptToLoadDB);
            }

            if (compound)  {
                g.compound = compound;
                //cerr << "Found compound:"  << compound->cid << endl;
            } else {
                g.tagString = compoundName + "|" + adductName + " | id=" + compoundId;
            }


        } else if (!compoundName.empty() && !compoundDB.empty()) {
            vector<Compound*>matches = DB.findSpeciesByName(compoundName, compoundDB, isAttemptToLoadDB);
            if (matches.size()>0) g.compound = matches[0];
        }

        //Issue 283
        if (peakGroupMap.find(g.groupId) == peakGroupMap.end()) {
            loadGroupPeaks(&g);
        } else {
            g.peaks = peakGroupMap.at(g.groupId);
        }

        if (parentGroupId == 0) {
            allgroups.push_back(g);
        } else {
            peakGroupChildren.push_back(make_pair(g, parentGroupId));
        }

        }

        vector<pair<PeakGroup, int>> matchedChildren;

        if (peakGroupChildren.size() > 0) {

            sort(peakGroupChildren.begin(), peakGroupChildren.end(), [](const pair<PeakGroup, int>& lhs, const pair<PeakGroup, int>& rhs){
                return lhs.second < rhs.second;
            });

            sort(allgroups.begin(), allgroups.end(), [](const PeakGroup& lhs, const PeakGroup& rhs){
                return lhs.groupId < rhs.groupId;
            });

            unsigned int childPosition = 0;
            unsigned int parentPosition = 0;

            while (childPosition < peakGroupChildren.size()-1) {

                bool isMatchedChild = false;

                pair<PeakGroup, int> pair = peakGroupChildren.at(childPosition);

               PeakGroup child = pair.first;
                int parentGroupId = pair.second;

                PeakGroup& parent = allgroups.at(parentPosition);

                if (parent.groupId == parentGroupId) { //found parent for child
                    parent.children.push_back(child);
                    child.parent = &parent;
                    isMatchedChild = true;
                   matchedChildren.push_back(pair);
               }

              if (isMatchedChild) { //child matches to current parent, check next child

                    //if the next child has a different parent, check the next parent
                   if (peakGroupChildren.at(childPosition+1).second != peakGroupChildren.at(childPosition).second){
                        parentPosition++;
                    }

                    childPosition++;

                } else { //child did not match to the current parent, check next parent
                    parentPosition++;
                 }

                //All parents have been considered
                if (parentPosition == allgroups.size()){
                    break;
                }
            }

            //last entry
            for (unsigned int j = parentPosition; j < allgroups.size(); j++){

                PeakGroup& parent = allgroups.at(j);

                pair<PeakGroup, int> pair = peakGroupChildren.at(peakGroupChildren.size()-1);

                PeakGroup child = pair.first;
                int parentGroupId = pair.second;

                if (parent.groupId == parentGroupId) { //found parent for child
                    parent.children.push_back(child);
                    child.parent = &parent;
                    matchedChildren.push_back(pair);
                }
            }

        //debugging
        if (peakGroupChildren.size() != matchedChildren.size()) {
            cerr << "WARNING: some peak groups had parent peak groups that were not included in the file!" << endl;
            cerr << "All children: " << peakGroupChildren.size() << ", matched children: " << matchedChildren.size() << endl;
            //cerr << "child peakgroup id# " << child.groupId << " is missing parent id# " << parentGroupId << endl;
        }

        }

   cerr << "ProjectDB::loadPeakGroups(): Read in " << (allgroups.size()+matchedChildren.size()) << " total peak groups, "
        << allgroups.size() << " parents and " << matchedChildren.size() << " children." << endl;
}

void ProjectDB::loadMatchTable() {

        qDebug() << "ProjectDB::loadMatchTable()... ";

        QSqlQuery queryCheckTable(sqlDB);

        if (!queryCheckTable.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='matches';")){
            qDebug() << "Ho..." << queryCheckTable.lastError();
            qDebug() << "Could not check file for presence or absence of match table. Skipping.";
            return; // If the match table couldn't be loaded, bail
        }

        bool isHasMatchesTable = false;
        while (queryCheckTable.next()) {
        if ("matches" == queryCheckTable.value(0).toString()){
                isHasMatchesTable = true;
                break;
            }
        }

        if (!isHasMatchesTable) {
            qDebug() << "Match table not found in file.";
            return;
        }

        QSqlQuery queryMatches(sqlDB);
        queryMatches.exec("select * from matches;");

        while (queryMatches.next()) {

            int groupId = queryMatches.value("groupId").toInt();

            int ionId = queryMatches.value("ionId").toInt();
            string compoundName = queryMatches.value("compoundName").toString().toStdString();
            string adductName = queryMatches.value("adductName").toString().toStdString();
            int summarizationLevel = queryMatches.value("summarizationLevel").toInt();
            string originalCompoundName = queryMatches.value("originalCompoundName").toString().toStdString();
            float score = queryMatches.value("score").toFloat();
            bool is_match = queryMatches.value("is_match").toInt() == 1;

            shared_ptr<mzrollDBMatch> mzrollDBMatchPtr = shared_ptr<mzrollDBMatch>(
                new mzrollDBMatch(
                groupId,
                ionId,
                compoundName,
                adductName,
                summarizationLevel,
                originalCompoundName,
                score,
                is_match)
            );

            if (topMatch.find(groupId) == topMatch.end()) {
                topMatch.insert(make_pair(groupId, mzrollDBMatchPtr));
            } else {
                shared_ptr<mzrollDBMatch> oldMatch = topMatch.at(groupId);
                if (score > oldMatch->score) {
                    topMatch.at(groupId) = mzrollDBMatchPtr;
                }
            }

            allMatches.insert(make_pair(groupId, mzrollDBMatchPtr));
        }

        qDebug() << "ProjectDB::loadMatchTable(): Loaded " << topMatch.size() << " top matches.";
        qDebug() << "ProjectDB::loadMatchTable(): Loaded " << allMatches.size() << "matches.";
}

void ProjectDB::saveMatchTable() {

        qDebug() << "ProjectDB::saveMatchTable()... ";

        QSqlQuery query0(sqlDB);

        query0.exec("begin transaction");

        //create table if not exists
        if(!query0.exec("CREATE TABLE IF NOT EXISTS matches("
                        "groupId INT NOT NULL,"
                        "ionId INT NOT NULL,"
                        "compoundName TEXT,"
                        "adductName TEXT,"
                        "summarizationLevel INT,"
                        "originalCompoundName TEXT,"
                        "score REAL,"
                        "is_match INT"
                        ");")){
        qDebug() << "Ho... " << query0.lastError();
        }

        QSqlQuery query1(sqlDB);
        query1.prepare("INSERT INTO matches VALUES(?,?,?,?,?,  ?,?,?)");

        for (matchIterator it = allMatches.begin(); it != allMatches.end(); ++it){

            shared_ptr<mzrollDBMatch> match = it->second;

            if (rumsDBOldToNewGroupIDs.find(match->groupId) != rumsDBOldToNewGroupIDs.end()) {

                int updatedGroupId = rumsDBOldToNewGroupIDs.at(match->groupId);

                query1.addBindValue(updatedGroupId);

                query1.addBindValue(match->ionId);
                query1.addBindValue(match->compoundName.c_str());
                query1.addBindValue(match->adductName.c_str());
                query1.addBindValue(match->summarizationLevel);
                query1.addBindValue(match->originalCompoundName.c_str());
                query1.addBindValue(match->score);
                query1.addBindValue(match->isMatchAsInt());

                if(!query1.exec()){
                    qDebug() << query1.lastError();
                }
            }

        if (bookmarksOldToNewGroupIDs.find(match->groupId) != bookmarksOldToNewGroupIDs.end()) {

                int updatedGroupId = bookmarksOldToNewGroupIDs.at(match->groupId);

                query1.addBindValue(updatedGroupId);

                query1.addBindValue(match->ionId);
                query1.addBindValue(match->compoundName.c_str());
                query1.addBindValue(match->adductName.c_str());
                query1.addBindValue(match->summarizationLevel);
                query1.addBindValue(match->originalCompoundName.c_str());
                query1.addBindValue(match->score);
                query1.addBindValue(match->isMatchAsInt());

                if(!query1.exec()){
                    qDebug() << query1.lastError();
                }
            }
        }

        query0.exec("end transaction");

        qDebug() << "ProjectDB::saveMatchTable(): Saved " << allMatches.size() << "matches.";
}

void ProjectDB::saveUiTable() {
        qDebug() << "ProjectDB::saveUiTable()... ";

        //As additional UI options are available, update this logic
        if (quantTypeMap.empty()) {
            return;
        }

        QSqlQuery query0(sqlDB);
        query0.exec("begin transaction");

        //create table if it doesn't exist already
        if(!query0.exec("CREATE TABLE IF NOT EXISTS ui("
                        "key TEXT,"
                        "value TEXT"
                        ");")){
        qDebug() << "Ho... " << query0.lastError();
        }

        QString key = "quantType";
        QString value = "{";
        for (auto it = quantTypeMap.begin(); it != quantTypeMap.end(); ++it) {
            value.append(it->first.c_str());
            value.append("=");
            value.append(it->second.c_str());
            value.append(";");
        }
        value.append("}");

        QSqlQuery query1(sqlDB);
        query1.prepare("INSERT INTO ui VALUES(?,?)");
        query1.addBindValue(key);
        query1.addBindValue(value);

        if(!query1.exec()){
            qDebug() << query1.lastError();
        }

        query0.exec("end transaction");

        qDebug() << "ProjectDB::saveUiTable(): Saved UI options.";
}

mzSample* ProjectDB::getSampleById(int sampleId) { 
	for (mzSample* s: samples) {
		if (s->sampleId == sampleId)  return s;
	}
	return 0;
}

void ProjectDB::saveAlignment() {
            qDebug() << "ProjectDB::saveAlignment()";

            QSqlQuery query0(sqlDB);
            query0.exec("begin transaction");
            //create table if not exists
            if(!query0.exec("CREATE TABLE IF NOT EXISTS rt_update_key(sampleId int NOT NULL, rt real NOT NULL, rt_update real NOT NULL)"))
                    qDebug() << "Ho... " << query0.lastError();

            //delete current alignment
            query0.exec("delete from rt_update_key");

            //save new alignment
           QSqlQuery query1(sqlDB);
           query1.prepare("insert into rt_update_key values(?,?,?)");

        for(mzSample* s : samples) {

                 cerr << "Saving alignment for sample: " << s->sampleName  << " #scans=" << s->scans.size() << endl;

                 for (int i=0; i < s->scans.size(); i++ ) {

                     if (i % 200 == 0 || i == s->scans.size()-1) {

                             Scan* scan = s->scans[i];
                             float rt_update = scan->rt;
                             float rt = scan->rt;

                             if(s->originalRetentionTimes.size() > i){
                                 rt = s->originalRetentionTimes[i];
                             }

//                           //debugging
//                           cout << s->sampleName << "\t" << to_string(rt) << "\t" << to_string(rt_update) << "\n";

                             query1.addBindValue( s->getSampleId() );
                             query1.addBindValue( rt);
                             query1.addBindValue( rt_update );

                             if(!query1.exec()) {
                                 qDebug() << query1.lastError();}
                             }
        }
     }
     query0.exec("end transaction");
}


void ProjectDB::doAlignment() {
    if (!sqlDB.isOpen() ) { qDebug() << "doAlignment: database is not opened"; return; }

	QSqlQuery query(sqlDB);
	query.exec("select S.name, S.sampleId, R.* from rt_update_key R, samples S where S.sampleId = R.sampleId");

	AlignmentSegment* lastSegment=0;
	Aligner aligner;
	aligner.setSamples(samples);

     int segCount=0;
     while (query.next()) {
        string sampleName = query.value("name").toString().toStdString();
        //mzUtils::replace(sampleName,".mzXML",""); //bug fix.. alignment.rt files do not strore extensions.

        int sampleId =   query.value("sampleId").toString().toInt();
        mzSample* sample = this->getSampleById(sampleId);
        if (!sample) continue;
		segCount++;

        AlignmentSegment* seg = new AlignmentSegment();
        seg->sampleName   = sampleName;
        seg->seg_start = 0;
        seg->seg_end   = query.value("rt").toString().toDouble();
        seg->new_start = 0;
        seg->new_end   = query.value("rt_update").toString().toDouble();

		if (lastSegment and lastSegment->sampleName == seg->sampleName) { 
			seg->seg_start = lastSegment->seg_end;
			seg->new_start = lastSegment->new_end;
		}

		aligner.addSegment(sample->sampleName,seg);
		lastSegment = seg;
       }
     cerr << "ProjectDB::doAlignment() loaded " << segCount << " segments\t"; 
     aligner.doSegmentedAligment();
}


void ProjectDB::loadGroupPeaks(PeakGroup* parent) {

     QSqlQuery query(sqlDB);
     query.prepare("select P.*, S.name as sampleName from peaks P, samples S where P.sampleId = S.sampleId and P.groupId = ?");
     query.bindValue(0,parent->groupId);
     //qDebug() << "loadin peaks for group " << parent->groupId;
     query.exec();

     while (query.next()) {
            Peak p;
            p.pos = query.value("pos").toString().toInt();
            p.minpos = query.value("minpos").toString().toInt();
            p.maxpos = query.value("maxpos").toString().toInt();
            p.rt = query.value("rt").toString().toDouble();
            p.rtmin = query.value("rtmin").toString().toDouble();
            p.rtmax = query.value("rtmax").toString().toDouble();
            p.mzmin = query.value("mzmin").toString().toDouble();
            p.mzmax = query.value("mzmax").toString().toDouble();
            p.scan = query.value("scan").toString().toInt();
            p.minscan = query.value("minscan").toString().toInt();
            p.maxscan = query.value("maxscan").toString().toInt();
            p.peakArea = query.value("peakArea").toString().toDouble();
            p.peakAreaCorrected = query.value("peakAreaCorrected").toString().toDouble();
            p.peakAreaTop = query.value("peakAreaTop").toString().toDouble();
            p.peakAreaFractional = query.value("peakAreaFractional").toString().toDouble();
            p.peakRank = query.value("peakRank").toString().toDouble();
            p.peakIntensity = query.value("peakIntensity").toString().toDouble();
            p.peakBaseLineLevel = query.value("peakBaseLineLevel").toString().toDouble();
            p.peakMz = query.value("peakMz").toString().toDouble();
            p.medianMz = query.value("medianMz").toString().toDouble();
            p.baseMz = query.value("baseMz").toString().toDouble();
            p.quality = query.value("quality").toString().toDouble();
            p.width = query.value("width").toString().toInt();
            p.gaussFitSigma = query.value("gaussFitSigma").toString().toDouble();
            p.gaussFitR2 = query.value("gaussFitR2").toString().toDouble();
            p.groupNum= query.value("groupId").toString().toInt();
            p.noNoiseObs = query.value("noNoiseObs").toString().toInt();
            p.noNoiseFraction = query.value("noNoiseFraction").toString().toDouble();
            p.symmetry = query.value("symmetry").toString().toDouble();
            p.signalBaselineRatio = query.value("signalBaselineRatio").toString().toDouble();
            p.groupOverlap = query.value("groupOverlap").toString().toDouble();
            p.groupOverlapFrac = query.value("groupOverlapFrac").toString().toDouble();
            p.localMaxFlag = query.value("localMaxFlag").toString().toInt();
            p.fromBlankSample = query.value("fromBlankSample").toString().toInt();
            p.label = query.value("label").toString().toInt();

            string sampleName = query.value("sampleName").toString().toStdString();

            for(int i=0; i< samples.size(); i++ ) {
                if (samples[i]->sampleName == sampleName ) { p.setSample(samples[i]); break;}
            }
            //cerr << "\t\t\t" << p.getSample()->sampleName << " " << p.rt << endl;
            parent->addPeak(p);
        }
}

QString  ProjectDB::projectPath() {
 	QFileInfo fileinfo(sqlDB.databaseName());
    	return fileinfo.path();
}

QString  ProjectDB::projectName() {
 	QFileInfo fileinfo(sqlDB.databaseName());
    	return fileinfo.fileName();
}

    
bool ProjectDB::openDatabaseConnection(QString dbname) {
    if (sqlDB.isOpen() and  sqlDB.databaseName() == dbname) {
        qDebug() << "Already opened. ";
        return true;
    }

    if (!sqlDB.isOpen()) {
        qDebug() << "opening... " << dbname;
        sqlDB = QSqlDatabase::addDatabase("QSQLITE", dbname);
        sqlDB.setDatabaseName(dbname);
        sqlDB.open();
    }

    return sqlDB.isOpen();
}


QString ProjectDB::locateSample(QString filepath, QStringList pathlist) {

        //found file, all is good
        QFileInfo sampleFile(filepath);
        if (sampleFile.exists())  return filepath;

        //search for file
        QString fileName = sampleFile.fileName();
        foreach(QString path, pathlist) {
            QString filepath= path + QDir::separator() + fileName;
            QFileInfo checkFile(filepath);
            if (checkFile.exists())  return filepath;
        }
        return ""; //empty string

}


void ProjectDB::loadSamples() {

    	if (!sqlDB.isOpen()) { 
		qDebug() << "error loadSamples().. datatabase is not open";
		return;
	}

	QString projectPath = this->projectPath();
	QSqlQuery query(sqlDB);
	query.exec("select * from samples");

	QStringList filelist;
	while (query.next()) {
		QString fname   = query.value("filename").toString();
		QString sname   = query.value("name").toString();
		QString setname  = query.value("setName").toString();
		int sampleId    = query.value("sampleId").toString().toInt();

		int sampleOrder  = query.value("sampleOrder").toInt();
		int isSelected   = query.value("isSelected").toInt();
		float color_red    = query.value("color_red").toDouble();
		float color_blue   = query.value("color_blue").toDouble();
		float color_green = query.value("color_green").toDouble();
		float color_alpha  = query.value("color_alpha").toDouble();
		float norml_const   = query.value("norml_const").toDouble(); if(norml_const == 0) norml_const=1.0;

		//skip files that have been loaded already
		bool checkLoaded=false;
		foreach(mzSample* loadedFile, samples) {
			if (loadedFile->sampleName == sname.toStdString()) checkLoaded=true;
			if (QString(loadedFile->fileName.c_str()) == fname) checkLoaded=true;
        }

        if(checkLoaded == true) continue;  // skip files that have been loaded already

        QStringList pathlist;
        pathlist << projectPath << "./" << "../";

        QString filepath = ProjectDB::locateSample(fname, pathlist);
        if(!filepath.isEmpty()) {
            	mzSample* s = new mzSample();
                s->loadSample(filepath.toLatin1());
                s->fileName = filepath.toStdString();
                s->setSampleId(sampleId);
                s->setSampleOrder(sampleOrder);
                s->isSelected = isSelected;
                s->color[0]   = color_red;
                s->color[1]   = color_green;
                s->color[2]   = color_blue;
                s->color[3]   = color_alpha;
                s->setNormalizationConstant(norml_const);
                if(s->scans.size() > 0) this->samples.push_back(s);
        } else {
            qDebug() << "Can't find sample:" << fname;
        }
    }
}


bool ProjectDB::closeDatabaseConnection() {

    if (sqlDB.isOpen()) {
        sqlDB.close();
        QSqlDatabase::removeDatabase(sqlDB.databaseName());
    }

    return !sqlDB.isOpen();
}

void ProjectDB::savePeakGroupsTableData(map<QString, QString> searchTableData){

        QSqlQuery query0(sqlDB);

        query0.exec("begin transaction");

        //create table if not exists
        if(!query0.exec("CREATE TABLE IF NOT EXISTS search_params("
                        "searchTableName TEXT,"
                        "parameters TEXT"
                        ");")){

            qDebug() << "Ho... " << query0.lastError();
        }

        QSqlQuery query1(sqlDB);
        query1.prepare("INSERT INTO search_params VALUES(?,?)");

        for (auto it = searchTableData.begin(); it != searchTableData.end(); ++it) {

            QString tableName = it->first;
            QString encodedTableData = it->second;

            query1.addBindValue(tableName);
            query1.addBindValue(encodedTableData);

            if(!query1.exec())  qDebug() << query1.lastError();

        }

        query0.exec("end transaction");
        qDebug() << "ProjectDB::savePeakGroupsTableData(): Saved" << searchTableData.size() << "searches.";
}

void ProjectDB::loadSearchParams(){
        diSearchParameters.clear();
        peaksSearchParameters.clear();

        qDebug() << "ProjectDB::loadSearchParams()... ";

        QSqlQuery queryCheckTable(sqlDB);

        if (!queryCheckTable.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='search_params';")){
            qDebug() << "Ho..." << queryCheckTable.lastError();
            qDebug() << "Could not check file for presence or absence of search_params table. Skipping.";
            return;
        }

        bool isHasSearchParamsTable = false;
        while (queryCheckTable.next()) {
        if ("search_params" == queryCheckTable.value(0).toString()){
                isHasSearchParamsTable = true;
                break;
            }
        }

        if (!isHasSearchParamsTable) return;

        QSqlQuery queryMatches(sqlDB);
        queryMatches.exec("select * from search_params;");

        QRegExp DIexpr("^Direct Infusion Analysis");
        QRegExp ms3Expr("^Targeted MS3 Search");
        QRegExp LCMSDDAexpr("^Detected Features");
        QRegExp LibSearchExpr("^Compound DB Search");
        QRegExp QQQLibSearchExpr("^QQQ Compound DB Search");

        while (queryMatches.next()) {

            QString searchTableName = queryMatches.value("searchTableName").toString();
            QString encodedSearchParams = queryMatches.value("parameters").toString();

            int pos = DIexpr.indexIn(searchTableName);

            //Issue 226
            if (pos == -1) {
                pos = ms3Expr.indexIn(searchTableName);
            }

            if (pos != -1) {

                string encodedSearchParamsString = encodedSearchParams.toStdString();
                shared_ptr<DirectInfusionSearchParameters> directInfusionSearchParameters = DirectInfusionSearchParameters::decode(encodedSearchParamsString);
                diSearchParameters.insert(make_pair(searchTableName.toStdString(), directInfusionSearchParameters));
                continue;
            }

            //Issue 197
            pos = LCMSDDAexpr.indexIn(searchTableName);

            if (pos == -1) {
                pos = LibSearchExpr.indexIn(searchTableName);
            }

            if (pos == -1) {
                pos = QQQLibSearchExpr.indexIn(searchTableName);
            }

            if (pos != -1){
                string encodedSearchParamsString = encodedSearchParams.toStdString();
                shared_ptr<PeaksSearchParameters> peaksSearchParametersTbl = PeaksSearchParameters::decode(encodedSearchParamsString);
                peaksSearchParameters.insert(make_pair(searchTableName.toStdString(), peaksSearchParametersTbl));
                continue;
            }
        }
}



        QStringList ProjectDB::getProjectFileDatabaseNames() {
            QSqlQuery query(sqlDB);
            query.prepare("SELECT distinct dbName from compounds");
            if (!query.exec())  qDebug() << query.lastError();

            QStringList dbnames;
            while (query.next())  dbnames << query.value(0).toString();
            return dbnames;
        }

void ProjectDB::loadUIOptions() {

    quantTypeMap.clear();
    quantTypeInverseMap.clear();

    qDebug() << "ProjectDB::loadUIOptions()... ";

        QSqlQuery queryCheckTable(sqlDB);

        if (!queryCheckTable.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='ui';")){
            qDebug() << "Ho..." << queryCheckTable.lastError();
            qDebug() << "Could not check file for presence or absence of ui table. Skipping.";
            return;
        }

        bool isHasUITable = false;
        while (queryCheckTable.next()) {
        if ("ui" == queryCheckTable.value(0).toString()){
                isHasUITable = true;
                break;
            }
        }

        if (!isHasUITable) return;

        QSqlQuery queryMatches(sqlDB);
        queryMatches.exec("select * from ui;");

        while (queryMatches.next()) {

            QString key = queryMatches.value("key").toString();
            QString value = queryMatches.value("value").toString();

        if (key == "quantType") {
            quantTypeMap = mzUtils::decodeParameterMap(value.toStdString());
            for (auto it = quantTypeMap.begin(); it != quantTypeMap.end(); ++it) {
                quantTypeInverseMap.insert(make_pair(it->second, it->first));
            }

        }
        }
}
