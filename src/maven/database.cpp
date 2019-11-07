#include "database.h"

bool Database::connect(QString filename) {
    qDebug() << "Connecting to " << filename << endl;

    ligandDB = QSqlDatabase::addDatabase("QSQLITE", "ligandDB");
    ligandDB.setDatabaseName(filename);
    ligandDB.open();

    if (!ligandDB.isOpen()) {
        qDebug()  << "Failed to open ligand database" + filename;
        return false;
    } else {
        return true;
    }
}

bool Database::disconnect() {

    qDebug() << "Disconnecting ligand.db";

    ligandDB.close();

    for (auto connection : ligandDB.connectionNames()) {
        qDebug() << "Removing connection " << connection;
        QSqlDatabase::removeDatabase(connection);
    }

    if (ligandDB.isOpen()) {
        qDebug() << "Failed to close ligand database.";
        return false;
    } else {
        return true;
    }
}

void Database::reloadAll() {

	//compounds subsets
    const std::string EmptyString;
    compoundIdMap.clear();
    compoundsDB.clear();
    loadCompoundsSQL("ALL",ligandDB);

    cerr << "compoundsDB=" << compoundsDB.size() << " " << compoundIdMap.size() << endl;
    cerr << "adductsDB=" << adductsDB.size() << endl;
    cerr << "fragmentsDB=" << fragmentsDB.size() << endl;
    cerr << endl;
}

void Database::loadMethodsFolder(QString methodsFolder) {
    QDir dir(methodsFolder);
    if (dir.exists()) {
        dir.setFilter(QDir::Files );
        QFileInfoList list = dir.entryInfoList();
        for (int i = 0; i < list.size(); ++i) {
            QFileInfo fileInfo = list.at(i);
            loadCompoundsFile(fileInfo.absoluteFilePath());
        }
    }

    reloadAll();
}

int Database::loadCompoundsFile(QString filename) {
    string dbname = mzUtils::cleanFilename(filename.toStdString());

    vector<Compound*>compounds;

    if ( filename.endsWith("msp",Qt::CaseInsensitive)
         || filename.endsWith("sptxt",Qt::CaseInsensitive)) {
        compounds = loadNISTLibrary(filename);
    } else {
        compounds = loadCompoundCSVFile(filename);
    }

    deleteCompoundsSQL(dbname.c_str(),ligandDB);
    saveCompoundsSQL(compounds,ligandDB,true);

    sort(compoundsDB.begin(),compoundsDB.end(), Compound::compMass);
    return compounds.size();
}


void Database::closeAll() {
    compoundIdMap.clear();
    mzUtils::delete_all(adductsDB);     adductsDB.clear();
    mzUtils::delete_all(compoundsDB);   compoundsDB.clear();
    mzUtils::delete_all(fragmentsDB);   fragmentsDB.clear();

    disconnect();
}

multimap<string,Compound*> Database::keywordSearch(string needle) { 
    QSqlQuery query(ligandDB);
    query.prepare("SELECT compound_id, keyword from sets where keyword like '%?%'");
	query.addBindValue(needle.c_str());
 	if (!query.exec())   qDebug() << query.lastError();
	

    multimap<string,Compound*>matches;
    while (query.next()) {
        std::string id  = query.value(0).toString().toStdString();
        std::string keyword  = query.value(1).toString().toStdString();
        Compound* cmpd = findSpeciesById(id,"");
        if (cmpd != NULL ) matches.insert(pair<string,Compound*>(keyword,cmpd));
    }
	return matches;
}

void Database::addCompound(Compound* c) { 
    if(c == NULL) return;
    compoundIdMap[c->id + c->db]=c;
    compoundsDB.push_back(c);
}

