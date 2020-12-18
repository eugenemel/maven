#include "csvreports.h"
#include "lipidsummarizationutils.h"

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
                        << "ID"
                        << "goodPeakCount"
                        << "medMz"
                        << "medRt"
                        << "maxQuality"
                        << "note"
                        << "displayName"
                        << "compound"
                        << "compoundId"
                        << "compoundFormula"
                        << "compoundSMILES"
                        << "adductName"
                        << "category"
                        << "database"

                           //START Issue 321
                        << "summarizationLevel"
                        << "summarizationDescription"
                        << "compoundStrucDefSummary"
                        << "compoundSnPositionSummary"
                        << "compoundMolecularSpeciesSummary"
                        << "compoundSpeciesSummary"
                        << "compoundLipidClass"
                           //END Issue 321

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
                        << "mzFragError"
                        << "ms2purity";

    for(unsigned int i=0; i< samples.size(); i++) {
        string sample = samples[i]->sampleName + " (intensity)";
        Header << sample.c_str();
    }

    for(unsigned int i=0; i< samples.size(); i++) {
        string sample = samples[i]->sampleName + " (RT width)";
        Header << sample.c_str();
    }

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
        cerr << "CSVReports::writeGroupInfo(): group Report is closed" << endl;
        return;
    }

    //get ionization mode
	/* this is not used for now
    int ionizationMode; 
    if( samples.size()>0 && samples[0]->getPolarity()>0 ) ionizationMode=+1;
    else ionizationMode = -1;
	*/

    vector<float> yvalues = group->getOrderedIntensityVector(samples,qtype);

    map<mzSample*, Peak> sampleToPeak = {};
    for (auto p : group->peaks) {
        sampleToPeak.insert(make_pair(p.getSample(), p));
    }

    vector<float> peakWidths(samples.size(), 0.0);

    for (unsigned int i = 0; i < samples.size(); i++) {
        mzSample *sample = samples.at(i);
        if (sampleToPeak.find(sample) != sampleToPeak.end()) {
            peakWidths.at(i) = sampleToPeak.at(sample).rtmax - sampleToPeak.at(sample).rtmin;
        }
    }

    string idString = TableDockWidget::groupTagString(group).toStdString();

    groupReport << group->getPeakGroupLabel() << SEP << setprecision(7)
                << group->metaGroupId << SEP
                << group->groupId << SEP
                << doubleQuoteString(idString) << SEP //header is titled 'ID'
                << group->goodPeakCount << SEP
                << group->meanMz << SEP
                << group->meanRt << SEP
                << group->maxQuality << SEP
                << doubleQuoteString(group->tagString) << SEP //header is titled 'note'
                << doubleQuoteString(group->displayName);

    string compoundName;
    string compoundCategory;
    string compoundID;
    string database;
    string compoundMolecularFormula;
    string compoundSMILES;
    float  expectedRtDiff=1000;
    float  ppmDist=1000;

    if ( group->compound) {
        Compound* c = group->compound;
        ppmDist = group->fragMatchScore.ppmError;
        expectedRtDiff = c->expectedRt-group->meanRt;
        compoundName = c->name;
        compoundID =  c->id;
        database = c->db;
        compoundMolecularFormula = c->formula;
        compoundSMILES = c->smileString;
        if (c->category.size()) compoundCategory = c->category[0];
    }

    //Issue 142: imported compound name always takes precedence
    if (!group->importedCompoundName.empty()){
        compoundName = group->importedCompoundName;
    }

    string adductName;
    if (group -> adduct) {
        adductName = group->adduct->name;
    }

    groupReport << SEP << doubleQuoteString(compoundName);
    groupReport << SEP << doubleQuoteString(compoundID);
    groupReport << SEP << doubleQuoteString(compoundMolecularFormula);
    groupReport << SEP << doubleQuoteString(compoundSMILES);
    groupReport << SEP << doubleQuoteString(adductName);
    groupReport << SEP << compoundCategory;
    groupReport << SEP << database;

    //Issue 321: summarization information, if appropriate
    int summarizationLevel = 4; //default to level 4

    string strucDefSummarized;
    string snChainSummarized;
    string acylChainLengthSummarized;
    string acylChainCompositionSummarized;
    string lipidClassSummarized;

    if (!compoundName.empty()){

        LipidNameComponents lipidNameComponents = LipidSummarizationUtils::getNameComponents(compoundName);

        strucDefSummarized = LipidSummarizationUtils::getStrucDefSummary(compoundName);
        snChainSummarized = LipidSummarizationUtils::getSnPositionSummary(compoundName);
        acylChainLengthSummarized = LipidSummarizationUtils::getAcylChainLengthSummary(compoundName);
        acylChainCompositionSummarized = LipidSummarizationUtils::getAcylChainCompositionSummary(compoundName);
        lipidClassSummarized = LipidSummarizationUtils::getLipidClassSummary(compoundName);

        if (idString == compoundName) {
            summarizationLevel = lipidNameComponents.initialLevel;
        }

        if (idString == strucDefSummarized && strucDefSummarized != snChainSummarized) {
            summarizationLevel = 5;
        } else if (idString == snChainSummarized && snChainSummarized != acylChainLengthSummarized) {
            summarizationLevel = 4;
        } else if (idString == acylChainLengthSummarized && acylChainLengthSummarized != strucDefSummarized) {
            summarizationLevel = 3;
        } else if (idString == acylChainCompositionSummarized && acylChainCompositionSummarized != acylChainLengthSummarized) {
            summarizationLevel = 2;
        } else if (idString == lipidClassSummarized && lipidClassSummarized != acylChainCompositionSummarized) {
            summarizationLevel = 1;
        }

    }

    groupReport << SEP << to_string(summarizationLevel);
    groupReport << SEP << LipidSummarizationUtils::getSummarizationLevelAttributeKey(summarizationLevel);
    groupReport << SEP << doubleQuoteString(strucDefSummarized);
    groupReport << SEP << doubleQuoteString(snChainSummarized);
    groupReport << SEP << doubleQuoteString(acylChainLengthSummarized);
    groupReport << SEP << doubleQuoteString(acylChainCompositionSummarized);
    groupReport << SEP << doubleQuoteString(lipidClassSummarized);

    groupReport << SEP << expectedRtDiff;
    groupReport << SEP << ppmDist;

    if ( group->parent) {
        groupReport << SEP << group->parent->meanMz;
    } else {
        groupReport << SEP << group->meanMz;
    }

    groupReport << SEP << group->ms2EventCount;
    ///
    ///
    groupReport << SEP << group->fragMatchScore.numMatches;
    groupReport << SEP << group->fragMatchScore.fractionMatched;
    groupReport << SEP << group->fragMatchScore.ticMatched;
    groupReport << SEP << group->fragMatchScore.dotProduct;
    groupReport << SEP << group->fragMatchScore.weightedDotProduct;
    groupReport << SEP << group->fragMatchScore.hypergeomScore;
    groupReport << SEP << group->fragMatchScore.spearmanRankCorrelation;
    groupReport << SEP << group->fragMatchScore.mzFragError;
    groupReport << SEP << group->fragmentationPattern.purity;

    for( unsigned int j=0; j < samples.size(); j++) groupReport << SEP <<  yvalues[j];
    for( unsigned int j=0; j < samples.size(); j++) groupReport << SEP <<  peakWidths[j];

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

