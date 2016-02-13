#include "projectDB.h"

void ProjectDB::saveGroups(vector<PeakGroup>& allgroups, QString setName) {
    for(unsigned int i=0; i < allgroups.size(); i++ ) {
        writeGroupSqlite(&allgroups[i]);
    }
}

void ProjectDB::deleteAll() {
    QSqlQuery query(projectDB);
    query.exec("delete from peakgroups");
    query.exec("delete from samples");
    query.exec("delete from peaks");
}

void ProjectDB::saveSamples(vector<mzSample *> &sampleSet) {
    QSqlQuery query0(projectDB);

    query0.exec("begin transaction");
    if(!query0.exec("create table IF NOT EXISTS samples(\
                    sampleId integer primary key AUTOINCREMENT,\
                    name varchar(255),\
                    filename varchar(255),\
                    setName varchar(255),\
                    sampleOrder int,\
                    isSelected  int,\
                    color_red float,\
                    color_blue float,\
                    color_green float,\
                    color_alpha float,\
                    transform_a0 float,\
                    transform_a1 float,\
                    transform_a2 float,\
                    transform_a4 float,\
                    transform_a5 float \
                    )"))  qDebug() << "Ho... " << query0.lastError();

        QSqlQuery query1(projectDB);
        query1.prepare("insert into samples values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");

        for(mzSample* s : sampleSet) {
            query1.bindValue( 0, s->getSampleOrder() );
            query1.bindValue( 1, QString(s->getSampleName().c_str()) );
            query1.bindValue( 2, QString(s->fileName.c_str()));
            query1.bindValue( 3, QString(s->getSetName().c_str()));

            query1.bindValue( 4, s->getSampleOrder() );
            query1.bindValue( 5, s->isSelected );

            query1.bindValue( 6, s->color[0] );
            query1.bindValue( 7, s->color[1] );
            query1.bindValue( 8, s->color[2] );
            query1.bindValue( 9, s->color[3] );

            query1.bindValue( 10,  0);
            query1.bindValue( 11,  0);
            query1.bindValue( 12,  0);
            query1.bindValue( 13,  0);
            query1.bindValue( 14,  0);

            if(!query1.exec())  qDebug() << query1.lastError();
        }
    query0.exec("end transaction");
}

void ProjectDB::writeGroupSqlite(PeakGroup* g) {
	if (!g) return;

     QSqlQuery query0(projectDB);
     query0.exec("begin transaction");
     if(!query0.exec("create table IF NOT EXISTS peakgroups( \
                        groupId integer primary key AUTOINCREMENT, \
                        tagString varchar(255), \
                        metaGroupId int, \
                        expectedRtDiff float, \
                        groupRank i snt, \
                        label varchar(10),\
                        type int,\
                        srmId varchar(255),\
                        compoundId varchar(255),\
                        compoundName varchar(255),\
						compoundDB varchar(255)\ 
                        )"))  qDebug() << query0.lastError();

     //cerr << "inserting .. " << g->groupId << endl;
	 QSqlQuery query1(projectDB);
        query1.prepare("insert into peakgroups values(NULL,?,?,?,?,?,?,?,?,?,?)");
        //query1.bindValue( 0, QString::number(g->groupId));
        query1.bindValue( 0, QString(g->tagString.c_str()));
        query1.bindValue( 1, QString::number(g->metaGroupId));
        query1.bindValue( 2, QString::number(g->expectedRtDiff,'f',2));
        query1.bindValue( 3, QString::number(g->groupRank));
        query1.bindValue( 4, QString(g->label));
        query1.bindValue( 5, QString::number(g->type()));
        query1.bindValue( 6, QString(g->srmId.c_str()));

        if (g->compound != NULL) {
        query1.bindValue( 7, QString(g->compound->id.c_str()) );
        query1.bindValue( 8, QString(g->compound->name.c_str()) );
        query1.bindValue( 9, QString(g->compound->db.c_str()) );
} else{
        query1.bindValue(8,  "" );
        query1.bindValue(9,  "" );
        query1.bindValue(10, "" );
}

     query1.exec();
     int lastInsertGroupId = query1.lastInsertId().toString().toInt();

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
                    query3.addBindValue(QString::number(lastInsertGroupId));
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


void ProjectDB::loadPeakGroups(QString tableName) {
     tableName = "peakgroups";
     vector<PeakGroup> groups;

     QSqlQuery query(projectDB);
     query.exec("select * from peakgroups");

     while (query.next()) {
        PeakGroup g;
        g.groupId = query.value("groupId").toInt();
        g.tagString = query.value("tagString").toString().toStdString();
        g.metaGroupId = query.value("metaGroupId").toString().toInt();
        g.expectedRtDiff = query.value("expectedRtDiff").toString().toDouble();
        g.groupRank = query.value("groupRank").toString().toInt();
        g.label     =  query.value("label").toString().toInt();
        g.setType( (PeakGroup::GroupType) query.value("type").toString().toInt());

        string compoundId = query.value("compoundId").toString().toStdString();
        string compoundDB = query.value("compoundDB").toString().toStdString();
        string compoundName = query.value("compoundName").toString().toStdString();

        if(!compoundName.empty()) g.tagString=compoundName;

        string srmId = query.value("srmId").toString().toStdString();
        if (!srmId.empty()) g.setSrmId(srmId);

        if (!compoundId.empty()){
            Compound* c = DB.findSpeciesById(compoundId);
            if (c) g.compound = c;
        } else if (!compoundName.empty() && !compoundDB.empty()) {
            vector<Compound*>matches = DB.findSpeciesByName(compoundName,compoundDB);
            if (matches.size()>0) g.compound = matches[0];
        }

        loadGroupPeaks(&g);
        allgroups.push_back(g);
    }
}

void ProjectDB::loadGroupPeaks(PeakGroup* parent) {

     QSqlQuery query(projectDB);
     query.prepare("select P.*, S.name as sampleName from peaks P, samples S where P.sampleId = S.sampleId and P.groupId = ?");
     query.bindValue(0,parent->groupId);
     qDebug() << "loadin peaks for group " << parent->groupId;
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
            cerr << "\t\t\t" << p.getSample() << " " << p.rt << endl;
            parent->addPeak(p);
        }
}
