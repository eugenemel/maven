#include "background_peaks_update.h"

BackgroundPeakUpdate::BackgroundPeakUpdate(QWidget*) { 
    clsf = nullptr;	//initially classifier is not loaded
    mainwindow = nullptr;
    _stopped = true;
    //	setTerminationEnabled(false);

    runFunction = "computeKnowsPeaks";
    alignSamplesFlag=false;
    processMassSlicesFlag=false;
    pullIsotopesFlag=false;
    matchRtFlag=false;
    checkConvergence=false;

    outputdir = "reports" + string(DIR_SEPARATOR_STR);

    writeCSVFlag=false;
    ionizationMode = -1;
    keepFoundGroups=true;
    showProgressFlag=true;

    rtStepSize=20;
    ppmMerge=20;
    avgScanTime=static_cast<float>(0.2); //min

    limitGroupCount=INT_MAX;

    isotopeMzTolr = 20.0f;

    peakGroupCompoundMatchingPolicy=ALL_MATCHES;

    //peak detection
    eic_smoothingWindow= 10;
    eic_smoothingAlgorithm= 0;
    baseline_smoothingWindow=5;
    baseline_dropTopX=40;

    //peak grouping across samples
    grouping_maxRtWindow=0.5;

    //peak filtering criteria
    minGoodPeakCount=1;
    minSignalBlankRatio=2;
    minNoNoiseObs=1;
    minSignalBaseLineRatio=2;
    minGroupIntensity=500;
    minQuality=0.5;

    //Peaks compound matching
    featureCompoundMatchMzTolerance = 20.0f;
    featureCompoundMatchRtTolerance = 2.0f;

    //compound detection setting
    mustHaveMS2=false;
    compoundPPMWindow=10;
    compoundRTWindow=2;
    eicMaxGroups=INT_MAX;
    productPpmTolr=20.0;

    excludeIsotopicPeaks=false;

    //triple quad matching options
    amuQ1=0.25;
    amuQ3=static_cast<float>(0.3);


    //fragmentaiton matching
    searchProton=false;

    //matching options
    isRequireMatchingAdduct = false;
    isRetainUnmatchedCompounds = false;
    isClusterPeakGroups = false;


}

BackgroundPeakUpdate::~BackgroundPeakUpdate() {
	cleanup();
}

void BackgroundPeakUpdate::run(void) {

    if(!mainwindow)   { quit(); return; }
	_stopped = false;

    //Issue 722: flexibility around which samples are included in peak groups
    samples = mainwindow->getVisibleSamples();
    
    // Issue 746: Enforce natural ordering
    QCollator collator;
	collator.setNumericMode(true);
	
	sort(samples.begin(), samples.end(), [collator](mzSample* lhs, mzSample* rhs){
		
		QString leftName = QString(lhs->sampleName.c_str());
		QString rightName = QString(rhs->sampleName.c_str());
		
		return collator.compare(leftName, rightName) < 0;
	});
	

    if (samples.size() > 0 && samples[0]->getPolarity() > 0 ) {
        ionizationMode = +1;
    } else {
        ionizationMode = -1; //set ionization mode for compound matching
    }

	if ( runFunction == "findPeaksQQQ" ) {
		findPeaksQQQ();
	} else if ( runFunction == "processSlices" ) { 
        processSlices();
	} else if ( runFunction == "processMassSlices" ) { 
		processMassSlices();
    } else if  (runFunction == "pullIsotopes" ) {
        IsotopeParameters isotopeParameters = getSearchIsotopeParameters();
        _group->pullIsotopes(isotopeParameters, samples);
        _group->isotopeParameters = isotopeParameters;
    } else if  ( runFunction == "computePeaks" ) { // database search, calibrated dialog
        computePeaks(); // DB Compound Search dialog
	} else {
		qDebug() << "Unknown Function " << runFunction.c_str();
	}

	quit();
	return;
}

void BackgroundPeakUpdate::processSlices() { 
    processSlices(_slices,"sliceset");
}

void BackgroundPeakUpdate::processSlice(mzSlice& slice) { 
	vector<mzSlice*>slices; 
	slices.push_back(&slice);
    processSlices(slices,"sliceset");
}