void Database::loadCompoundsSQL(QString databaseName, QSqlDatabase &dbConnection) {

        if (loadedDatabase.count(databaseName)) {
            qDebug()  << databaseName << "already loaded";
            return;
        }

        QSqlQuery query(dbConnection);
        QString sql = "select * from compounds";
        if (databaseName != "ALL")  sql += " where dbName='" + databaseName + "'";

        bool isHasAdductStringColumn = false;
        QSqlQuery tableInfoQuery(dbConnection);
        tableInfoQuery.exec("PRAGMA table_info(compounds)");
        while(tableInfoQuery.next()) {
            if (tableInfoQuery.value(1).toString() == "adductString") {
                isHasAdductStringColumn = true;
                break;
            }
        }

        query.prepare(sql);
        if(!query.exec()) qDebug() << "loadCompoundsSQL: query error " << query.lastError();
        MassCalculator mcalc;

        int loadcount=0;
        while (query.next()) {
            string id =   query.value("compoundId").toString().toStdString();
            string name = query.value("name").toString().toStdString();
            string formula = query.value("formula").toString().toStdString();
            int charge = query.value("charge").toInt();
            float exactMass = query.value("mass").toDouble();
	    int cid  =  query.value("cid").toInt();
            string db   =  query.value("dbName").toString().toStdString();
            double expectedRt =  query.value("expectedRt").toDouble();

	    //skip already exists in internal database
    	    if ( compoundIdMap.contains(id + db) ) continue;

            //the neutral mass is computated automatically  inside the constructor
            Compound* compound = new Compound(id,name,formula,charge);
            compound->cid  =  cid;
            compound->db   =  db;
            compound->expectedRt =  expectedRt;

            auto boundValues = query.boundValues();

            //To avoid breaking with old versions
            if (isHasAdductStringColumn) {
                compound->adductString = query.value("adductString").toString().toStdString();

                if (compound->charge == 0){
                    if (!compound->adductString.empty()){
                       if (compound->adductString.compare (compound->adductString.length() - 1, 1, "+") == 0){
                           compound->charge = 1;
                       } else if (compound->adductString.compare (compound->adductString.length() - 1, 1, "-") == 0) {
                           compound->charge = -1;
                       }
                    }
                }
            }

            if (formula.empty()) {
                if(exactMass >0) compound->setExactMass(exactMass);
            } else {
                 compound->setExactMass(mcalc.computeNeutralMass(formula));
            }

            compound->precursorMz =  query.value("precursorMz").toDouble();
            compound->productMz =  query.value("productMz").toDouble();
            compound->collisionEnergy =  query.value("collisionEnergy").toDouble();
            compound->smileString=  query.value("smileString").toString().toStdString();

            //mark compound as decoy if names contains DECOY string
            if(compound->name.find("DECOY") > 0) compound->isDecoy=true;

            for(QString f: query.value("category").toString().split(";") ) {
                compound->category.push_back(f.toStdString());
            }

            for(QString f: query.value("fragment_mzs").toString().split(";") ) {
                if(f.toDouble()) {
                    compound->fragment_mzs.push_back(f.toDouble());
                }
            }

            for(QString f: query.value("fragment_intensity").toString().split(";") ) {
                if(f.toDouble()) {
                    compound->fragment_intensity.push_back(f.toDouble());
                }
            }

            compoundIdMap[compound->id + compound->db]=compound;
            compoundsDB.push_back(compound);
            loadcount++;
        }

        if (loadcount > 0) {
            bool isHasUnequalMatches = false;
            for (Compound *cpd : compoundsDB) {
                if (cpd->fragment_mzs.size() != cpd->fragment_intensity.size()){
                    isHasUnequalMatches = true;
                    cerr << "COMPOUND: " << cpd->name << ": #mzs=" << cpd->fragment_mzs.size() << "; #ints=" << cpd->fragment_intensity.size() << endl;
                }
            }

            cerr << "COMPOUND LIBRARY HAS ANY COMPOUNDS WITH MISMATCHED (m/z, intensity) PAIRS? " << (isHasUnequalMatches ? "yes" : "no") << endl;
            if (isHasUnequalMatches) {
                qDebug() << "One or more compounds in the library" << databaseName << "does not have proper corresponding (m/z, intensity) pairs in its database.";
                qDebug() << "Please delete this library and reload it.";
                qDebug() << "Exiting Program.";
                abort();
            }

            for (Compound *cpd : compoundsDB) {

                //sort all compounds by m/z (and re-order corresponding intensity vector)
                vector<pair<float,int>> pairsArray = vector<pair<float,int>>(cpd->fragment_mzs.size());
                for (unsigned int pos = 0; pos < cpd->fragment_mzs.size(); pos++){
                    pairsArray.at(pos) = make_pair(cpd->fragment_mzs.at(pos), pos);
                }

                sort(pairsArray.begin(), pairsArray.end());

                vector<float> sortedMzs = vector<float>(pairsArray.size());
                vector<float> sortedIntensities = vector<float>(pairsArray.size());

                for (unsigned int pos = 0; pos < pairsArray.size(); pos++) {
                    sortedMzs.at(pos) = pairsArray.at(pos).first;
                    sortedIntensities.at(pos) = cpd->fragment_intensity.at(pairsArray.at(pos).second);
                }

                cpd->fragment_mzs = sortedMzs;
                cpd->fragment_intensity = sortedIntensities;
            }

            sort(compoundsDB.begin(),compoundsDB.end(), Compound::compMass);
            qDebug() << "loadCompoundSQL : " << databaseName << compoundsDB.size();
        }

        if(loadcount > 0 and databaseName != "ALL") loadedDatabase[databaseName] = 1;
}

