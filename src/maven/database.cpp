#include <QElapsedTimer>
#include "database.h"
#include <unordered_map>
#include "lipidsummarizationutils.h"

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
    saveCompoundsSQL(compounds,ligandDB);

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
    if(!c) return;
    compoundIdMap[c->id + c->db]=c;
    compoundsDB.push_back(c);
}

void Database::unloadCompounds(QString databaseName) {

    loadedDatabase.remove(databaseName);

    string databaseNameStd = databaseName.toStdString();

    unsigned long numCompoundsToRemove = 0;
    for (auto x : compoundsDB) {
        if (x->db == databaseName.toStdString()) {
            numCompoundsToRemove++;
        }
    }

    vector<Compound*> compoundsToRemove(numCompoundsToRemove);
    vector<Compound*> updatedCompounds(compoundsDB.size()-numCompoundsToRemove);

    unsigned int removeCounter = 0;
    unsigned int updatedCounter = 0;

    for (auto it = compoundIdMap.begin(); it != compoundIdMap.end();) {
        Compound *compound = it.value();
        if (compound->db == databaseNameStd) {
            compoundsToRemove[removeCounter] = compound;
            it = compoundIdMap.erase(it);
            removeCounter++;
        } else {
            updatedCompounds[updatedCounter] = compound;
            updatedCounter++;
            it++;
        }
    }

    qDebug() << "Database::unloadCompounds(): initial=" << compoundsDB.size()
             << "removing" << removeCounter
             << "retaining" << updatedCounter
             << "(removed+retained)" << (removeCounter+updatedCounter);

    compoundsDB = updatedCompounds;

    //re-sort for matching
    sort(compoundsDB.begin(), compoundsDB.end(), Compound::compMass);

    delete_all(compoundsToRemove);
}

void Database::unloadAllCompounds() {
    compoundIdMap.clear();
    loadedDatabase.clear();
    delete_all(compoundsDB);
}