void BackgroundPeakUpdate::processCompoundSlices(vector<mzSlice*>&slices, string setName){

    //for debuggging
    cerr << "The PeakGroupCompoundMatchingPolicy is set to ";

    if (peakGroupCompoundMatchingPolicy == ALL_MATCHES) {
        cerr << "ALL_MATCHES" << endl;
    } else if (peakGroupCompoundMatchingPolicy == SINGLE_TOP_HIT) {
        cerr << "SINGLE_TOP_HIT" << endl;
    } else if (peakGroupCompoundMatchingPolicy == TOP_SCORE_HITS) {
        cerr << "TOP_SCORE_HITS" << endl;
    }

    if (slices.size() == 0 ) return;

    allgroups.clear();
    sort(slices.begin(), slices.end(), mzSlice::compIntensity );

    QTime timer;
    timer.start();
    qDebug() << "BackgroundPeakUpdate::processCompoundSlices(vector<mzSlice*>&slices, string setName)";
    qDebug() << "Processing slices: setName=" << setName.c_str() << " slices=" << slices.size();

    int eicCount=0;
    int peakCount=0;

    QSettings* settings = mainwindow->getSettings();
    amuQ1 = settings->value("amuQ1").toFloat();
    amuQ3 = settings->value("amuQ3").toFloat();
    string txtEICScanFilter = settings->value("txtEICScanFilter", "").toString().toStdString();

    IsotopeParameters isotopeParameters = getSearchIsotopeParameters();

    unsigned int numPassingPeakGroups = 0;
    unsigned long numAllPeakGroups = 0;
    vector<PeakGroup*> groups; //for alignment only

    //write reports
    CSVReports* csvreports = nullptr;
    if (writeCSVFlag) {
        string groupfilename = outputdir + setName + ".csv";
        csvreports = new CSVReports(samples);
        csvreports->setUserQuantType(mainwindow->getUserQuantType());
        csvreports->openGroupReport(groupfilename);
    }

    for (unsigned int s=0; s < slices.size();  s++ ) {

        mzSlice* slice = slices[s];

        vector<EIC*>eics = pullEICs(slice,samples,
                                    EicLoader::PeakDetection,
                                    static_cast<int>(eic_smoothingWindow),
                                    eic_smoothingAlgorithm,
                                    amuQ1,
                                    amuQ3,
                                    baseline_smoothingWindow,
                                    baseline_dropTopX,
                                    txtEICScanFilter);

        eicCount += eics.size();

        //find peaks
        for(int i=0; i < eics.size(); i++ )  {
            eics[i]->getPeakPositionsD(peakPickingAndGroupingParameters, false);
        }

        vector<PeakGroup> peakgroups = EIC::groupPeaksE(eics, peakPickingAndGroupingParameters);

        numAllPeakGroups += peakgroups.size();

         for (PeakGroup group : peakgroups) {

             group.computeAvgBlankArea(eics);
             //group.groupStatistics();

             Peak* highestpeak = group.getHighestIntensityPeak();
             if(!highestpeak)  continue;

             if (clsf && clsf->hasModel()) {
                 clsf->classify(&group);
                 group.groupStatistics();
             }

//             //Issue 632: Start Debugging
//             qDebug() <<  group.meanMz << "@" << group.medianRt() << ":";
//             if (group.compound) {
//                 qDebug() << group.compound->name.c_str();
//             }

//             qDebug() << "group.goodPeakCount=" << group.goodPeakCount
//                      << " <--> " << minGoodPeakCount
//                      << " ==> " << (group.goodPeakCount < minGoodPeakCount ? "FAIL" : "PASS");

//             qDebug() << "group.blankMax*minSignalBlankRatio=" << (group.blankMax*minSignalBlankRatio)
//                      << " <--> " << minGoodPeakCount
//                      << " ==> " << (group.blankMax*minSignalBlankRatio > group.maxIntensity ? "FAIL" : "PASS");

//             qDebug() << "group.maxNoNoiseObs=" << group.maxNoNoiseObs
//                      << " <--> " << minNoNoiseObs
//                      << " ==> " << (group.maxNoNoiseObs < minNoNoiseObs ? "FAIL" : "PASS");

//             qDebug() << "group.maxSignalBaselineRatio=" << group.maxSignalBaselineRatio
//                      << " <--> minSignalBaseLineRatio=" << minSignalBaseLineRatio
//                      << " ==> " << (group.maxSignalBaselineRatio < minSignalBaseLineRatio ? "FAIL" : "PASS");

//             qDebug() << "group.maxIntensity=" << group.maxIntensity
//                      << " <--> minGroupIntensity=" << minGroupIntensity
//                      << " ==> " << (group.maxIntensity < minGroupIntensity ? "FAIL" : "PASS");

//             qDebug() << endl;
//             //Issue 632: End Debugging

             if (clsf && clsf->hasModel()&& group.goodPeakCount < minGoodPeakCount) continue;
             if (group.blankMax*minSignalBlankRatio > group.maxIntensity) continue;
             if (group.maxNoNoiseObs < minNoNoiseObs) continue;
             if (group.maxSignalBaselineRatio < minSignalBaseLineRatio) continue;
             if (group.maxIntensity < minGroupIntensity ) continue;

             group.chargeState = group.getChargeStateFromMS1(compoundPPMWindow);
             vector<Isotope> isotopes = highestpeak->getScan()->getIsotopicPattern(highestpeak->peakMz,isotopeMzTolr,6,10);

             if(isotopes.size() > 0) {
                 //group.chargeState = isotopes.front().charge;
                 for(Isotope& isotope: isotopes) {
                     if (mzUtils::ppmDist(static_cast<float>(isotope.mz), static_cast<float>(group.meanMz)) < isotopeMzTolr) {
                         group.isotopicIndex=isotope.C13;
                     }
                 }
             }

             if (excludeIsotopicPeaks) {
                  if (group.chargeState > 0 and not group.isMonoisotopic(isotopeMzTolr)) continue;
             }

             group.computeFragPattern(productPpmTolr);

             if (compoundMustHaveMs2 && group.ms2EventCount == 0) continue;

             bool isGroupRetained = false;

             //Scan against all compounds associated with slice
             multimap<double, PeakGroup*> peakGroupCompoundMatches = {};
             double maxScore = -1;

             vector<pair<Compound*, Adduct*>> compoundAdductMatches;

             //TODO: an effort towards unifying Peaks Dialog and Compounds Dialog
             //If slices are not pre-associated with compounds, search for them in the database.
//             if (!slice->compound) {
//                 vector<MassCalculator::Match> compoundMatches = DB.findMatchingCompounds(group.meanMz, compoundPPMWindow, ionizationMode);
//                 for (auto match : compoundMatches) {
//                     compoundAdductMatches.push_back(make_pair(match.compoundLink, match.adductLink));
//                 }
//             } else {
//                 for (auto compound : slice->compoundVector) {
//                     compoundAdductMatches.push_back(make_pair(compound, slice->adduct));
//                 }
//             }


             //TODO: Issue 722
             for (Compound *compound : slice->compoundVector){

                 //MS2 criteria
                 FragmentationMatchScore s = compound->scoreCompoundHit(&group.fragmentationPattern, productPpmTolr, searchProton);
                 s.mergedScore = s.getScoreByName(scoringScheme.toStdString());

                 if (static_cast<float>(s.numMatches) < minNumFragments) continue;
                 if (Fragment::getNumDiagnosticFragmentsMatched("*",compound->fragment_labels, s.ranks) < minNumDiagnosticFragments) continue;
                 if (static_cast<float>(s.mergedScore) < minFragmentMatchScore) continue;

                 //Issue 161: new approach
                 if (isRequireMatchingAdduct && compound->adductString != slice->adduct->name) continue;

                 //Rt criteria and group rank
                 if (matchRtFlag && compound->expectedRt>0) {
                     float rtDiff =  abs(compound->expectedRt - (group.meanRt));
                     group.expectedRtDiff = rtDiff;
                     group.groupRank = rtDiff*rtDiff*(1.1-group.maxQuality)*(1/log(group.maxIntensity+1));
                 } else {
                     group.groupRank = (1.1-group.maxQuality)*(1/log(group.maxIntensity+1));
                 }

                 PeakGroup *peakGroupPtr = new PeakGroup(group);
                 //delete(peakGroupPtr) is called in the slot that receives the emit(newPeakGroup()) call

                 peakGroupPtr->compound = compound;
                 peakGroupPtr->compoundId = compound->id;
                 peakGroupPtr->compoundDb = compound->db;

                 peakGroupPtr->adduct = slice->adduct;
                 peakGroupPtr->tagString = slice->adduct->name;
                 peakGroupPtr->fragMatchScore = s;
                 peakGroupPtr->ms2EventCount = group.ms2EventCount;

                if (s.mergedScore > maxScore){
                    maxScore = s.mergedScore;
                }

                peakGroupCompoundMatches.insert(make_pair(s.mergedScore, peakGroupPtr));
             }

             typedef multimap<double, PeakGroup*>::iterator peakGroupCompoundMatchIterator;

             vector<PeakGroup*> passingPeakGroups;
             vector<PeakGroup*> failingPeakGroups;

             for (peakGroupCompoundMatchIterator it = peakGroupCompoundMatches.begin(); it != peakGroupCompoundMatches.end(); it++){

                 if (peakGroupCompoundMatchingPolicy == ALL_MATCHES){

                     passingPeakGroups.push_back(it->second);

                 } else if (peakGroupCompoundMatchingPolicy == TOP_SCORE_HITS || peakGroupCompoundMatchingPolicy == SINGLE_TOP_HIT) {

                     if (abs(it->first - maxScore) < 1e-6) { //note tolerance of 1e-6
                        passingPeakGroups.push_back(it->second);
                     } else {
                        failingPeakGroups.push_back(it->second);
                     }
                 }
             }

             if (peakGroupCompoundMatchingPolicy == SINGLE_TOP_HIT && passingPeakGroups.size() > 0) {

                 PeakGroup* bestHit = nullptr;

                 for (auto peakGroupPtr : passingPeakGroups){
                     if (!bestHit or peakGroupPtr->compound->name.compare(bestHit->compound->name) < 0){
                         bestHit = peakGroupPtr;
                     }
                 }

                 for (auto peakGroupPtr: passingPeakGroups){
                     if (peakGroupPtr != bestHit){
                         failingPeakGroups.push_back(peakGroupPtr);
                     }
                 }

                 passingPeakGroups.clear();
                 passingPeakGroups.push_back(bestHit);
             }

             //Issue 161
             if (isRetainUnmatchedCompounds && passingPeakGroups.empty()) {
                passingPeakGroups.push_back(new PeakGroup(group));
             }


             numPassingPeakGroups += passingPeakGroups.size();
             delete_all(failingPeakGroups); //passingPeakGroups are deleted on main thread with emit() call.

             for (PeakGroup *peakGroupPtr : passingPeakGroups){

                 //Issue 197: Support isotopic extraction for compound db search.
                if(pullIsotopesFlag && ((excludeIsotopicPeaks && group.isMonoisotopic(isotopeMzTolr)) || !excludeIsotopicPeaks)){
                     peakGroupPtr->pullIsotopes(isotopeParameters,
                                                mainwindow->getSamples(),
                                                false //debug
                                                );

                     auto grpIsotopeParameters = isotopeParameters;
                     grpIsotopeParameters.isotopeParametersType = IsotopeParametersType::SAVED;
                     peakGroupPtr->isotopeParameters = grpIsotopeParameters;
                 }

                 if (csvreports) {
                     csvreports->addGroup(peakGroupPtr);
                 }

                 emit(newPeakGroup(peakGroupPtr, false, true)); // note that 'isDeletePeakGroupPtr' flag is set to true
             }

         } // end peak groups

         delete_all(eics);

         if ( stopped() ) break;

         if (showProgressFlag && s % 10 == 0) {
             QString progressText = "Found " + QString::number(numPassingPeakGroups) + " Compound <--> Peak Group matches.";
             emit(updateProgressBar( progressText , (s+1), std::min(static_cast<int>(slices.size()),limitGroupCount)));
         }

    } //end slices

    //Clustering is not enabled for library search
//    //Issue 161: run clustering
//    if (isClusterPeakGroups){
//        double maxRtDiff = 0.2;
//        double minSampleCorrelation= 0.8;
//        double minPeakShapeCorrelation=0.9;
//        PeakGroup::clusterGroups(allgroups,samples,maxRtDiff,minSampleCorrelation,minPeakShapeCorrelation,compoundPPMWindow);
//    }

    if (csvreports){
        csvreports->closeFiles();
        delete(csvreports);
    }

    emit(updateProgressBar(QString("Done"), 1, 1));

    qDebug() << "processCompoundSlices() Slices=" << slices.size();
    qDebug() << "processCompoundSlices() EICs="   << eicCount;
    qDebug() << "processCompoundSlices() Groups before filtering=" << numAllPeakGroups;
    qDebug() << "processCompoundSlices() Groups="   << groups.size();
    qDebug() << "processCompoundSlices() Peaks="   << peakCount;
    qDebug() << "Compound <--> Peak Group Matches=" << numPassingPeakGroups;
    qDebug() << "BackgroundPeakUpdate:processCompoundSlices() done. " << timer.elapsed() << " sec.";
}