set<Compound*> Database::findSpeciesByMass(float mz, float ppm) { 
	set<Compound*>uniqset;

    Compound x("find", "", "",0);
    x.setExactMass(mz-(mz/1e6*ppm));
    vector<Compound*>::iterator itr = lower_bound(
            compoundsDB.begin(),compoundsDB.end(), 
            &x, Compound::compMass );

    for(;itr != compoundsDB.end(); itr++ ) {
        Compound* c = *itr;
        if (c->getExactMass() > mz+1) break;

        if ( mzUtils::ppmDist(c->getExactMass(),mz) < ppm ) {
			if (uniqset.count(c)) continue;
            uniqset.insert(c);
        }
    }
    return uniqset;
}

Adduct* Database::findAdductByName(string id) {

    if(id == "[M+H]+")  return MassCalculator::PlusHAdduct;
    else if(id == "[M-H]-")  return MassCalculator::MinusHAdduct;
    else if(id == "[M]") return MassCalculator::ZeroMassAdduct;

    for(Adduct* a: this->adductsDB)  if (a->name == id) return a;
    return nullptr;
}

Compound* Database::findSpeciesById(string id,string db) {

    if (!loadedDatabase.count(db.c_str()))  loadCompoundsSQL(db.c_str(),ligandDB);

    //cerr << "searching for " << id << " " << compoundIdMap.size() << " " << db << endl;
    if ( compoundIdMap.contains(id + db) ) return compoundIdMap[id + db];
    if ( compoundIdMap.contains(id) ) return compoundIdMap[id];

    return NULL;

    /*Compound* c = NULL;
    for(int i=0; i < compoundsDB.size(); i++ ) {
        if (compoundsDB[i]->id == id ) { c = compoundsDB[i]; break; }
    }
    return c;
    */
}


vector<Compound*> Database::findSpeciesByName(string name, string dbname) {
        if (!loadedDatabase.count(dbname.c_str()))  loadCompoundsSQL(dbname.c_str(),ligandDB);

        vector<Compound*> set;
        qDebug() << "findSpeciesByName" << name.c_str();
		for(unsigned int i=0; i < compoundsDB.size(); i++ ) {
				if (compoundsDB[i]->name == name && compoundsDB[i]->db == dbname) {
					set.push_back(compoundsDB[i]);
				}
		}
		return set;
}

Compound* Database::findSpeciesByPrecursor(float precursorMz, float productMz, int polarity,double amuQ1, double amuQ3) {
		Compound* x=NULL;
		float dist=FLT_MAX;

		for(unsigned int i=0; i < compoundsDB.size(); i++ ) {
				if (compoundsDB[i]->precursorMz == 0 ) continue;
				//cerr << polarity << " " << compoundsDB[i]->charge << endl;
				if ((int) compoundsDB[i]->charge != polarity ) continue;
				float a = abs(compoundsDB[i]->precursorMz - precursorMz);
				if ( a > amuQ1 ) continue; // q1 tollorance
				float b = abs(compoundsDB[i]->productMz - productMz);
				if ( b > amuQ3 ) continue; // q2 tollarance
				float d = sqrt(a*a+b*b);
				if ( d < dist) { x = compoundsDB[i]; dist=d; }
		}
		return x;
}

vector<MassCalculator::Match> Database::findMatchingCompounds(float mz, float ppm, float charge) {
    vector<MassCalculator::Match>uniqset;

    //compute plausable adducts
    for(Adduct* a: adductsDB ) {
            if (SIGN(a->charge) != SIGN(charge)) continue;
            float pmz = a->computeParentMass(mz);
            set<Compound*>hits = this->findSpeciesByMass(pmz,ppm);

            for(Compound* c: hits) {
            float adductMass=a->computeAdductMass(c->getExactMass());
            MassCalculator::Match match;
            match.name = c->name + " : " + a->name;
            match.adductLink = a;
            match.compoundLink = c;
            match.mass=adductMass;
            match.diff = mzUtils::ppmDist((double) adductMass, (double) mz);
            uniqset.push_back(match);
            //cerr << "F:" << match.name << " " << match.diff << endl;
        }
     }
    return uniqset;
}

vector<MassCalculator::Match> Database::findMathchingCompoundsSLOW(float mz, float ppm, float charge) {
    vector<MassCalculator::Match>uniqset;
    MassCalculator mcalc;

    for(Compound* c: compoundsDB) {
        float parentMass = mcalc.computeNeutralMass(c->formula);
        for(Adduct* a: adductsDB ) {
            if (SIGN(a->charge) != SIGN(charge)) continue;
                float adductMass=a->computeAdductMass(parentMass);
                if (mzUtils::ppmDist((double) adductMass, (double) mz) < ppm ) {
                    MassCalculator::Match match;
                    match.name = c->name + " : " + a->name;
                    match.adductLink = a;
                    match.compoundLink = c;
                    match.mass=adductMass;
                    match.diff = mzUtils::ppmDist((double) adductMass, (double) mz);
                    uniqset.push_back(match);
                    //cerr << "S:" << match.name << " " << match.diff << endl;
                }
        }
     }
    return uniqset;
}

