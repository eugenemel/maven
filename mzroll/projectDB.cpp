#include "projectDB.h"

void ProjectDB::saveGroups(vector<PeakGroup>& allgroups) { 
    for(unsigned int i=0; i < allgroups.size(); i++ ) writeGroupSqlite(&allgroups[i]);
}

void ProjectDB::saveSamples(vector<mzSample *> &sampleSet) {
    QSqlQuery query0(projectDB);

    query0.exec("begin transaction");
    if(!query0.exec("create table IF NOT EXISTS samples( \
                    sampleId int primary key, \
                    name varchar(255), \
                    filename varchar(255), \
                    setName varchar(255), \
                    color_red float, \
                    color_blue float, \
                    color_green float, \
                    color_alpha float, \
                    transform_a0 float, \
                    transform_a1 float, \
                    transform_a2 float, \
                    transform_a4 float, \
                    transform_a5 float \
                    )"))  qDebug() << "Ho... " << query0.lastError();

        QSqlQuery query1(projectDB);
        query1.prepare("insert into samples values(?,?,?,?,?,?,?,?,?,?,?,?,?)");

        for(mzSample* s : sampleSet) {
            query1.bindValue( 0, s->getSampleOrder() );
            query1.bindValue( 1, QString(s->getSampleName().c_str()) );
            query1.bindValue( 2, QString(s->fileName.c_str()));
            query1.bindValue( 3, QString(s->getSetName().c_str()));
            query1.bindValue( 4, s->color[0] );
            query1.bindValue( 5, s->color[1] );
            query1.bindValue( 6, s->color[2] );
            query1.bindValue( 7, s->color[3] );
            query1.bindValue( 8,  0);
            query1.bindValue( 9,  0);
            query1.bindValue( 10,  0);
            query1.bindValue( 11,  0);
            query1.bindValue( 12,  0);
            if(!query1.exec())  qDebug() << query1.lastError();
        }
    query0.exec("end transaction");
}

void ProjectDB::writeGroupSqlite(PeakGroup* g) {
	if (!g) return;

     QSqlQuery query0(projectDB);
     query0.exec("begin transaction");
     if(!query0.exec("create table IF NOT EXISTS peakgroups( \
                        groupId int primary key, \
                        tagString varchar(255), \
                        metaGroupId int, \
                        expectedRtDiff float, \
                        groupRank int, \
                        label varchar(10),\
                        type int,\
                        srmId varchar(255),\
                        compoundId varchar(255),\
                        compoundName varchar(255),\
						compoundDB varchar(255)\ 
                        )"))  qDebug() << query0.lastError();

     //cerr << "inserting .. " << g->groupId << endl;
	 QSqlQuery query1(projectDB);
		query1.prepare("insert into peakgroups values(?,?,?,?,?,?,?,?,?,?,?)");
        query1.bindValue( 0, QString::number(g->groupId)  );
		query1.bindValue( 1, QString(g->tagString.c_str()) );
		query1.bindValue( 2, QString::number(g->metaGroupId ));
		query1.bindValue( 3, QString::number(g->expectedRtDiff,'f',2));
		query1.bindValue( 4, QString::number(g->groupRank));
		query1.bindValue( 5, QString(g->label)  );
		query1.bindValue( 6,QString::number(g->type()));
		query1.bindValue( 7, QString(g->srmId.c_str()));

		if (g->compound != NULL) {
			query1.bindValue( 8, QString(g->compound->id.c_str()) );
			query1.bindValue( 9, QString(g->compound->name.c_str()) );
			query1.bindValue( 10, QString(g->compound->db.c_str()) );
		} else{
			query1.bindValue(8,  "" );
			query1.bindValue(9, "" );
			query1.bindValue(10, "" );
		}
    	if(!query1.exec())  qDebug() << query1.lastError();

     QSqlQuery query2(projectDB);
     if(!query2.exec("create table IF NOT EXISTS peaks( \
                    peakId integer primary key AUTOINCREMENT, \
                    groupId int,\
                    sampleId int, \
                    pos int, \
					minpos int, \
					maxpos int, \
					rt float, \
					rtmin float, \
					rtmax float, \
					mzmin float, \
					mzmax float, \
					scan int,	\ 
					minscan int, \
					maxscan int,\
					peakArea float,\
					peakAreaCorrected float,\
					peakAreaTop float,\
					peakAreaFractional float,\
					peakRank float,\
					peakIntensity float,\
					peakBaseLineLevel float,\
					peakMz float,\
					medianMz float,\
					baseMz float,\
					quality float,\
					width int,\
					gaussFitSigma float,\
					gaussFitR2 float,\
					noNoiseObs int,\
					noNoiseFraction float,\
					symmetry float,\
					signalBaselineRatio float,\
					groupOverlap float,\
					groupOverlapFrac float,\
					localMaxFlag float,\
                    fromBlankSample int,\
                    label int)"))  qDebug() << query2.lastError();
			
	 		QSqlQuery query3(projectDB); 
            query3.prepare("insert into peaks values(NULL,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
		
			for(int j=0; j < g->peaks.size(); j++ ) { 
					Peak& p = g->peaks[j];
                    query3.addBindValue(QString::number(g->groupId));
                    query3.addBindValue(p.getSample()->getSampleOrder());
                    query3.addBindValue(QString::number(p.pos));
					query3.addBindValue(QString::number(p.minpos));
					query3.addBindValue(QString::number(p.maxpos));
					query3.addBindValue(QString::number(p.rt));
					query3.addBindValue(QString::number(p.rtmin));
					query3.addBindValue(QString::number(p.rtmax));
					query3.addBindValue(QString::number(p.mzmin));
					query3.addBindValue(QString::number(p.mzmax));
					query3.addBindValue(QString::number((int) p.scan));
					query3.addBindValue(QString::number((int) p.minscan));
					query3.addBindValue(QString::number((int) p.maxscan));
					query3.addBindValue(QString::number(p.peakArea));
					query3.addBindValue(QString::number(p.peakAreaCorrected));
					query3.addBindValue(QString::number(p.peakAreaTop));
					query3.addBindValue(QString::number(p.peakAreaFractional));
					query3.addBindValue(QString::number(p.peakRank));
					query3.addBindValue(QString::number(p.peakIntensity));;
					query3.addBindValue(QString::number(p.peakBaseLineLevel));
					query3.addBindValue(QString::number(p.peakMz));
					query3.addBindValue(QString::number(p.medianMz));
					query3.addBindValue(QString::number(p.baseMz));
					query3.addBindValue(QString::number(p.quality));
					query3.addBindValue(QString::number((int) p.width));
					query3.addBindValue(QString::number(p.gaussFitSigma));
					query3.addBindValue(QString::number(p.gaussFitR2));
					query3.addBindValue(QString::number((int) p.noNoiseObs));
					query3.addBindValue(QString::number(p.noNoiseFraction));
					query3.addBindValue(QString::number(p.symmetry));
					query3.addBindValue(QString::number(p.signalBaselineRatio));
					query3.addBindValue(QString::number(p.groupOverlap));
					query3.addBindValue(QString::number(p.groupOverlapFrac));
					query3.addBindValue(QString::number(p.localMaxFlag));
					query3.addBindValue(QString::number(p.fromBlankSample));
					query3.addBindValue(QString::number(p.label));
    				if(!query3.exec())  qDebug() << query3.lastError();
		}

        if ( g->childCount() ) {
           for(int i=0; i < g->children.size(); i++ ) {
                PeakGroup* child = &(g->children[i]); 
                writeGroupSqlite(child);
            }
        }

    query0.exec("end transaction");
}