void BackgroundPeakUpdate::processSlices(vector<mzSlice*>&slices, string setName) { 
    if (slices.size() == 0 ) return;
    allgroups.clear();
    sort(slices.begin(), slices.end(), mzSlice::compIntensity );

    QTime timer2;
    timer2.start();
    qDebug() << "Building searchable database.";
    vector<CompoundIon> searchableDatabase = prepareCompoundDatabase(this->compounds);
    qDebug() << "Built searchable database of " << searchableDatabase.size() << " entries in " << timer2.elapsed() << " sec.";

    //process KNOWNS
    QTime timer;
    timer.start();
    qDebug() << "BackgroundPeakUpdate::processSlices(vector<mzSlice*>&slices, string setName)";
    qDebug() << "Processing slices: setName=" << setName.c_str() << " slices=" << slices.size();

    int converged=0;
    int foundGroups=0;

    int eicCount=0;
    long peakCountPreFilter=0;
    long totalGroupPreFilteringCounter=0;
    long totalGroupsToAppendCounter=0;

    QSettings* settings = mainwindow->getSettings();

    amuQ1 = settings->value("amuQ1").toFloat();
    amuQ3 = settings->value("amuQ3").toFloat();

    IsotopeParameters isotopeParameters = getSearchIsotopeParameters();

    for (unsigned int s=0; s < slices.size();  s++ ) {
        mzSlice* slice = slices[s];
        Compound* compound = slice->compound;

       // if (compound)  cerr << "Searching for: " << compound->name << "\trt=" << compound->expectedRt << endl;

        if (compound && compound->hasGroup())
            compound->unlinkGroup();

        if (checkConvergence ) {
            allgroups.size()-foundGroups > 0 ? converged=0 : converged++;
            if ( converged > 1000 ){
                cout << "Convergence condition reached. Exiting main loop." << endl;
                break;	 //exit main loop
            }
            foundGroups = allgroups.size();
        }

        string txtEICScanFilter = settings->value("txtEICScanFilter", "").toString().toStdString();

        vector<EIC*>eics = pullEICs(slice,samples,
                                    EicLoader::PeakDetection,
                                    static_cast<int>(eic_smoothingWindow),
                                    eic_smoothingAlgorithm,
                                    amuQ1,
                                    amuQ3,
                                    baseline_smoothingWindow,
                                    baseline_dropTopX,
                                    txtEICScanFilter);

        float eicMaxIntensity=0;

        for ( unsigned int j=0; j < eics.size(); j++ ) {
            if (eics[j]->maxIntensity > eicMaxIntensity) eicMaxIntensity=eics[j]->maxIntensity;
        }
        if (eicMaxIntensity < minGroupIntensity) { delete_all(eics); continue; }

        eicCount += eics.size();

        //find peaks
        for(unsigned int i=0; i < eics.size(); i++ ) {
            eics[i]->getPeakPositionsD(peakPickingAndGroupingParameters, false);
            peakCountPreFilter += eics[i]->peaks.size();
        }

        //Issue 381
        vector<PeakGroup> peakgroups = EIC::groupPeaksE(eics, peakPickingAndGroupingParameters);

        totalGroupPreFilteringCounter += peakgroups.size();

        //score quality of each group
        vector<PeakGroup*> groupsToAppend;
        for(unsigned int j=0; j < peakgroups.size(); j++ ) {

            PeakGroup& group = peakgroups[j];

            //debugging
            //cout << "group= (" << group.meanMz << ", " << group.meanRt << "), #" << allgroups.size() << "/" << limitGroupCount << endl;

            Peak* highestpeak = group.getHighestIntensityPeak();
            if(!highestpeak)  continue;

            if (clsf && clsf->hasModel()) {
                clsf->classify(&group);
            }

            group.computeAvgBlankArea(eics);
            group.groupStatistics();
            
            if (clsf && clsf->hasModel() && group.goodPeakCount < minGoodPeakCount) continue;
            if (clsf && clsf->hasModel() && group.maxQuality < minQuality) continue;

            // if (group.blankMean*minBlankRatio > group.sampleMean ) continue;
            if (group.maxNoNoiseObs < minNoNoiseObs) continue;
            if (group.maxSignalBaselineRatio < minSignalBaseLineRatio) continue;
            if (group.maxIntensity < minGroupIntensity) continue;

            if (group.blankMax*minSignalBlankRatio > group.maxIntensity) continue;

            group.chargeState = group.getChargeStateFromMS1(ppmMerge);
            if (excludeIsotopicPeaks && group.chargeState > 0 and not group.isMonoisotopic(isotopeMzTolr)) continue;

            //if (getChargeStateFromMS1(&group) < minPrecursorCharge) continue;
                        //build consensus ms2 specta
            //vector<Scan*>ms2events = group.getFragmenationEvents();
            //cerr << "\tFound ms2events" << ms2events.size() << "\n";

            group.computeFragPattern(productPpmTolr);
            // BROKEN group.findHighestPurityMS2Pattern(compoundPPMWindow);

            //Issue 746: MS2 required for peak group
            if (mustHaveMS2 and group.ms2EventCount == 0) continue;

            //Issue 727: unified interface for spectral matching
            assignToGroup(&group, searchableDatabase);

            if (!isRetainUnmatchedCompounds && !group.compound){
                continue;
            }

            //Issue 707: extract isotopes
            //Issue 720: diff abundance isotope search implies that isotopes need to be determined
            if (pullIsotopesFlag || isDiffAbundanceIsotopeSearch) {

                //Issue 707: extract isotopic peaks (requires compound by this point)
                group.pullIsotopes(isotopeParameters,
                                   samples,
                                   false //debug
                                   );
                group.isotopeParameters = isotopeParameters;

            }

            //Issue 720: Differential isotopes search
            if (isDiffAbundanceIsotopeSearch) {

                group.scoreIsotopesDifferentialAbundance(
                    isotopeParameters,
                    unlabeledSamples,
                    labeledSamples,
                    false); // debug

            }

            if (!slice->srmId.empty()) group.srmId = slice->srmId;

            groupsToAppend.push_back(&group);
        }

        std::sort(groupsToAppend.begin(), groupsToAppend.end(),PeakGroup::compRankPtr);
        totalGroupsToAppendCounter += groupsToAppend.size();

        for(int j=0; j<groupsToAppend.size(); j++) {
            //check for duplicates  and append group
            if(j >= eicMaxGroups) break;

            PeakGroup* group = groupsToAppend[j];
            bool ok = addPeakGroup(*group);

            //force insert when processing compounds.. even if duplicated
            if (ok == false && compound){
                allgroups.push_back(*group);
            }
        }

        delete_all(eics);

        if (allgroups.size() > limitGroupCount ) break;
        if ( stopped() ) break;

        if (showProgressFlag && s % 10 == 0) {
            QString progressText = "Found " + QString::number(allgroups.size()) + " groups";
            emit(updateProgressBar( progressText , s+1, std::min((int)slices.size(),limitGroupCount)));
        }
    }

    if(alignSamplesFlag) {
        emit(updateProgressBar("Aligning Samples" ,1, 100));
        vector<PeakGroup*> groups(allgroups.size());
        for(int i=0; i <allgroups.size(); i++ ) groups[i]=&allgroups[i];
        Aligner aligner;
        aligner.setMaxItterations(mainwindow->alignmentDialog->maxItterations->value());
        aligner.setPolymialDegree(mainwindow->alignmentDialog->polynomialDegree->value());
        aligner.doAlignment(groups);
    }

    //run clustering
    if (isClusterPeakGroups){
        double maxRtDiff = 0.2;
        double minSampleCorrelation= 0.8;
        double minPeakShapeCorrelation=0.9;
        PeakGroup::clusterGroups(allgroups,samples,maxRtDiff,minSampleCorrelation,minPeakShapeCorrelation,ppmMerge);
    }

    //write reports
    CSVReports* csvreports = nullptr;
    if (writeCSVFlag) {
        string groupfilename = outputdir + setName + ".csv";
        csvreports = new CSVReports(samples);
        csvreports->setUserQuantType(mainwindow->getUserQuantType());
        csvreports->openGroupReport(groupfilename);
    }

    for(int j=0; j < allgroups.size(); j++ ) {
        PeakGroup& group = allgroups[j];
        Compound* compound = group.compound;

        if(csvreports) csvreports->addGroup(&group);

//Adds the links between compounds and peak groups - but not great if there are multiple searches

//        if (compound) {
//            if(!compound->hasGroup() || group.groupRank < compound->getPeakGroup()->groupRank )
//                compound->setPeakGroup(group);
//        }

        if(keepFoundGroups) {

            //Issue 501: debugging
            if (allgroups[j].compound && allgroups[j].adduct) {
                qDebug() << "PeakGroup: " << allgroups[j].compound->name.c_str() << " " << allgroups[j].adduct->name.c_str();
            }

            //cerr << "[BackgroundPeakUpdate::processSlices] "<< &allgroups[j] << " Id="<< (&allgroups[j])->groupId << ":(" << (&allgroups[j])->meanMz << "," << (&allgroups[j])->meanRt << ")" << endl;
            emit(newPeakGroup(&allgroups[j],false, false));
            //qDebug() << "Emiting..." << allgroups[j].meanMz;
            QCoreApplication::processEvents();
        }

        if (showProgressFlag && pullIsotopesFlag && j % 10 == 0) {
            emit(updateProgressBar("Calculating Isotopes", j , allgroups.size()));
        }
    }

    if(csvreports) { csvreports->closeFiles(); delete(csvreports); csvreports=nullptr; }

    emit(updateProgressBar("Not Started", 0, 1));

    qDebug() << "processSlices() Slices=" << slices.size();
    qDebug() << "processSlices() EICs="   << eicCount;
    qDebug() << "processSlices() Peaks, prior to EIC filtering=" << peakCountPreFilter;
    qDebug() << "processSlices() Groups, prior to filtering="   << totalGroupPreFilteringCounter;
    qDebug() << "processSlices() Groups, prior to eicMaxGroups filtering=" << totalGroupsToAppendCounter;
    qDebug() << "BackgroundPeakUpdate:processSlices() done. " << timer.elapsed() << " sec.";
    //cleanup();
}