void Database::saveRetentionTime(Compound* c, float rt, QString method) {
	if (!c) return;

	cerr << "setExpectedRetentionTime() " << rt << endl;
	//QSqlDatabase db = QSqlDatabase::database("ligand.db");
	 ligandDB.transaction();
     QSqlQuery query(ligandDB);

	query.prepare("insert into knowns_times values (?,?,?)");
	query.addBindValue(QString(c->id.c_str()));
	query.addBindValue(method);
	query.addBindValue(QString::number(rt,'f',4) );

	if(!query.exec()) qDebug() << query.lastError();

	ligandDB.commit();
	query.clear();
}

vector<Compound*> Database::getCopoundsSubset(string dbname) {
	vector<Compound*> subset;
	for (unsigned int i=0; i < compoundsDB.size(); i++ ) {
			if (compoundsDB[i]->db == dbname) { 
					subset.push_back(compoundsDB[i]);
			}
	}
	return subset;
}

QStringList Database::getDatabaseNames() {
    QSqlQuery query(ligandDB);
    query.prepare("SELECT distinct dbName from compounds");
    if (!query.exec())  qDebug() << query.lastError();

    QStringList dbnames;
    while (query.next())  dbnames << query.value(0).toString();
    return dbnames;
}

vector<Adduct*> Database::loadAdducts(string filename) {
    vector<Adduct*> adducts;
    ifstream myfile(filename.c_str());

    if (! myfile.is_open()) return adducts;

    string line;
    while ( getline(myfile,line) ) {
        if (line.empty()) continue;
        if (line[0] == '#') continue;

        vector<string>fields;
        mzUtils::split(line,',', fields);

        //ionization
        if(fields.size() < 2 ) continue;
        string name=fields[0];
        int nmol=string2float(fields[1]);
        int charge=string2float(fields[2]);
        float mass=string2float(fields[3]);

        if ( name.empty() || nmol < 0 ) continue;
        Adduct* a = new Adduct(name,mass,charge,nmol);
        adducts.push_back(a);
    }
    cerr << "loadAdducts() " << filename << " count=" << adducts.size() << endl;
    return adducts;
    myfile.close();
}

vector<Adduct*> Database::defaultAdducts() {
    vector<Adduct*> adducts;
    adducts.push_back( new Adduct("[M-H]+",  PROTON , 1, 1));
    adducts.push_back( new Adduct("[M-H]-", -PROTON, -1, 1));
    adducts.push_back( new Adduct("[M]",0 ,1, 1));
    return adducts;
}