void Database::loadCompoundsSQL(QString databaseName, QSqlDatabase &dbConnection) {

        QElapsedTimer *timer = new QElapsedTimer();
        timer->start();

        if (loadedDatabase.count(databaseName)) {
            qDebug()  << databaseName << "already loaded";
            return;
        }

        QSqlQuery tableHasCompounds(dbConnection);
        if (!tableHasCompounds.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='compounds';")) {
            qDebug() << "Database::loadCompoundsSQL() sql error in query tableHasCompounds: " << tableHasCompounds.lastError();
            qDebug() << "Unable to determine if database has compounds table. exiting program.";
            abort();
        }

        bool isHasCompoundsTable = false;
        while (tableHasCompounds.next()) {
            isHasCompoundsTable = true;
        }

        //Issue 172: Automatically add compounds table if it does not already exist.
        if (!isHasCompoundsTable) {
            vector<Compound*> emptyCompoundVector(0);
            saveCompoundsSQL(emptyCompoundVector, dbConnection);
        }


        bool isHasAdductStringColumn = false;
        bool isHasFragLabelsColumn = false;

        QSqlQuery tableInfoQuery(dbConnection);
        if(!tableInfoQuery.exec("PRAGMA table_info(compounds)")){
            qDebug() << "Database::loadCompoundsSQL() sql error in query tableInfoQuery: " << tableInfoQuery.lastError();
            qDebug() << "Unable to determine information for compounds table. exiting program.";
            abort();
        }

        while(tableInfoQuery.next()) {
            if (tableInfoQuery.value(1).toString() == "adductString") {
                isHasAdductStringColumn = true;
            }

            //Issue 159: Read and write fragment labels from msp databases.
            if (tableInfoQuery.value(1).toString() == "fragment_labels") {
                isHasFragLabelsColumn = true;
            }
        }

        if (!isHasFragLabelsColumn) {
            QSqlQuery queryAddFragLabelsColumn(dbConnection);
            if (!queryAddFragLabelsColumn.exec("ALTER TABLE compounds ADD fragment_labels TEXT;")){
                qDebug() << "Database::loadCompoundsSQL() sql error in query queryAddFragLabelsColumn: " << queryAddFragLabelsColumn.lastError();
                qDebug() << "Unable to adjust compounds database with fragment annotation column! exiting program.";
                abort();
            }
        }

        QSqlQuery countCompounds(dbConnection);
        QString countSql = "select compoundId, dbName from compounds";
        if (databaseName != "ALL")  countSql += " where dbName='" + databaseName + "'";

        if (!countCompounds.exec(countSql)) {
            qDebug() << "Database::loadCompoundsSQL() sql error in query tableInfoQuery: " << countCompounds.lastError();
            qDebug() << "Unable to count compounds in compounds table. exiting program.";
            abort();
        }

        unsigned long numCompoundsToAdd = 0;
        while (countCompounds.next()) {
            string id = countCompounds.value("compoundId").toString().toStdString();
            string db = countCompounds.value("dbName").toString().toStdString();
            if (!compoundIdMap.contains(id + db) ){
                numCompoundsToAdd++;
            }
        }

        vector<Compound*> addedCompounds(numCompoundsToAdd);

        QSqlQuery query(dbConnection);
        QString sql = "select * from compounds";
        if (databaseName != "ALL")  sql += " where dbName='" + databaseName + "'";

        query.prepare(sql);
        if(!query.exec()) qDebug() << "Database::loadCompoundsSQL(): query error in sql: " << query.lastError();

        MassCalculator mcalc;

        unsigned long addedCompoundCounter = 0;

        while (query.next()) {
            string id =   query.value("compoundId").toString().toStdString();
            string name = query.value("name").toString().toStdString();
            string formula = query.value("formula").toString().toStdString();
            int charge = query.value("charge").toInt();
            float exactMass = query.value("mass").toDouble();
            int cid  =  query.value("cid").toInt();
            string db   =  query.value("dbName").toString().toStdString();
            double expectedRt =  query.value("expectedRt").toDouble();

            //skip compound if it already exists in internal database
            if ( compoundIdMap.contains(id + db) ) continue;

            if (!formula.empty()) {
                exactMass = mcalc.computeNeutralMass(formula);
            }

            Compound* compound = new Compound(id,name,formula,charge,exactMass);
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

            compound->precursorMz =  query.value("precursorMz").toDouble();
            compound->productMz =  query.value("productMz").toDouble();
            compound->collisionEnergy =  query.value("collisionEnergy").toDouble();
            compound->smileString=  query.value("smileString").toString().toStdString();

            //mark compound as decoy if names contains DECOY string
            if(compound->name.find("DECOY") > 0) compound->isDecoy=true;

            for(QString f: query.value("category").toString().split(";") ) {
                compound->category.push_back(f.toStdString());
            }

            QStringList fragMzsList = query.value("fragment_mzs").toString().split(";");
            compound->fragment_mzs.resize(fragMzsList.size());
            for (unsigned int i = 0; i < compound->fragment_mzs.size(); i++) {
                compound->fragment_mzs[i] = fragMzsList[i].toDouble();
            }

            QStringList fragIntensityList = query.value("fragment_intensity").toString().split(";");
            compound->fragment_intensity.resize(fragIntensityList.size());
            for (unsigned int i = 0; i < compound->fragment_intensity.size(); i++) {
                compound->fragment_intensity[i] = fragIntensityList[i].toDouble();
            }

            if (!query.value("fragment_labels").isNull()) {

                QStringList fragLabelsList = query.value("fragment_labels").toString().split(";");
                compound->fragment_labels.resize(fragLabelsList.size());
                for (unsigned int i = 0; i < compound->fragment_labels.size(); i++) {
                    compound->fragment_labels[i] = fragLabelsList[i].toStdString();
                }

            } else {
                compound->fragment_labels = vector<string>(compound->fragment_mzs.size());
            }

            if (compound->fragment_mzs.size() != compound->fragment_intensity.size()) {
                qDebug() << "COMPOUND: " << compound->name.c_str() << ": #mzs=" << compound->fragment_mzs.size() << "; #ints=" << compound->fragment_intensity.size() << "Has a different number of mz and intensity elements.";
                qDebug() << "This is not permitted. Please fix this entry in the library file and reload the corrected library.";
                qDebug() << "Exiting program.";
                abort();
            }

            //sort all compounds by m/z (and re-order corresponding intensity vector and labels vector)
            vector<pair<float,int>> pairsArray = vector<pair<float,int>>(compound->fragment_mzs.size());
            for (unsigned int pos = 0; pos < compound->fragment_mzs.size(); pos++){
                pairsArray[pos] = make_pair(compound->fragment_mzs[pos], pos);
            }

            sort(pairsArray.begin(), pairsArray.end());

            vector<float> sortedMzs = vector<float>(pairsArray.size());
            vector<float> sortedIntensities = vector<float>(pairsArray.size());
            vector<string> sortedLabels = vector<string>(pairsArray.size());

            for (unsigned int pos = 0; pos < pairsArray.size(); pos++) {
                sortedMzs[pos] = pairsArray[pos].first;
                sortedIntensities[pos] = compound->fragment_intensity[pairsArray[pos].second];
                sortedLabels[pos] = compound->fragment_labels[pairsArray[pos].second];
            }

            compound->fragment_mzs = sortedMzs;
            compound->fragment_intensity = sortedIntensities;
            compound->fragment_labels = sortedLabels;

            compoundIdMap[compound->id + compound->db]=compound; //20% of time spent here
            addedCompounds[addedCompoundCounter] = compound;
            addedCompoundCounter++;
        }

        if (addedCompoundCounter > 0 && databaseName != "ALL") {
            loadedDatabase[databaseName] = static_cast<int>(addedCompoundCounter);
        }

        //Issue 184: Duplicate entries in a single DB can lead to mismatch in counts (numCompoundsToAdd can be too high)
        if (addedCompoundCounter < numCompoundsToAdd) {
            addedCompounds.resize(addedCompoundCounter);
        }

        if (databaseName != "summarized") qDebug() << "Database::loadCompoundSQL() Finished reading in data for" << databaseName;

        compoundsDB.reserve(compoundsDB.size() + addedCompounds.size());
        compoundsDB.insert(compoundsDB.end(), addedCompounds.begin(), addedCompounds.end());
        sort(compoundsDB.begin(),compoundsDB.end(), Compound::compMass);

        if (databaseName != "summarized") {
            qDebug() << "Database::loadCompoundSQL(): Finished appending data for"
                     << databaseName
                     << "to compoundDB"
                     << "in" << timer->elapsed() << "msec. size="
                     << compoundsDB.size();
        }

        bool isUpdatedAdducts = false;

        //Issue 462: add adducts
        for (QString adductName : getAdductNames(databaseName)) {

            bool isAlreadyLoaded = false;
            for (auto adduct : adductsDB) {
                if (adduct->name == adductName.toStdString()){
                    isAlreadyLoaded = true;
                    break;
                }
            }

            if (!isAlreadyLoaded) {
                for (auto adduct : availableAdducts) {
                    if (adduct->name == adductName.toStdString()) {
                        adductsDB.push_back(adduct);
                        isUpdatedAdducts = true;
                        break;
                    }
                }
            }

        }

        delete(timer);
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

Compound* Database::findSpeciesById(string id, string db, bool attemptToLoadDB) {

    if (attemptToLoadDB && !loadedDatabase.count(db.c_str())) loadCompoundsSQL(db.c_str(),ligandDB);

    //cerr << "searching for " << id << " " << compoundIdMap.size() << " " << db << endl;
    if ( compoundIdMap.contains(id + db) ) return compoundIdMap[id + db];
    if ( compoundIdMap.contains(id) ) return compoundIdMap[id];

    return nullptr;

    /*Compound* c = NULL;
    for(int i=0; i < compoundsDB.size(); i++ ) {
        if (compoundsDB[i]->id == id ) { c = compoundsDB[i]; break; }
    }
    return c;
    */
}


vector<Compound*> Database::findSpeciesByName(string name, string dbname, bool attemptToLoadDB) {
        if (attemptToLoadDB && !loadedDatabase.count(dbname.c_str()))  loadCompoundsSQL(dbname.c_str(),ligandDB);

        vector<Compound*> set;
        qDebug() << "Database::findSpeciesByName()" << name.c_str();
		for(unsigned int i=0; i < compoundsDB.size(); i++ ) {
				if (compoundsDB[i]->name == name && compoundsDB[i]->db == dbname) {
					set.push_back(compoundsDB[i]);
				}
		}
		return set;
}

/**
 * @brief Database::findSpeciesByNameAndAdduct
 * @param compoundName
 * @param adductName
 * @param dbname
 * @return
 *
 * @deprecated
 * no usages
 */
Compound* Database::findSpeciesByNameAndAdduct(string compoundName, string adductName, string dbname){

    if (!loadedDatabase.count(dbname.c_str()))  loadCompoundsSQL(dbname.c_str(),ligandDB);

    qDebug() << "Database::findSpeciesByNameAndAdduct()" << compoundName.c_str();
    for(unsigned int i=0; i < compoundsDB.size(); i++ ) {
            if (compoundsDB[i]->name == compoundName && compoundsDB[i]->db == dbname && compoundsDB[i]->adductString == adductName) {
                return compoundsDB[i];
            }
    }

    return nullptr;
}

Compound* Database::findSpeciesByPrecursor(float precursorMz, float productMz, int polarity,double amuQ1, double amuQ3) {

        Compound* x= nullptr;
        float dist=FLT_MAX;

        for( unsigned int i=0; i < compoundsDB.size(); i++ ) {

                if (compoundsDB[i]->precursorMz <= 0 ) continue;
                if (static_cast<int>(compoundsDB[i]->charge) != polarity) continue;

                float a = abs(compoundsDB[i]->precursorMz - precursorMz);
                if ( a > static_cast<float>(amuQ1)) continue; // Q1 tolerance (precursor ion)

                float b = abs(compoundsDB[i]->productMz - productMz);
                if ( b > static_cast<float>(amuQ3) ) continue; // Q3 tolerance (product ion)

                float d = sqrt(a*a+b*b);

                if ( d < dist) {
                    x = compoundsDB[i];
                    dist=d;
                }
        }

		return x;
}

vector<MassCalculator::Match> Database::findMatchingCompounds(float mz, float ppm, float charge) {
    vector<MassCalculator::Match>uniqset;

    //compute plausable adducts
    for(Adduct* a: adductsDB ) {

            // Issue 495: Treat a zero-charge adduct like a wildcard
            if (charge != 0 && SIGN(a->charge) != SIGN(charge)) continue;

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

vector<MassCalculator::Match> Database::findMatchingCompoundsSLOW(float mz, float ppm, float charge) {
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

vector<Compound*> Database::getCompoundsSubset(string dbname) {
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

//Issue 462
QStringList Database::getAdductNames(QString dbName) {
    QSqlQuery query(ligandDB);

    QString sqlQuery("select distinct adductString from compounds where compounds.dbName IS '");
    sqlQuery.append(dbName);
    sqlQuery.append("'");

    query.prepare(sqlQuery);
    if (!query.exec())  qDebug() << query.lastError();

    QStringList adductNames;
    while(query.next()) adductNames << query.value(0).toString();
    return adductNames;
}

/**
 * @brief Database::loadAdducts
 * @param filename
 * @return
 *
 * @deprecated in favor of Adduct::loadAdducts().
 */
vector<Adduct*> Database::loadAdducts(string filename) {
    qDebug() << "ADDUCTS FILE=" << QString(filename.c_str());

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
    myfile.close();
    return adducts;
}

void Database::loadPeakGroupTags(string filename) {

    ifstream tagsFile(filename.c_str());

    if (! tagsFile.is_open()) return;

    vector<char> usedLabels;
    vector<char> usedHotKeys;

    string line;
    while ( getline(tagsFile, line) ) {
        if (line.empty()) continue;
        if (line[0] == '#') continue; //comments

        vector<string> fields;
        mzUtils::split(line,',', fields);

        if (fields.size() < 5) continue; //skip lines with missing information

        string name = fields[0];

        //Use lower case characters only to fit with previous usage
        QString labelStr = QString(fields[1].c_str()).toLower();
        QString hotKeyStr = QString(fields[2].c_str()).toLower();

        char label = labelStr.toStdString().c_str()[0];
        char hotkey = hotKeyStr.toStdString().c_str()[0];

        string icon = fields[3];

        QString descriptionQ;
        for (unsigned int i = 4; i < fields.size(); i++) {
            if (i > 4) {
                descriptionQ.append(",");
            }
            descriptionQ.append(fields[i].c_str());
        }

        string description = descriptionQ.toStdString();
        description.erase(remove(description.begin(), description.end(),'\"'), description.end());

        if (
                hotkey == 'o' || //open file
                hotkey == 't' || //train model
                hotkey == 'g' || //mark group good
                hotkey == 'b' || //mark group bad
                hotkey == 'q' || //quit application
                hotkey == 'p' || //print
                hotkey == 'a' || //select all
                hotkey == 'c' || //copy
                hotkey == 's' || //save
                hotkey == 'd' || //add bookmark
                hotkey == 'f' || //find
                hotkey == 'h' || //hide window
                hotkey == 'k' || //yield focus
                hotkey == 'u'    //unmark group
                )
        {
            qDebug() << "PeakGroupTag with hotkey ==" << hotkey << "could not be used: reserved hotkey.";
            continue;
        }

        if (
                label == PeakGroup::ReservedLabel::GOOD ||
                label == PeakGroup::ReservedLabel::BAD ||
                label == PeakGroup::ReservedLabel::DELETED ||
                label == PeakGroup::ReservedLabel::COMPOUND_MANUALLY_CHANGED
                )
        {
            qDebug() << "PeakGroupTag with label ==" << label << "could not be used: reserved label.";
            continue;
        }

        for (auto &x : usedLabels) {
            if (label == x) {
                qDebug() << "PeakGroupTag encoded as: " << line.c_str() << "could not be used: label already exists.";
                continue;
            }
        }

        for (auto &x : usedHotKeys) {
            if (hotkey == x) {
                qDebug() << "PeakGroupTag encoded as: " << line.c_str() << " could not be used: hotkey already exists.";
                continue;
            }
        }

        PeakGroupTag *tag = new PeakGroupTag(name, label, hotkey, icon, description);

        usedLabels.push_back(label);
        usedHotKeys.push_back(hotkey);

        peakGroupTags.insert(make_pair(label, tag));
    }
    tagsFile.close();

    qDebug() << "Database::loadPeakGroupTags():" << peakGroupTags.size() << "tags.";
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
                fields[i]= fields[i].replace("\'","");
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

        string id, name, formula, smile, adductName;
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

        if ( header.count("adduct")) adductName = fields[ header["adduct"] ].toStdString();

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
            compound->adductString = adductName;
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

   unordered_map<string, int> idCounts{};

    do {
        line = stream.readLine();

        //Issue 235: debugging
        //qDebug() << line;

        if(line.startsWith("Name:",Qt::CaseInsensitive)){

            if(cpd and !cpd->name.empty()) {
                //cerr << "NIST LIBRARY:" << cpd->db << " " << cpd->name << " " << cpd->formula << " " << cpd->id << endl;
                if(!cpd->formula.empty())  cpd->setExactMass( mcalc.computeMass(cpd->formula,0));

                string id = cpd->id;

                //Issue 235: Ensure that no duplicate IDs are included in the same library.
                //If an ID is duplicated, add a serial number for each duplicate.
                if (idCounts.find(id) != idCounts.end()) {
                    idCounts[id]++;
                    cpd->id = id + "_" + to_string(idCounts[id]);
                } else {
                    idCounts.insert(make_pair(id, 1));
                }

                compoundSet.push_back(cpd);
            }

            //NEW COMPOUND
            QString name = line.mid(5,line.length()).simplified();

            //Issue 235: by default, ID is the name. this can be overwritten later by an explicit "ID:" field in the msp file.
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
            if(!smileString.isEmpty() && smileString.trimmed() != "NA") cpd->smileString=smileString.toStdString();
         } else if (line.startsWith("PRECURSORMZ:",Qt::CaseInsensitive)) {
            cpd->precursorMz = line.mid(13,line.length()).simplified().toDouble();
         } else if (line.startsWith("EXACTMASS:",Qt::CaseInsensitive)) {
            float exactMass = line.mid(10,line.length()).simplified().toDouble();
            cpd->setExactMass(exactMass);
        } else if (line.startsWith("HMDB:", Qt::CaseInsensitive)) {
           string hmdb = line.mid(6, line.length()).simplified().toStdString();
           cpd->metaDataMap.insert(make_pair(CompoundUtils::getIdentifierKey(IdentifierType::HMDB), hmdb));
           cpd->category.push_back(hmdb);
        } else if (line.startsWith("CLASS:", Qt::CaseInsensitive)) {
            string lipidClass = line.mid(7, line.length()).simplified().toStdString();
            cpd->metaDataMap.insert(make_pair(LipidSummarizationUtils::getLipidClassSummaryKey(),
                                              lipidClass));
         } else if (line.startsWith("SUMCOMPOSITION:", Qt::CaseInsensitive)) {
            string sumComposition = line.mid(16, line.length()).simplified().toStdString();
            cpd->metaDataMap.insert(make_pair(LipidSummarizationUtils::getAcylChainCompositionSummaryAttributeKey(),
                                              sumComposition));
         } else if (line.startsWith("SUMCHAINLENGTHS:", Qt::CaseInsensitive)) {
            string chainLengthSummary = line.mid(17, line.length()).simplified().toStdString();
            cpd->metaDataMap.insert(make_pair(LipidSummarizationUtils::getAcylChainLengthSummaryAttributeKey(),
                                              chainLengthSummary));
         } else if (line.startsWith("ADDUCT:",Qt::CaseInsensitive)) {
            cpd->adductString = line.mid(8,line.length()).simplified().toStdString();
            cpd->setChargeFromAdductName();
         } else if (line.startsWith("PRECURSORTYPE:", Qt::CaseInsensitive)) {
           cpd->adductString = line.mid(15,line.length()).simplified().toStdString();
           cpd->setChargeFromAdductName();
         } else if (line.startsWith("PRECURSOR TYPE:", Qt::CaseInsensitive) || line.startsWith("PRECURSOR_TYPE:", Qt::CaseInsensitive)) {
            cpd->adductString = line.mid(16,line.length()).simplified().toStdString();
            cpd->setChargeFromAdductName();
         } else if (line.startsWith("IONIZATION:", Qt::CaseInsensitive)) {
            QString ionizationString = line.mid(12,line.length()).simplified();

            if (ionizationString.endsWith("+") || ionizationString.endsWith("-")) {
                cpd->adductString = ionizationString.toStdString();
            } else if (ionizationString == "[M-H]") {
                cpd->adductString = "[M-H]-";
                cpd->setChargeFromAdductName();
            } else if (ionizationString == "[M+H]") {
                cpd->adductString = "[M+H]+";
                cpd->setChargeFromAdductName();
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
         } else if (line.startsWith("COMPOUNDCLASS:", Qt::CaseInsensitive)) {
            cpd->category.push_back(line.mid(15,line.length()).simplified().toStdString());
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

                 bool ok=false;
                 bool ook=false;
                 float mz = mzintpair.at(0).toDouble(&ok);
                 float ints = mzintpair.at(1).toDouble(&ook);

                 if (ok && ook && mz >= 0 && ints >= 0) {

                     long sameMz = -1;
                     if (cpd->fragment_mzs.size() > 0) {

                         //Issue 237: do not assume sorted fragment lists
                         for (unsigned int i = 0; i < cpd->fragment_mzs.size(); i++) {
                             if (abs(mz - cpd->fragment_mzs[i]) < 1e-6) {
                                 sameMz = i;
                                 break;
                             }
                         }
                     }

                     //MS3 fragments may have duplicate m/z values, if they have different precursor m/zs.
                     //This check does not explicitly check the ms2 precursor and the ms3 fragment m/z value
                     if (sameMz != -1 && mzintpair.size() >= 3) {
                         string label = mzintpair.at(2).toStdString();
                         if (label.find("ms3-{", 0) == 0) {
                            sameMz = -1;
                         }
                     }

                     if (sameMz != -1) {

                         cpd->fragment_intensity[sameMz] = 0.5 * (cpd->fragment_intensity[sameMz] + ints);

                         if(mzintpair.size() >= 3) {
                             QString fragLabel;
                             if (!cpd->fragment_labels[sameMz].empty()){
                                 fragLabel.append(cpd->fragment_labels[sameMz].c_str());
                                 fragLabel.append("/");
                             }
                             for (unsigned int k = 2; k < mzintpair.size(); k++) {
                                 if (k > 2) {
                                     fragLabel.append(" ");
                                 }
                                 fragLabel.append(mzintpair[k]);
                             }
                             cpd->fragment_labels[sameMz] = fragLabel.toStdString();
                         }

                     } else {
                         cpd->fragment_intensity.push_back(ints);
                         cpd->fragment_mzs.push_back(mz);
                         int frag_indx = cpd->fragment_mzs.size()-1;

                         if(mzintpair.size() >= 3) {
                             cpd->fragment_iontype[frag_indx] = mzintpair.at(2).toStdString();
                             QString fragLabel("");
                             for (unsigned int k = 2; k < mzintpair.size(); k++) {
                                 if (k > 2) {
                                     fragLabel.append(" ");
                                 }
                                 fragLabel.append(mzintpair[k]);
                             }
                             cpd->fragment_labels.push_back(fragLabel.toStdString());
                         } else {
                             cpd->fragment_labels.push_back("");
                         }
                     }
                 }
             }
         }

    } while (!line.isNull());

   if(cpd and !cpd->name.empty()) {
       //cerr << "NIST LIBRARY:" << cpd->db << " " << cpd->name << " " << cpd->formula << " " << cpd->id << endl;
       if(!cpd->formula.empty())  cpd->setExactMass( mcalc.computeMass(cpd->formula,0));

       string id = cpd->id;

       //Issue 235: Ensure that no duplicate IDs are included in the same library.
       //If an ID is duplicated, add a serial number for each duplicate.
       if (idCounts.find(id) != idCounts.end()) {
           idCounts[id]++;
           cpd->id = id + "_" + to_string(idCounts[id]);
       } else {
           idCounts.insert(make_pair(id, 1));
       }

       compoundSet.push_back(cpd);
   }

    //Issue 551: Expand error message
    bool isHasUnequalMatches = false;
    for (Compound *cpd : compoundSet) {
        if (cpd->fragment_mzs.size() != cpd->fragment_intensity.size()){
            isHasUnequalMatches = true;
            qDebug() << "COMPOUND:"
                     << cpd->name.c_str()
                     << cpd->adductString.c_str()
                     << ":#mzs=" << cpd->fragment_mzs.size()
                     << "; #ints=" << cpd->fragment_intensity.size();

            qDebug() << "FRAGMENTS:" << endl;
            for (auto mz : cpd->fragment_mzs) {
                qDebug() << mz;
            }
            qDebug() << endl;
            qDebug() << "INTENSITIES:";
            for (auto i : cpd->fragment_intensity) {
                qDebug() << i;
            }
            qDebug() << endl;
        }
    }

    qDebug() << "COMPOUND LIBRARY HAS ANY COMPOUNDS WITH MISMATCHED (m/z, intensity) PAIRS? " << (isHasUnequalMatches ? "yes" : "no") << endl;
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


void Database::saveCompoundsSQL(vector<Compound*> &compoundSet, QSqlDatabase& dbConnection) {
    qDebug() << "Database::saveCompoundsSQL(): Saving"
             << compoundSet.size()
             << "Compounds";

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
    bool isHasFragLabelsColumn = false;

    while (queryCheckCols.next()){
        isHasExistingCols = true;

        if ("adductString" == queryCheckCols.value(1).toString()){
            isHasAdductString = true;
        }

        //Issue 159: Read and write fragment labels from msp databases.
        if ("fragment_labels" == queryCheckCols.value(1).toString()) {
            isHasFragLabelsColumn = true;
        }

    }

    if (isHasExistingCols) {
        if (!isHasAdductString) {
            QSqlQuery queryAddAdductsStringColumn(dbConnection);
            if (!queryAddAdductsStringColumn.exec("ALTER TABLE compounds ADD adductString varchar(255);")){
                qDebug() << "Database::saveCompoundsSQL() sql error in query queryAddFragLabelsColumn: " << queryAddAdductsStringColumn.lastError();
                qDebug() << "Unable to adjust compounds database with adduct string column! exiting program.";
                abort();
            }
        }

        if (!isHasFragLabelsColumn) {
            QSqlQuery queryAddFragLabelsColumn(dbConnection);
            if (!queryAddFragLabelsColumn.exec("ALTER TABLE compounds ADD fragment_labels TEXT;")){
                qDebug() << "Database::saveCompoundsSQL() sql error in query queryAddFragLabelsColumn: " << queryAddFragLabelsColumn.lastError();
                qDebug() << "Unable to adjust compounds database with fragment annotation column! exiting program.";
                abort();
            }
        }
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
                    fragment_intensity text, \
                    fragment_labels text \
                    )"))  qDebug() << "Ho... " << query0.lastError();

        query0.exec("create index if not exists compound_db_idx on compounds(dbName)");

        for(Compound* c : compoundSet) {
            QStringList cat;
            QStringList fragMz;
            QStringList fragIntensity;
            QStringList fragLabels;

            for(string s : c->category) { cat << s.c_str(); }

            for (unsigned int i = 0; i < c->fragment_mzs.size(); i++) {
                float mz = c->fragment_mzs[i];
                float intensity = c->fragment_intensity[i];

                QString mzQString = QString::number(mz, 'f', 5);
                QString intensityQString = QString::number(intensity, 'f', 5);
                QString fragLabel = QString(c->fragment_labels[i].c_str());

                if (mzQString != "0.00000" && intensityQString != "0.00000") {
                    fragMz << mzQString;
                    fragIntensity << intensityQString;
                    fragLabels << fragLabel;
                }
            }

	    QVariant cid =  QVariant(QVariant::Int);
        if (c->cid)cid = QVariant(c->cid);

            QSqlQuery query1(dbConnection);

            query1.prepare("replace into compounds values(?,?,?,?,?, ?,?,?,?,?, ?,?,?,?,?, ?,?,?,?,?, ?)");

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
            query1.bindValue( 20, fragLabels.join(";"));

            if(!query1.exec())  qDebug() << query1.lastError();
        }

    query0.exec("end transaction");
}