bool BackgroundPeakUpdate::addPeakGroup(PeakGroup& grp) {

    for(int i=0; i<allgroups.size(); i++) {
        PeakGroup& grpB = allgroups[i];
        float rtoverlap = mzUtils::checkOverlap(grp.minRt, grp.maxRt, grpB.minRt, grpB.maxRt );
        if ( rtoverlap > 0.9 && ppmDist(grpB.meanMz, grp.meanMz) < ppmMerge  ) {
            return false;
        }
    }

    allgroups.push_back(grp);
    return true;
}

void BackgroundPeakUpdate::cleanup() { 
    allgroups.clear();
}

vector<CompoundIon> BackgroundPeakUpdate::prepareCompoundDatabase(vector<Compound*> set, bool debug) {

    vector<Adduct*> validAdducts;
    for (Adduct * a : DB.adductsDB) {
        if (SIGN(a->charge) == SIGN(ionizationMode)) {
            validAdducts.push_back(a);
        }
    }

    if (validAdducts.size() == 0) {
        qDebug() << "All adducts disagree with the ionizationMode of the experiment. No peaks can be matched.";
        return vector<CompoundIon>{};
    }

    long numEntries = static_cast<long>(set.size()) * static_cast<long>(validAdducts.size());

    vector<CompoundIon> searchableDatabase = vector<CompoundIon>(numEntries);

    unsigned int entryCounter = 0;

    for(Compound* c : set){
        for (Adduct* a : validAdducts) {

            float precMz = 0.0f;

            //Issue 501: predetermined precursor m/z is only valid for one specific adduct form
            if (c->precursorMz > 0 && c->adductString == a->name) {
                precMz = c->precursorMz;
            } else if (!c->formula.empty()){
                precMz = a->computeAdductMass(c->getExactMass());
            } else {
                precMz = c->getExactMass();
            }

            //DISQUALIFIER: adduct agreement
            // These compounds will be removed later
            if (isRequireMatchingAdduct && c->adductString != a->name) {
                precMz = -1.0f;
            }

            searchableDatabase[entryCounter] = CompoundIon(c, a, precMz);

            //Issue 725: Debugging
            // qDebug() << c->name.c_str() << a->name.c_str() << ": " << precMz;

            emit(updateProgressBar("Preparing Libraries for Peaks Search... ", entryCounter, numEntries));

            entryCounter++;
        }
    }

    qDebug() << "# CompoundIon before removing:" << searchableDatabase.size();

    //erase/remove any compounds that violate search constraint.
    searchableDatabase.erase(
        remove_if(searchableDatabase.begin(), searchableDatabase.end(),
                    [](const CompoundIon& obj){ return obj.precursorMz <= 0; }
                  ), searchableDatabase.end()
        );

    qDebug() << "# CompoundIon after removing:" << searchableDatabase.size();

    sort(searchableDatabase.begin(), searchableDatabase.end(), [](CompoundIon& lhs, CompoundIon& rhs){
        if (lhs.precursorMz != rhs.precursorMz) {
            return lhs.precursorMz < rhs.precursorMz;
        }

        if (lhs.compound->name != rhs.compound->name){
            return lhs.compound->name < rhs.compound->name;
        }

        return lhs.adduct->name < rhs.adduct->name;
    });

    if (debug) {
        cout << "searchableDatabase:\n";
        for (auto compoundIon : searchableDatabase) {
            cout << compoundIon.toString() << "\n";
        }
        cout << endl;
    }

    return searchableDatabase;
}