vector<Compound*> Database::loadCompoundCSVFile(QString fileName){

    vector<Compound*> compoundSet; //return

    QFile data(fileName);
    if (!data.open(QFile::ReadOnly) ) {
        qDebug() << "Can't open " << fileName; exit(-1);
        return compoundSet;
    }

    string dbname = mzUtils::cleanFilename(fileName.toStdString());
    QMap<QString, int>header;
    QVector<QString> headers;
    MassCalculator mcalc;

    //assume that files are tab delimited, unless matched ".csv", then comma delimited
    char sep='\t';
    if(fileName.endsWith(".csv",Qt::CaseInsensitive)) sep = ',';
    QTextStream stream(&data);

    QString line;
    int loadCount=0;
    int lineCount=0;

    do {
        line = stream.readLine().trimmed();
        if (line.isEmpty() || line[0] == '#') continue;

        lineCount++;
        //vector<string>fields;
        //mzUtils::split(line.toStdString(), sep, fields);

        QStringList fieldsA = line.split(sep);
        QVector<QString> fields = fieldsA.toVector();

        for(unsigned int i=0; i < fields.size(); i++ ) {
            int n = fields[i].length();
            if (n>2 && fields[i][0] == '"' && fields[i][n-1] == '"') {
                fields[i]= fields[i].replace("\"","");
            }
            if (n>2 && fields[i][0] == '\'' && fields[i][n-1] == '\'') {
                fields[i]= fields[i].replace("\"","");
            }
            fields[i] = fields[i].trimmed();
        }

        if (lineCount==1) {
            headers = fields;
            for(unsigned int i=0; i < fields.size(); i++ ) {
                fields[i]=fields[i].toLower();
                header[ fields[i] ] = i;
            }
            continue;
        }

        string id, name, formula,smile;
        float rt=0;
        float mz=0;
        float charge=0;
        float collisionenergy=0;
        float precursormz=0;
        float productmz=0;
        float logP=0;
        int N=fields.size();
        vector<string>categorylist;
        //qDebug() << N << line;

        if ( header.count("mz") && header["mz"]<N)  mz = fields[ header["mz"]].toDouble();
        if ( header.count("rt") && header["rt"]<N)  rt = fields[ header["rt"]].toDouble();
        if ( header.count("expectedrt") && header["expectedrt"]<N) rt = fields[ header["expectedrt"]].toDouble();
        if ( header.count("charge")&& header["charge"]<N) charge = fields[ header["charge"]].toDouble();
        if ( header.count("formula")&& header["formala"]<N) formula = fields[ header["formula"] ].toStdString();
        if ( header.count("id")&& header["id"]<N) 	 id = fields[ header["id"] ].toStdString();
        if ( header.count("name")&& header["name"]<N) 	 name = fields[ header["name"] ].toStdString();
        if ( header.count("compound")&& header["compound"]<N) 	 name = fields[ header["compound"] ].toStdString();
        if ( header.count("metabolite")&& header["metabolite"]<N) 	 name = fields[ header["compound"] ].toStdString();
        if ( header.count("smile")&& header["smile"]<N) 	 smile = fields[ header["smile"] ].toStdString();
        if ( header.count("logp")&& header["logp"]<N) 	 logP = fields[ header["logp"] ].toDouble();

        if ( header.count("precursormz") && header["precursormz"]<N) precursormz=fields[ header["precursormz"]].toDouble();
        if ( header.count("productmz") && header["productmz"]<N)  productmz = fields[header["productmz"]].toDouble();
        if ( header.count("collisionenergy") && header["collisionenergy"]<N) collisionenergy=fields[ header["collisionenergy"]].toDouble();

        if ( header.count("Q1") && header["Q1"]<N) precursormz=fields[ header["Q1"]].toDouble();
        if ( header.count("Q3") && header["Q3"]<N)  productmz = fields[header["Q3"]].toDouble();
        if ( header.count("CE") && header["CE"]<N) collisionenergy=fields[ header["CE"]].toDouble();

        //cerr << lineCount << " " << endl;
        //for(int i=0; i<headers.size(); i++) cerr << headers[i] << ", ";
        //cerr << "   -> category=" << header.count("category") << endl;
        if ( header.count("category") && header["category"]<N) {
            QString catstring=fields[header["category"]];
            if (!catstring.isEmpty()) {
                for(QString x: catstring.split(";"))  categorylist.push_back(x.toStdString());
            }
         }

        if ( header.count("polarity") && header["polarity"] <N)  {
            QString x = fields[ header["polarity"]];
            if ( x == "+" ) {
                charge = 1;
            } else if ( x == "-" ) {
                charge = -1;
            } else  {
                charge = x.toInt();
            }

        }

        //cerr << "Loading: " << id << " " << formula << "mz=" << mz << " rt=" << rt << " charge=" << charge << endl;

        if (mz == 0) mz=precursormz;
        if (id.empty()&& !name.empty()) id=name;
        if (id.empty() && name.empty()) id="cmpd:" + integer2string(loadCount);

        if ( mz > 0 || ! formula.empty() ) {
            Compound* compound = new Compound(id,name,formula,charge);

            compound->expectedRt = rt;
            if (mz == 0) mz = mcalc.computeMass(formula,charge);
            compound->setExactMass(mz);
            compound->db = dbname;
            compound->expectedRt=rt;
            compound->precursorMz=precursormz;
            compound->productMz=productmz;
            compound->collisionEnergy=collisionenergy;
            compound->smileString=smile;
            compound->logP=logP;
            for(int i=0; i < categorylist.size(); i++) compound->category.push_back(categorylist[i]);
            compoundSet.push_back(compound);
            //addCompound(compound);
            loadCount++;
            //cerr << "Loading: " << id << " " << formula << "mz=" << compound->mass << " rt=" << compound->expectedRt << " charge=" << charge << endl;
        }
    } while (!line.isNull());

    qDebug() << "Read in" << lineCount << " lines." <<  " compounds=" << loadCount << endl;

    sort(compoundSet.begin(),compoundSet.end(), Compound::compMass);
    return compoundSet;
}

