#include "csvreports.h"

CSVReports::CSVReports(vector<mzSample*>&insamples) {
    samples = insamples;
    groupId = 0;
    setUserQuantType(PeakGroup::AreaTop);
    setTabDelimited();
    sort(samples.begin(), samples.end(), mzSample::compSampleOrder);
}


CSVReports::~CSVReports() { 
    closeFiles();
}

void CSVReports::openGroupReport(string outputfile) {
    if (samples.size()==0) return;

    groupReport.open(outputfile.c_str());
    if(! groupReport.is_open()) return;

    QStringList Header;
    Header << "label"<< "metaGroupId"
                        << "groupId"
                        << "goodPeakCount"
                        << "medMz"
                        << "medRt"
                        << "maxQuality"
                        << "note"
                        << "compound"
                        << "compoundId"
                        << "category"
                        << "database"
                        << "expectedRtDiff"
                        << "ppmDiff"
                        << "parent"
                        << "ms2EventCount"
                        << "fragNumIonsMatched"
                        << "fragFracMatched"
                        << "ticMatched"
                        << "dotProduct"
                        << "weigtedDotProduct"
                        << "hyperGeomScore"
                        << "spearmanRankCorrelation"
                        << "mzFragError";

    for(unsigned int i=0; i< samples.size(); i++) { Header << samples[i]->sampleName.c_str(); }

    foreach(QString h, Header)  groupReport << h.toStdString() << SEP;
    groupReport << endl;
}

void CSVReports::openPeakReport(string outputfile) {
    if (samples.size()==0) return;

    if(QString(outputfile.c_str()).endsWith(".csv",Qt::CaseInsensitive) ) setCommaDelimited();

    QStringList colnames;
    colnames    <<"groupId" <<"compound"<<"compoundId"<<"sample"<<"peakMz"
                <<"medianMz"<<"baseMz"<<"rt"<<"rtmin"<<"rtmax"<<"quality"
                <<"peakIntensity"<<"peakArea"<<"peakAreaTop"<<"peakAreaCorrected"
                <<"noNoiseObs"<<"signalBaseLineRatio"<<"fromBlankSample";


    peakReport.open(outputfile.c_str());
    if (peakReport.is_open()) {
        QString header = colnames.join(SEP.c_str());
        peakReport << header.toStdString() << endl;
    }
}

//Feng note: CSVReports::addGroup modified to (1) output only C12 data without labeling or wen compound is unknown, (2) output all related isotopic forms with labeling, 
//even when peak height is zero
void CSVReports::writeGroupInfo(PeakGroup* group) {
    if(! groupReport.is_open()) {
        cerr << "writeGroupInfo: group Report is closed";
        return;
    }

    //get ionization mode
    int ionizationMode; 
    if( samples.size()>0 && samples[0]->getPolarity()>0 ) ionizationMode=+1;
    else ionizationMode = -1;

    vector<float> yvalues = group->getOrderedIntensityVector(samples,qtype);

    char label[2];
    sprintf(label,"%c",group->label);

    groupReport << label << SEP
                << setprecision(7)
                << group->metaGroupId << SEP
                << group->groupId << SEP
                << group->goodPeakCount << SEP
                << group->meanMz << SEP
                << group->meanRt << SEP
                << group->maxQuality << SEP
                << group->tagString;

    string compoundName;
    string compoundCategory;
    string compoundID;
    string database;
    float  expectedRtDiff=1000;
    float  ppmDist=1000;

    if ( group->compound != NULL) {
        Compound* c = group->compound;
        ppmDist = group->fragMatchScore.ppmError;
        expectedRtDiff = c->expectedRt-group->meanRt;
        compoundName = c->name;
        compoundID =  c->id;
        database = c->db;
        if (c->category.size()) compoundCategory = c->category[0];
    }

    groupReport << SEP << doubleQuoteString(compoundName);
    groupReport << SEP << doubleQuoteString(compoundID);
    groupReport << SEP << compoundCategory;
    groupReport << SEP << database;
    groupReport << SEP << expectedRtDiff;
    groupReport << SEP << ppmDist;

    if ( group->parent != NULL ) {
        groupReport << SEP << group->parent->meanMz;
    } else {
        groupReport << SEP << group->meanMz;
    }

    groupReport << SEP << group->ms2EventCount;
    groupReport << SEP << group->fragMatchScore.numMatches;
    groupReport << SEP << group->fragMatchScore.fractionMatched;
    groupReport << SEP << group->fragMatchScore.ticMatched;
    groupReport << SEP << group->fragMatchScore.dotProduct;
    groupReport << SEP << group->fragMatchScore.weightedDotProduct;
    groupReport << SEP << group->fragMatchScore.hypergeomScore;
    groupReport << SEP << group->fragMatchScore.spearmanRankCorrelation;
    groupReport << SEP << group->fragMatchScore.mzFragError;

    for( unsigned int j=0; j < samples.size(); j++) groupReport << SEP <<  yvalues[j];
    groupReport << endl;

    for (unsigned int k=0; k < group->children.size(); k++) {
        addGroup(&group->children[k]);
    }
}

string CSVReports::doubleQuoteString(std::string& in) {
    if(in.find('\"') != std::string::npos or in.find(",") != std::string::npos) {
        return "\"" + in + "\"";
    } else{
        return in;
    }
}

void CSVReports::addGroup(PeakGroup* group) { 
    writeGroupInfo(group);
    writePeakInfo(group);
}

void CSVReports::closeFiles() { 
    if(groupReport.is_open()) groupReport.close();
    if(peakReport.is_open() ) peakReport.close();
}

void CSVReports::writePeakInfo(PeakGroup* group) {
    if(! peakReport.is_open()) return;
    string compoundName = "";
    string compoundID = "";

    if (group->compound != NULL) {
        compoundName = group->compound->name;
        compoundID   = group->compound->id;
    }


    for(unsigned int j=0; j<group->peaks.size(); j++ ) {
        Peak& peak = group->peaks[j];
        mzSample* sample = peak.getSample();
        string sampleName;
        if ( sample !=NULL) sampleName= sample->sampleName;

        peakReport << setprecision(8)
                << groupId << SEP
                << doubleQuoteString(compoundName) << SEP
                << doubleQuoteString(compoundID) << SEP
                << sampleName << SEP
                << peak.peakMz <<  SEP
                << peak.medianMz <<  SEP
                << peak.baseMz <<  SEP
                << setprecision(3)
                << peak.rt <<  SEP
                << peak.rtmin <<  SEP
                << peak.rtmax <<  SEP
                << peak.quality << SEP
                << peak.peakIntensity << SEP
                << peak.peakArea <<  SEP
                << peak.peakAreaTop <<  SEP
                << peak.peakAreaCorrected <<  SEP
                << peak.noNoiseObs <<  SEP
                << peak.signalBaselineRatio <<  SEP
                << peak.fromBlankSample << SEP
                << endl;
    }
}