void BackgroundPeakUpdate::processCompounds(vector<Compound*> set, string setName) { 

    qDebug() << "BackgroundPeakUpdate:processCompounds(vector<Compound*> set, string setName)";

    if (set.size() == 0 ) return;

    vector<mzSlice*>slices;
    MassCalculator massCalc;

    vector<Adduct*> adductList;

    //Issue 161: search all adducts unless no adducts are selected.
    if (DB.adductsDB.empty()) {
        if (ionizationMode > 0) adductList.push_back(MassCalculator::PlusHAdduct);
        else if (ionizationMode < 0) adductList.push_back(MassCalculator::MinusHAdduct);
        else adductList.push_back(MassCalculator::ZeroMassAdduct);
    } else {
        adductList = DB.adductsDB;
    }

    multimap<string, Compound*> stringToCompoundMap = {};
    std::set<string> formulae = std::set<string>();

    int compoundCounter = 0;
    int numCompounds = static_cast<int>(set.size());

    for(Compound* c : set){

        compoundCounter++;
        emit(updateProgressBar("Preparing Libraries for Search", compoundCounter, numCompounds));

        if (!c){
            continue;
        }

        if (c->formula.empty()) {
            cerr << "skipping Compound \"" << c->name << "\" which is missing chemical formula!" << endl;
            continue;
        }

        string key = c->formula;

        formulae.insert(key);
        pair<string, Compound*> keyValuePair = make_pair(key, c);

        stringToCompoundMap.insert(keyValuePair);
    }

    typedef multimap<string, Compound*>::iterator compoundIterator;

    //TODO: instead of separating by sample, just organize all samples together into a single vector
    map<mzSample*, vector<Scan*>> allMs2Scans = {};

    if (compoundMustHaveMs2) {
        for (mzSample *sample : samples){

            vector<Scan*> scanVector;

            for (Scan *scan : sample->scans){
                if (scan->mslevel == 2){
                    scanVector.push_back(scan);
                }
            }

              sort(scanVector.begin(), scanVector.end(),
              [ ](const Scan* lhs, const Scan* rhs){
                    return lhs->precursorMz < rhs->precursorMz;
                }
              );

            allMs2Scans.insert(make_pair(sample, scanVector));
        }
    }

    int counter = 0;
    int allFormulaeCount = static_cast<int>(formulae.size());
    for (string formula : formulae) {

        counter++;
        emit(updateProgressBar("Computing Mass Slices", counter, allFormulaeCount));

        pair<compoundIterator, compoundIterator> compounds = stringToCompoundMap.equal_range(formula);

        Compound *c = nullptr;
        vector<Compound*> compoundVector;
        for (compoundIterator it = compounds.first; it != compounds.second; it++) {
            c = it->second;
            compoundVector.push_back(c);
        }

        float M0 = massCalc.computeNeutralMass(c->formula);
        if (M0 <= 0) continue;

        for(Adduct* a: adductList) {
            if (SIGN(a->charge) != SIGN(ionizationMode)) continue;

             mzSlice* slice = new mzSlice();
             slice->mz = a->computeAdductMass(M0);
             slice->compound = c; //this is just a random compound (not to be trusted) TODO: delete me
             slice->compoundVector = compoundVector;
             slice->adduct   = a;
             slice->mzmin = slice->mz - compoundPPMWindow * slice->mz/1e6;
             slice->mzmax = slice->mz + compoundPPMWindow * slice->mz/1e6;
             slice->rtmin = 0;
             slice->rtmax = 1e9;
             if (!c->srmId.empty()) slice->srmId=c->srmId;

            if (matchRtFlag && c->expectedRt > 0 ) {
                slice->rtmin = c->expectedRt-compoundRTWindow;
                slice->rtmax = c->expectedRt+compoundRTWindow;
            } else {
                slice->rtmin = 0;
                slice->rtmax = 1e9;
            }

            //debugging
//            cerr << "Formula: " << formula <<", Adduct: " << a->name << endl;
//            for (Compound *c : compoundVector) {
//                cerr << "Compound: " << c->name << "(formula=" << c->formula << ")" << endl;
//            }
//            cerr << endl;

            if(compoundMustHaveMs2) {

                bool isKeepSlice = false;
                for (mzSample *sample : samples){
                    map<mzSample*, vector<Scan*>>::iterator it = allMs2Scans.find(sample);

                    if (it != allMs2Scans.end()){
                        vector<Scan*> scanVector = it->second;

                        vector<Scan*>::iterator low = lower_bound(scanVector.begin(), scanVector.end(), slice->mzmin,
                                               [](const Scan* scan, const double& val){
                                    return scan->precursorMz < val;
                        });

                        for (vector<Scan*>::iterator scanIterator = low; scanIterator != scanVector.end(); scanIterator++){
                             Scan *scan = *scanIterator;
                             if (scan->precursorMz >= slice->mzmin && scan->precursorMz <= slice->mzmax){
                                 isKeepSlice = true;
                                 break;
                             } else if (scan->precursorMz > slice->mzmax){
                                 break;
                             }
                        }
                    }

                    if (isKeepSlice){
                        break;
                    }
                }

                if (!isKeepSlice){
                    delete(slice);
                    continue;
                }

            }

            slices.push_back(slice);
        }
    }

    //printSettings();
    //processSlices(slices,setName); //old approach

    //new approach
    processCompoundSlices(slices, setName);
    delete_all(slices);
}