vector<Compound*> Database::loadNISTLibrary(QString fileName) {
    qDebug() << "Loading NIST Libary: " << fileName;
    QFile data(fileName);

    vector<Compound*> compoundSet;
    if (!data.open(QFile::ReadOnly) ) {
        qDebug() << "Can't open " << fileName; exit(-1);
        return compoundSet;
    }

    string dbfilename = fileName.toStdString();
    string dbname = mzUtils::cleanFilename(dbfilename);

   QTextStream stream(&data);
   QRegExp whiteSpace("\\s+");
   QRegExp formulaMatch("(C\\d+H\\d+\\S*)");
   QRegExp retentionTimeMatch("AvgRt\\=(\\S+)");

   QString line;
   MassCalculator mcalc;
   Compound* cpd = nullptr;
   bool capturePeaks=false;

    do {
        line = stream.readLine();
        if(line.startsWith("Name:",Qt::CaseInsensitive)){

            if(cpd and !cpd->name.empty()) {
                //cerr << "NIST LIBRARY:" << cpd->db << " " << cpd->name << " " << cpd->formula << " " << cpd->id << endl;
                if(!cpd->formula.empty())  cpd->setExactMass( mcalc.computeMass(cpd->formula,0));
                compoundSet.push_back(cpd);
            }

            //NEW COMPOUND
            QString name = line.mid(5,line.length()).simplified();
            cpd = new Compound(name.toStdString(), name.toStdString(),"", 0);
            cpd->db = dbname;
            capturePeaks=false;
        }

        if(cpd == nullptr) continue;

        if (line.startsWith("MW:",Qt::CaseInsensitive)) {
            cpd->setExactMass( line.mid(3,line.length()).simplified().toDouble());
         } else if (line.startsWith("CE:",Qt::CaseInsensitive)) {
            cpd->collisionEnergy = line.mid(3,line.length()).simplified().toDouble();
         } else if (line.startsWith("ID:",Qt::CaseInsensitive)) {
            QString id = line.mid(3,line.length()).simplified();
            if(!id.isEmpty()) cpd->id = id.toStdString();
         } else if (line.startsWith("LOGP:",Qt::CaseInsensitive)) {
            cpd->logP = line.mid(5,line.length()).simplified().toDouble();
        } else if (line.startsWith("RT:",Qt::CaseInsensitive)) {
           cpd->expectedRt= line.mid(3,line.length()).simplified().toDouble();
        } else if (line.startsWith("SMILE:",Qt::CaseInsensitive)) {
            QString smileString = line.mid(7,line.length()).simplified();
            if(!smileString.isEmpty()) cpd->smileString=smileString.toStdString();
         } else if (line.startsWith("SMILES:",Qt::CaseInsensitive)) {
            QString smileString = line.mid(8,line.length()).simplified();
            if(!smileString.isEmpty()) cpd->smileString=smileString.toStdString();
         } else if (line.startsWith("PRECURSORMZ:",Qt::CaseInsensitive)) {
            cpd->precursorMz = line.mid(13,line.length()).simplified().toDouble();
         } else if (line.startsWith("EXACTMASS:",Qt::CaseInsensitive)) {
            float exactMass = line.mid(10,line.length()).simplified().toDouble();
            cpd->setExactMass(exactMass);
         } else if (line.startsWith("ADDUCT:",Qt::CaseInsensitive)) {
            cpd->adductString = line.mid(8,line.length()).simplified().toStdString();

            if (cpd->charge == 0) {
                if (cpd->adductString.compare (cpd->adductString.length() - 1, 1, "+") == 0){
                    cpd->charge = 1;
                } else if (cpd->adductString.compare (cpd->adductString.length() - 1, 1, "-") == 0) {
                    cpd->charge = -1;
                }
            }

         } else if (line.startsWith("FORMULA:",Qt::CaseInsensitive)) {
             QString formula = line.mid(9,line.length()).simplified();
             formula.replace("\"","",Qt::CaseInsensitive);
             if(!formula.isEmpty()) cpd->formula = formula.toStdString();
         } else if (line.startsWith("molecule formula:",Qt::CaseInsensitive)) {
             QString formula = line.mid(17,line.length()).simplified();
             formula.replace("\"","",Qt::CaseInsensitive);
             if(!formula.isEmpty()) cpd->formula = formula.toStdString();
         } else if (line.startsWith("CATEGORY:",Qt::CaseInsensitive)) {
             cpd->category.push_back(line.mid(10,line.length()).simplified().toStdString());
         } else if (line.startsWith("Tag:",Qt::CaseInsensitive)) {
            if(line.contains("virtual",Qt::CaseInsensitive)) cpd->virtualFragmentation=true;
         } else if (line.startsWith("ion mode:",Qt::CaseInsensitive)) {
            if(line.contains("neg",Qt::CaseInsensitive)) cpd->ionizationMode=-1;
            if(line.contains("pos",Qt::CaseInsensitive)) cpd->ionizationMode=+1;

             QString comment = line.mid(8,line.length()).simplified();
             if (comment.contains(formulaMatch)){
                 cpd->formula=formulaMatch.capturedTexts().at(1).toStdString();
                 //qDebug() << "Formula=" << formula;
             }
            if (comment.contains(retentionTimeMatch)){
               cpd->expectedRt=retentionTimeMatch.capturedTexts().at(1).simplified().toDouble();
                 //qDebug() << "retentionTime=" << retentionTimeString;
             }
         } else if (line.startsWith("Num Peaks:",Qt::CaseInsensitive) || line.startsWith("NumPeaks:",Qt::CaseInsensitive)) {
             capturePeaks=true;
         } else if (capturePeaks ) {
             QStringList mzintpair = line.split(whiteSpace);
             if( mzintpair.size() >=2 ) {
                 bool ok=false; bool ook=false;
                 float mz = mzintpair.at(0).toDouble(&ok);
                 float ints = mzintpair.at(1).toDouble(&ook);
                 if (ok && ook && mz >= 0 && ints >= 0) {
                     cpd->fragment_intensity.push_back(ints);
                     cpd->fragment_mzs.push_back(mz);
                     int frag_indx = cpd->fragment_mzs.size()-1;
					 if(mzintpair.size() >= 3) {
						 cpd->fragment_iontype[frag_indx] = mzintpair.at(2).toStdString();
					}
                 }
             }
         }

    } while (!line.isNull());

    if (cpd) compoundSet.push_back(cpd);

    bool isHasUnequalMatches = false;
    for (Compound *cpd : compoundSet) {
        if (cpd->fragment_mzs.size() != cpd->fragment_intensity.size()){
            isHasUnequalMatches = true;
            cerr << "COMPOUND: " << cpd->name << ": #mzs=" << cpd->fragment_mzs.size() << "; #ints=" << cpd->fragment_intensity.size() << endl;
        }
    }

    cerr << "COMPOUND LIBRARY HAS ANY COMPOUNDS WITH MISMATCHED (m/z, intensity) PAIRS? " << (isHasUnequalMatches ? "yes" : "no") << endl;
    if (isHasUnequalMatches) {
        qDebug() << "One or more compounds in the library" << fileName << "does not have proper corresponding (m/z, intensity) pairs in its database.";
        qDebug() << "Please delete this library and reload it.";
        qDebug() << "Exiting Program.";
        abort();
    }

    //compoundSet.push_back(compound);
    qDebug() << "Database::loadNISTLibrary() in" << compoundSet.size()  << " compounds.";
    //sort(compoundSet.begin(),compoundSet.end(), Compound::compMass);
    return compoundSet;
}