bool BackgroundPeakUpdate::sliceHasMS2Event(mzSlice* slice) {
    //check if slice has ms2events
    for(mzSample* sample: samples) {
        if(sample->getFragmentationEvents(slice).size() > 0 ) return true;
    }
   return false;
}

void BackgroundPeakUpdate::processMassSlices() { 

         qDebug() << "BackgroundPeakUpdate:processMassSlices()";

		showProgressFlag=true;

        //Issue 161:
        //If this flag is true, if too many groups go by without finding a compound match, this will exit.
        //checkConvergance=true;

        QTime timer; timer.start();
	
        if (samples.size() > 0 ){
            avgScanTime = 0;
            for (auto sample : samples) {
                float singleSampleAvgFullScanTime = sample->getAverageFullScanTime();
                qDebug() << sample->getSampleName().c_str() << ":" << singleSampleAvgFullScanTime;
                avgScanTime += singleSampleAvgFullScanTime;
            }
            avgScanTime /= samples.size();
        }

		emit (updateProgressBar( "Computing Mass Slices", 2, 10 ));
        ParallelMassSlicer massSlices;
        massSlices.setSamples(samples);

        qDebug() << "Average scan time:" << (60*avgScanTime) << "seconds";
    
        if(mustHaveMS2) {
            //samples must be loaded for this to work
            massSlices.algorithmE(ppmMerge, rtStepSize*avgScanTime);
        } else {
            massSlices.algorithmB(ppmMerge, minGroupIntensity ,rtStepSize);
        }

		if (massSlices.slices.size() == 0) massSlices.algorithmA();

		sort(massSlices.slices.begin(), massSlices.slices.end(), mzSlice::compIntensity );

		emit (updateProgressBar( "Computing Mass Slices", 0, 10 ));

		vector<mzSlice*>goodslices;
		goodslices.resize(massSlices.slices.size());
		for(int i=0; i< massSlices.slices.size(); i++ ) goodslices[i] = massSlices.slices[i];

		if (goodslices.size() == 0 ) {
            emit (updateProgressBar( "Quitting! No good mass slices found", 1, 1 ));
			return;
		}

		string setName = "allslices";
		processSlices(goodslices,setName);
		delete_all(massSlices.slices); 
		massSlices.slices.clear();
		goodslices.clear();
		qDebug() << tr("processMassSlices() Done. ElepsTime=%1 msec").arg(timer.elapsed());
}

void BackgroundPeakUpdate::computePeaks() { 
		if (compounds.size() == 0 ) { return; }
		processCompounds(compounds,"compounds");
}

void BackgroundPeakUpdate::findPeaksQQQ() {
    if(!mainwindow) return;

    vector<SRMTransition*> transitions = mainwindow->getSRMTransitions();

    vector<mzSlice*> slices = QQQProcessor::getMzSlices(
                transitions,
                true, // isRequireCompound
                false // debug
                );

    //processSlices(slices,"QQQ Peaks");
    //Issue 347
    if (slices.empty()) return;

    processSRMTransitions(slices);

//    delete_all(slices);
//    delete_all(transitions.second);

//    slices.clear();
//    transitions.second.clear();
}