void Database::deleteAllCompoundsSQL(QSqlDatabase& dbConnection) {
    QSqlQuery query(dbConnection);
    query.prepare("drop table compounds");
    if (!query.exec())  qDebug() << query.lastError();
    dbConnection.commit();
    qDebug() << "deleteAllCompounds: done";
}


void Database::deleteCompoundsSQL(QString dbName, QSqlDatabase& dbConnection) {
    QSqlQuery query(dbConnection);

    //create index based on database name.
     query.exec("create index if not exists compound_db_idx on compounds(dbName)");

    query.prepare("delete from compounds where dbName = ?");
    query.bindValue( 0, dbName );
    if (!query.exec())  qDebug() << query.lastError();
    dbConnection.commit();
    qDebug() << "deleteCompoundsSQL" << dbName <<  " " << query.numRowsAffected();
}


void Database::saveCompoundsSQL(vector<Compound*> &compoundSet, QSqlDatabase& dbConnection, bool isUpdateOldDBVersion) {
    qDebug() << "saveCompoundsSQL()" << compoundSet.size();

    /*
     * START CHECK DB VERSION
     *
     * ENSURE DB HAS CORRECT SCHEMA
     * IF IT DOES NOT, DELETE AND START OVER PRIOR TO WRITING THIS COMPOUND SET
     *
     * APPLIES BOTH To ligand.db and .mzrollDB files.
     */

    QSqlQuery queryCheckCols(dbConnection);

    if (!queryCheckCols.exec("pragma table_info(compounds)")) {
        qDebug() << "Ho..." << queryCheckCols.lastError();
    }

    bool isHasExistingCols = false;
    bool isHasAdductString = false;

    while (queryCheckCols.next()){
        isHasExistingCols = true;
        if ("adductString" == queryCheckCols.value(1).toString()){
            isHasAdductString = true;
            break;
        }
    }

    bool isOldVersion = isHasExistingCols && !isHasAdductString;

    if (isOldVersion && isUpdateOldDBVersion) {
        qDebug() << "compounds table of DB is not at most current version. Removing all old compounds via deleteAllCompoundsSQL()";
        deleteAllCompoundsSQL(dbConnection);
        isOldVersion = false;
    }

    /*
     *END CHECK DB VERSION
     */

    QSqlQuery query0(dbConnection);
    query0.exec("begin transaction");
    if(!query0.exec("create table IF NOT EXISTS compounds(\
                    cid integer primary key AUTOINCREMENT,\
                    dbName varchar(255),\
                    compoundId varchar(255),\
                    name varchar(255),\
                    adductString varchar(255),\
                    formula varchar(255),\
                    smileString  varchar(255),\
                    srmId  varchar(255),\
                    mass float,\
                    charge  int,\
                    expectedRt float, \
                    precursorMz float,\
                    productMz   float,\
                    collisionEnergy float,\
                    logP float,\
                    virtualFragmentation int,\
                    ionizationMode int,\
                    category varchar(255),\
                    fragment_mzs text, \
                    fragment_intensity text \
                    )"))  qDebug() << "Ho... " << query0.lastError();

        query0.exec("create index if not exists compound_db_idx on compounds(dbName)");

        for(Compound* c : compoundSet) {
            QStringList cat;
            QStringList fragMz;
            QStringList fragIntensity;

            for(string s : c->category) { cat << s.c_str(); }

            for (unsigned int i = 0; i < c->fragment_mzs.size(); i++) {
                float mz = c->fragment_mzs[i];
                float intensity = c->fragment_intensity[i];

                QString mzQString = QString::number(mz, 'f', 5);
                QString intensityQString = QString::number(intensity, 'f', 5);

                if (mzQString != "0.00000" && intensityQString != "0.00000") {
                    fragMz << mzQString;
                    fragIntensity << intensityQString;
                }
            }

	    QVariant cid =  QVariant(QVariant::Int);
        if (c->cid)cid = QVariant(c->cid);

            QSqlQuery query1(dbConnection);

            if (isOldVersion) {
                query1.prepare("replace into compounds values(?,?,?,?,?, ?,?,?,?,?, ?,?,?,?,?, ?,?,?,?)");

                query1.bindValue( 0, cid);
                query1.bindValue( 1, QString(c->db.c_str()) );
                query1.bindValue( 2, QString(c->id.c_str()) );
                query1.bindValue( 3, QString(c->name.c_str()));

                query1.bindValue( 4, QString(c->formula.c_str()));
                query1.bindValue( 5, QString(c->smileString.c_str()));
                query1.bindValue( 6, QString(c->srmId.c_str()));

                query1.bindValue( 7, c->getExactMass() );
                query1.bindValue( 8, c->charge);
                query1.bindValue( 9, c->expectedRt);
                query1.bindValue( 10, c->precursorMz);
                query1.bindValue( 11, c->productMz);

                query1.bindValue( 12, c->collisionEnergy);
                query1.bindValue( 13, c->logP);
                query1.bindValue( 14, c->virtualFragmentation);
                query1.bindValue( 15, c->ionizationMode);
                query1.bindValue( 16, cat.join(";"));

                query1.bindValue( 17, fragMz.join(";"));
                query1.bindValue( 18, fragIntensity.join(";"));

            } else {
                query1.prepare("replace into compounds values(?,?,?,?,?, ?,?,?,?,?, ?,?,?,?,?, ?,?,?,?,?)");

                query1.bindValue( 0, cid);
                query1.bindValue( 1, QString(c->db.c_str()) );
                query1.bindValue( 2, QString(c->id.c_str()) );
                query1.bindValue( 3, QString(c->name.c_str()));
                query1.bindValue( 4, QString(c->adductString.c_str()));

                query1.bindValue( 5, QString(c->formula.c_str()));
                query1.bindValue( 6, QString(c->smileString.c_str()));
                query1.bindValue( 7, QString(c->srmId.c_str()));

                query1.bindValue( 8, c->getExactMass() );
                query1.bindValue( 9, c->charge);
                query1.bindValue( 10, c->expectedRt);
                query1.bindValue( 11, c->precursorMz);
                query1.bindValue( 12, c->productMz);

                query1.bindValue( 13, c->collisionEnergy);
                query1.bindValue( 14, c->logP);
                query1.bindValue( 15, c->virtualFragmentation);
                query1.bindValue( 16, c->ionizationMode);
                query1.bindValue( 17, cat.join(";"));

                query1.bindValue( 18, fragMz.join(";"));
                query1.bindValue( 19, fragIntensity.join(";"));
            }

            if(!query1.exec())  qDebug() << query1.lastError();
        }

    query0.exec("end transaction");
}