void BackgroundPeakUpdate::processSRMTransitions(vector<mzSlice*>&slices){

    if (slices.size() == 0) return;
    allgroups.clear();

    QSettings* settings = mainwindow->getSettings();

    amuQ1 = settings->value("amuQ1").toFloat();
    amuQ3 = settings->value("amuQ3").toFloat();

    QTime timer;
    timer.start();
    qDebug() << "BackgroundPeakUpdate::processSRMTransitions(): " << slices.size() << " transitions.";

    int eicCount=0;
    int groupCount=0;
    int peakCount=0;

    for (unsigned int s = 0; s < slices.size(); s++) {

        mzSlice *slice = slices[s];

        if (!slice || !slice->srmTransition) continue;

        SRMTransition *transition = slice->srmTransition;

        //Issue 347: SRM transitions that do not match to a compound or adduct are not retained.
        if (!transition->compound || !transition->adduct) continue;

        vector<EIC*>eics = pullEICs(slice,
                                    samples,
                                    EicLoader::PeakDetection,
                                    static_cast<int>(eic_smoothingWindow),
                                    eic_smoothingAlgorithm,
                                    amuQ1,
                                    amuQ3,
                                    baseline_smoothingWindow,
                                    baseline_dropTopX,
                                    "",
                                    make_pair(transition->precursorMz, transition->productMz));

        float eicMaxIntensity=0;

        for ( unsigned int j=0; j < eics.size(); j++ ) {
            eicCount++;
            if (eics[j]->maxIntensity > eicMaxIntensity) eicMaxIntensity=eics[j]->maxIntensity;
        }

        if (eicMaxIntensity < minGroupIntensity) {
            delete_all(eics);
            continue;
        }

        //find peaks
        for(unsigned int i=0; i < eics.size(); i++ ) {
            eics[i]->getPeakPositionsD(peakPickingAndGroupingParameters, false);
        }

        //Issue 381
        vector<PeakGroup> peakgroups = EIC::groupPeaksE(eics, peakPickingAndGroupingParameters);

        vector<PeakGroup*> groupsToAppend;

        for(unsigned int j=0; j < peakgroups.size(); j++ ) {

            PeakGroup& group = peakgroups[j];

            Peak* highestpeak = group.getHighestIntensityPeak();
            if(!highestpeak)  continue;

            if (clsf && clsf->hasModel()) {
                clsf->classify(&group);
            }

            group.computeAvgBlankArea(eics);
            group.groupStatistics();

            if (clsf && clsf->hasModel()&& group.goodPeakCount < minGoodPeakCount) continue;
            if (clsf && clsf->hasModel() && group.maxQuality < minQuality) continue;
            if (group.maxNoNoiseObs < minNoNoiseObs) continue;
            if (group.maxSignalBaselineRatio < minSignalBaseLineRatio) continue;
            if (group.maxIntensity < minGroupIntensity ) continue;
            if (group.blankMax*minSignalBlankRatio > group.maxIntensity) continue;

            groupCount++;
            peakCount += group.peakCount();

            group.compound = transition->compound;
            group.adduct = transition->adduct;

            if (featureMatchRtFlag && group.compound && group.compound->expectedRt>0) {
                float rtDiff =  static_cast<float>(abs(group.compound->expectedRt - (group.meanRt)));
                group.expectedRtDiff = rtDiff;
                group.groupRank = rtDiff*rtDiff*(1.1f-group.maxQuality)*(1.0f/log(group.maxIntensity+1.0f));
                if (group.expectedRtDiff > featureCompoundMatchRtTolerance) continue;
            }

            group._type = PeakGroup::GroupType::SRMTransitionType;
            group.srmPrecursorMz = transition->precursorMz;
            group.srmProductMz = transition->productMz;

            groupsToAppend.push_back(&group);
        }

        std::sort(groupsToAppend.begin(), groupsToAppend.end(),PeakGroup::compRankPtr);

        for(unsigned int j=0; j<groupsToAppend.size(); j++) {

            //check for duplicates  and append group
            if(static_cast<int>(j) >= eicMaxGroups) break;

            PeakGroup* group = groupsToAppend[j];
            addPeakGroup(*group);
        }

        delete_all(eics);

        if (static_cast<int>(allgroups.size()) > limitGroupCount ) break;

        if ( stopped() ) break;

        if (showProgressFlag && s % 10 == 0) {
            QString progressText = "Found " + QString::number(allgroups.size()) + " groups";
            emit(updateProgressBar( progressText , (static_cast<int>(s)+1), std::min(static_cast<int>(slices.size()),limitGroupCount)));
        }

    }

    //write reports
    CSVReports* csvreports = nullptr;
    if (writeCSVFlag) {
        string groupfilename = outputdir + "SRM_PeakGroups.csv";
        csvreports = new CSVReports(samples);
        csvreports->setUserQuantType(mainwindow->getUserQuantType());
        csvreports->openGroupReport(groupfilename);
    }

    //write groups to peakgroups table
    for(unsigned int j=0; j < allgroups.size(); j++ ) {

        PeakGroup& group = allgroups[j];

        if(csvreports) csvreports->addGroup(&group);

        if(keepFoundGroups) {
            emit(newPeakGroup(&group,false, false));
            QCoreApplication::processEvents();
        }
    }

    if(csvreports) {
        csvreports->closeFiles();
        delete(csvreports);
        csvreports=nullptr;
    }

    qDebug() << "processSlices() EICs="   << eicCount;
    qDebug() << "processSlices() Groups="   << groupCount;
    qDebug() << "processSlices() Peaks="   << peakCount;
    qDebug() << "BackgroundPeakUpdate:processSRMTransitions() done. " << timer.elapsed() << " sec.";

}

void BackgroundPeakUpdate::setRunFunction(QString functionName) { 
	runFunction = functionName.toStdString();
}


vector<EIC*> BackgroundPeakUpdate::pullEICs(mzSlice* slice,
                                            std::vector<mzSample*>&samples,
                                            int peakDetect,
                                            int smoothingWindow,
                                            int smoothingAlgorithm,
                                            float amuQ1,
                                            float amuQ3,
                                            int baseline_smoothingWindow,
                                            int baseline_dropTopX,
                                            string scanFilterString,
                                            pair<float, float> mzKey) {
	vector<EIC*> eics; 
	vector<mzSample*>vsamples;

	for( unsigned int i=0; i< samples.size(); i++ ) {
        if (!samples[i]) continue;
		if (samples[i]->isSelected == false) continue;
		vsamples.push_back(samples[i]);
	}



	if (vsamples.size()) {	
        /*EicLoader::PeakDetectionFlag pdetect = (EicLoader::PeakDetectionFlag) peakDetect;
        QFuture<EIC*>future = QtConcurrent::mapped(vsamples, EicLoader(slice, pdetect,smoothingWindow, smoothingAlgorithm, amuQ1,amuQ3));

        //wait for async operations to finish
        future.waitForFinished();

        QFutureIterator<EIC*> itr(future);

        while(itr.hasNext()) {
            EIC* eic = itr.next();
            if ( eic && eic->size() > 0) eics.push_back(eic);
        }
        */

        /*
        QList<EIC*> _eics = result.results();
        for(int i=0; i < _eics.size(); i++ )  {
            if ( _eics[i] && _eics[i]->size() > 0) {
                eics.push_back(_eics[i]);
            }
        }*/
	}


    // single threaded version

	for( unsigned int i=0; i< vsamples.size(); i++ ) {
		mzSample* sample = vsamples[i];

        Compound *c = nullptr;
        if (slice) c =  slice->compound;

        EIC* e = nullptr;

        if (slice->srmTransition) {
            //Issue 721: Debugging
            e = sample->getEIC(slice->srmTransition, Fragment::ConsensusIntensityAgglomerationType::Median, true);
        } else if (mzKey.first > 0 && mzKey.second > 0) { // SRM <precursor mz, product mz>
           e = sample->getEIC(mzKey, slice);
        } else if ( ! slice->srmId.empty() ) {
            //cout << "computeEIC srm:" << slice->srmId << endl;
            e = sample->getEIC(slice->srmId);
        } else if ( c && c->precursorMz >0 && c->productMz >0 ) {
            //cout << "computeEIC qqq: " << c->precursorMz << "->" << c->productMz << endl;
            e = sample->getEIC(c->precursorMz, c->collisionEnergy, c->productMz, amuQ1, amuQ3);
        } else {
            //cout << "computeEIC mzrange" << setprecision(7) << slice->mzmin  << " " << slice->mzmax << slice->rtmin  << " " << slice->rtmax << endl;
            e = sample->getEIC(slice->mzmin, slice->mzmax, slice->rtmin, slice->rtmax, 1, scanFilterString);
        }

        if (e) {
            EIC::SmootherType smootherType = static_cast<EIC::SmootherType>(smoothingAlgorithm);
            e->setSmootherType(smootherType);
            e->setBaselineSmoothingWindow(baseline_smoothingWindow);
            e->setBaselineDropTopX(baseline_dropTopX);
            e->getPeakPositions(smoothingWindow);
            eics.push_back(e);
        }
    }
	return eics;
}

void BackgroundPeakUpdate::printSettings() {
    cerr << "#Output folder=" <<  outputdir << endl;
    cerr << "#ionizationMode=" << ionizationMode << endl;
    cerr << "#keepFoundGroups=" << keepFoundGroups << endl;
    cerr << "#showProgressFlag=" << showProgressFlag << endl;

    cerr << "#rtStepSize=" << rtStepSize << endl;
    cerr << "#ppmMerge="  << ppmMerge << endl;
    cerr << "#avgScanTime=" << avgScanTime << endl;

    //peak detection
    cerr << "#eic_smoothingWindow=" << eic_smoothingWindow << endl;

    //peak grouping across samples
    cerr << "#grouping_maxRtWindow=" << grouping_maxRtWindow << endl;

    //peak filtering criteria
    cerr << "#minGoodPeakCount=" << minGoodPeakCount << endl;
    cerr << "#minSignalBlankRatio=" << minSignalBlankRatio << endl;
    cerr << "#minNoNoiseObs=" << minNoNoiseObs << endl;
    cerr << "#minSignalBaseLineRatio=" << minSignalBaseLineRatio << endl;
    cerr << "#minGroupIntensity=" << minGroupIntensity << endl;

    //compound detection setting
    cerr << "#compoundPPMWindow=" << compoundPPMWindow << endl;
    cerr << "#compoundRTWindow=" << compoundRTWindow << endl;
}

void BackgroundPeakUpdate::assignToGroup(PeakGroup *g, vector<CompoundIon>& searchableDatabase, bool debug) {
    if (scoringScheme == MzKitchenProcessor::LIPID_SCORING_NAME){
        MzKitchenProcessor::assignBestLipidToGroup(g, searchableDatabase, lipidSearchParameters, debug);
    } else if (scoringScheme == MzKitchenProcessor::METABOLITES_SCORING_NAME) {
        MzKitchenProcessor::assignBestMetaboliteToGroup(g, searchableDatabase, mzkitchenMetaboliteSearchParameters, debug);
    } else {
        assignToGroupSimple(g, searchableDatabase);
    }
}

void BackgroundPeakUpdate::assignToGroupSimple(PeakGroup* g, vector<CompoundIon>& searchableDatabase) {
    if(searchableDatabase.size() == 0) return;
    if(mustHaveMS2 && g->ms2EventCount == 0) return;

    float minMz = g->meanMz - (g->meanMz*featureCompoundMatchMzTolerance/1000000);
    float maxMz = g->meanMz + (g->meanMz*featureCompoundMatchMzTolerance/1000000);

    auto lb = lower_bound(searchableDatabase.begin(), searchableDatabase.end(), minMz, [](const CompoundIon& lhs, const float& rhs){
        return lhs.precursorMz < rhs;
    });

    int bestMatchingId = -1;
    FragmentationMatchScore bestScore;
    bestScore.mergedScore = -1; // Issue 443
    for (unsigned int pos = lb - searchableDatabase.begin(); pos < searchableDatabase.size(); pos++){

        CompoundIon compoundIon = searchableDatabase[pos];
        float precMz = compoundIon.precursorMz;

        //stop searching when the maxMz has been exceeded.
        if (precMz > maxMz) {
            break;
        }

        Compound *cpd = compoundIon.compound;
        Adduct *a = compoundIon.adduct;

        //DISQUALIFIER: adduct agreement
        if (isRequireMatchingAdduct && cpd->adductString != a->name) {
            continue;
        }

        //DISQUALIFIER: rt agreement
        // Issue 792: Switch to new approach
        if (featureMatchRtFlag && !MzKitchenProcessor::isRtAgreement(g->medianRt(), cpd, featureCompoundMatchRtTolerance, false)) {
            continue;
        }

        float rtDiff =  abs(cpd->expectedRt - g->medianRt());
        if (cpd->expectedRt > 0) {
            g->expectedRtDiff = rtDiff;
        }

        FragmentationMatchScore s = cpd->scoreCompoundHit(&g->fragmentationPattern, productPpmTolr, searchProton);

        //DISQUALIFIER: min MS2 matches agreement
        if (s.numMatches < minNumFragments) continue;
        if (Fragment::getNumDiagnosticFragmentsMatched("*",cpd->fragment_labels, s.ranks) < minNumDiagnosticFragments) continue;

        s.mergedScore = s.getScoreByName(scoringScheme.toStdString());

        //DISQUALIFIER: min MS2 score
        if (s.mergedScore < minFragmentMatchScore) continue;

        //Issue 699: Support case with no MS2 information (match is made entirely based on precursor m/z, adduct, rt)
        if (s.mergedScore >= bestScore.mergedScore ) {
            bestMatchingId = static_cast<int>(pos);
            bestScore = s;
        }
    }

    if (bestMatchingId != -1){
        CompoundIon bestMatchingCompoundIon = searchableDatabase[static_cast<unsigned int>(bestMatchingId)];
        g->compound = bestMatchingCompoundIon.compound;
        g->compoundId = g->compound->id;
        g->compoundDb = g->compound->db;
        g->adduct = bestMatchingCompoundIon.adduct;
        g->fragMatchScore = bestScore;
    }
}

IsotopeParameters BackgroundPeakUpdate::getSearchIsotopeParameters() {

    IsotopeParameters isotopeParameters = mainwindow->getIsotopeParameters();

    //Issue 751: override main window peakPickingAndGroupingParameters with search-specific parameters.
    isotopeParameters.peakPickingAndGroupingParameters = this->peakPickingAndGroupingParameters;

    isotopeParameters.ppm = compoundPPMWindow;

    // only applies to diffIso searches
    isotopeParameters.diffIsoAgglomerationType = diffIsoAgglomerationType;
    isotopeParameters.diffIsoIncludeSingleZero = diffIsoIncludeSingleZero;
    isotopeParameters.diffIsoIncludeDoubleZero = diffIsoIncludeDoubleZero;
    isotopeParameters.diffIsoReproducibilityThreshold = diffIsoReproducibilityThreshold;

    //note that isotopes derived from a search
    isotopeParameters.isotopeParametersType = IsotopeParametersType::SAVED;

    return isotopeParameters;
}
