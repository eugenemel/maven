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
    searchAdductsFlag=false;
    matchRtFlag=false;
    checkConvergance=false;

    outputdir = "reports" + string(DIR_SEPARATOR_STR);

    writeCSVFlag=false;
    ionizationMode = -1;
    keepFoundGroups=true;
    showProgressFlag=true;

    mzBinStep=static_cast<float>(0.01);
    rtStepSize=20;
    ppmMerge=30;
    avgScanTime=static_cast<float>(0.2);

    limitGroupCount=INT_MAX;

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

    if ( samples.size() == 0) samples = mainwindow->getSamples(); //get samples
	clsf = mainwindow->getClassifier();	//get classification modej

	if (samples.size() > 0 && samples[0]->getPolarity() > 0 ) ionizationMode = +1; 
	else ionizationMode = -1; //set ionization mode for compound matching
	if (mainwindow->getIonizationMode()) ionizationMode=mainwindow->getIonizationMode(); //user specified ionization mode

	if ( runFunction == "findPeaksQQQ" ) {
		findPeaksQQQ();
	} else if ( runFunction == "processSlices" ) { 
        processSlices();
	} else if ( runFunction == "processMassSlices" ) { 
		processMassSlices();
    } else if  (runFunction == "pullIsotopes" ) {
        pullIsotopes(_group);
	} else if  ( runFunction == "computePeaks" ) { 
		computePeaks();
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
    baseline_smoothingWindow = settings->value("baseline_smoothing").toInt();
    baseline_dropTopX =  settings->value("baseline_quantile").toInt();

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
                                    baseline_dropTopX);

        eicCount += eics.size();

        //find peaks
        for(int i=0; i < eics.size(); i++ )  eics[i]->getPeakPositions(eic_smoothingWindow);
        vector<PeakGroup> peakgroups = EIC::groupPeaks(eics, static_cast<int>(eic_smoothingWindow), grouping_maxRtWindow);

        //20191002: Too slow! Reverting to previous approach.
//        vector<PeakGroup> peakgroups = EIC::groupPeaksB(eics, static_cast<int>(eic_smoothingWindow), grouping_maxRtWindow, minSmoothedPeakIntensity);

        numAllPeakGroups += peakgroups.size();

         for (PeakGroup group : peakgroups) {

             group.computeAvgBlankArea(eics);
             group.groupStatistics();

             Peak* highestpeak = group.getHighestIntensityPeak();
             if(!highestpeak)  continue;

             if (clsf->hasModel()) { clsf->classify(&group); group.groupStatistics(); }
             if (clsf->hasModel()&& group.goodPeakCount < minGoodPeakCount) continue;
             if (group.blankMax*minSignalBlankRatio > group.maxIntensity) continue;
             if (group.maxNoNoiseObs < minNoNoiseObs) continue;
             if (group.maxSignalBaselineRatio < minSignalBaseLineRatio) continue;
             if (group.maxIntensity < minGroupIntensity ) continue;

             group.chargeState = group.getChargeStateFromMS1(compoundPPMWindow);
             vector<Isotope> isotopes = highestpeak->getScan()->getIsotopicPattern(highestpeak->peakMz,compoundPPMWindow,6,10);

             if(isotopes.size() > 0) {
                 //group.chargeState = isotopes.front().charge;
                 for(Isotope& isotope: isotopes) {
                     if (mzUtils::ppmDist(static_cast<float>(isotope.mass), static_cast<float>(group.meanMz)) < compoundPPMWindow) {
                         group.isotopicIndex=isotope.C13;
                     }
                 }
             }

             if (excludeIsotopicPeaks) {
                  if (group.chargeState > 0 and not group.isMonoisotopic(compoundPPMWindow)) continue;
             }

             group.computeFragPattern(productPpmTolr);

             if (mustHaveMS2 && group.ms2EventCount == 0) continue;

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

             for (Compound *compound : slice->compoundVector){

                 //MS2 criteria
                 FragmentationMatchScore s = compound->scoreCompoundHit(&group.fragmentationPattern, productPpmTolr, searchProton);
                 s.mergedScore = s.getScoreByName(scoringScheme.toStdString());

                 if (static_cast<float>(s.numMatches) < minNumFragments) continue;
                 if (static_cast<float>(s.mergedScore) < minFragmentMatchScore) continue;

                 //Rt criteria and group rank
                 if (matchRtFlag && compound->expectedRt>0) {
                     float rtDiff =  abs(compound->expectedRt - (group.meanRt));
                     group.expectedRtDiff = rtDiff;
                     group.groupRank = rtDiff*rtDiff*(1.1-group.maxQuality)*(1/log(group.maxIntensity+1));
                     if (rtDiff > compoundRTWindow ) continue;
                 } else {
                     group.groupRank = (1.1-group.maxQuality)*(1/log(group.maxIntensity+1));
                 }

                 PeakGroup *peakGroupPtr = new PeakGroup(group);
                 //delete(peakGroupPtr) is called in the slot that receives the emit(newPeakGroup()) call

                 peakGroupPtr->compound = compound;
                 peakGroupPtr->adduct = slice->adduct;
                 peakGroupPtr->tagString = slice->adduct->name;
                 peakGroupPtr->fragMatchScore = s;
                 peakGroupPtr->ms2EventCount = group.ms2EventCount;

                 if (csvreports) {
                     csvreports->addGroup(peakGroupPtr);
                 }

                isGroupRetained = true;

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


             numPassingPeakGroups += passingPeakGroups.size();
             delete_all(failingPeakGroups); //passingPeakGroups are deleted on main thread with emit() call.

             for (auto peakGroupPtr : passingPeakGroups){

                 //debugging
//                 cerr << "[BackgroundPeakUpdate::processCompoundSlices] "
//                      << peakGroupPtr << " Id="
//                      << peakGroupPtr->groupId << ":("
//                      << peakGroupPtr->meanMz << ","
//                      << peakGroupPtr->meanRt << ") <--> "
//                      << peakGroupPtr->compound->name << ":"
//                      << peakGroupPtr->adduct->name << ", score="
//                      << peakGroupPtr->fragMatchScore.mergedScore << ", ms2EventCount="
//                      << peakGroupPtr->ms2EventCount
//                      << endl;

                 emit(newPeakGroup(peakGroupPtr, false, true)); // note that 'isDeletePeakGroupPtr' flag is set to true
             }

             if (isGroupRetained) {
                groups.push_back(&group);
                peakCount += group.peakCount();
             }

         } // end peak groups

         delete_all(eics);

         if ( stopped() ) break;

         if (showProgressFlag && s % 10 == 0) {
             QString progressText = "Found " + QString::number(numPassingPeakGroups) + " Compound <--> Peak Group matches.";
             emit(updateProgressBar( progressText , (s+1), std::min(static_cast<int>(slices.size()),limitGroupCount)));
         }

    } //end slices

    if(alignSamplesFlag) {
        emit(updateProgressBar("Aligning Samples" ,1, 100));
        Aligner aligner;
        aligner.setMaxItterations(mainwindow->alignmentDialog->maxItterations->value());
        aligner.setPolymialDegree(mainwindow->alignmentDialog->polynomialDegree->value());
        aligner.doAlignment(groups);
    }

    //TODO: respect clustering option

    if (csvreports){
        csvreports->closeFiles();
        delete(csvreports);
    }

    emit(updateProgressBar("Done" ,1, 1));

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

    //process KNOWNS
    QTime timer;
    timer.start();
    qDebug() << "BackgroundPeakUpdate::processSlices(vector<mzSlice*>&slices, string setName)";
    qDebug() << "Processing slices: setName=" << setName.c_str() << " slices=" << slices.size();

    int converged=0;
    int foundGroups=0;

    int eicCount=0;
    int groupCount=0;
    int peakCount=0;

    QSettings* settings = mainwindow->getSettings();
    amuQ1 = settings->value("amuQ1").toFloat();
    amuQ3 = settings->value("amuQ3").toFloat();
    baseline_smoothingWindow = settings->value("baseline_smoothing").toInt();
    baseline_dropTopX =  settings->value("baseline_quantile").toInt();

    for (unsigned int s=0; s < slices.size();  s++ ) {
        mzSlice* slice = slices[s];
        Compound* compound = slice->compound;

       // if (compound)  cerr << "Searching for: " << compound->name << "\trt=" << compound->expectedRt << endl;

        if (compound && compound->hasGroup())
            compound->unlinkGroup();

        if (checkConvergance ) {
            allgroups.size()-foundGroups > 0 ? converged=0 : converged++;
            if ( converged > 1000 ) break;	 //exit main loop
            foundGroups = allgroups.size();
        }

        vector<EIC*>eics = pullEICs(slice,samples,
                                    EicLoader::PeakDetection,
                                    static_cast<int>(eic_smoothingWindow),
                                    eic_smoothingAlgorithm,
                                    amuQ1,
                                    amuQ3,
                                    baseline_smoothingWindow,
                                    baseline_dropTopX);
        float eicMaxIntensity=0;

        for ( unsigned int j=0; j < eics.size(); j++ ) {
            eicCount++;
            if (eics[j]->maxIntensity > eicMaxIntensity) eicMaxIntensity=eics[j]->maxIntensity;
        }
        if (eicMaxIntensity < minGroupIntensity) { delete_all(eics); continue; }

        //find peaks
        for(int i=0; i < eics.size(); i++ )  eics[i]->getPeakPositions(eic_smoothingWindow);

        vector<PeakGroup> peakgroups = EIC::groupPeaks(eics, static_cast<int>(eic_smoothingWindow), grouping_maxRtWindow);

        //20191002: Too slow! Reverting to previous approach.
        //vector<PeakGroup> peakgroups = EIC::groupPeaksB(eics, static_cast<int>(eic_smoothingWindow), grouping_maxRtWindow, minSmoothedPeakIntensity);

        //cerr << "\tFound " << peakgroups.size() << "\n";

        //score quality of each group
        vector<PeakGroup*> groupsToAppend;
        for(unsigned int j=0; j < peakgroups.size(); j++ ) {

            PeakGroup& group = peakgroups[j];

            groupCount++;
            peakCount += group.peakCount();

            Peak* highestpeak = group.getHighestIntensityPeak();
            if(!highestpeak)  continue;

            if (clsf->hasModel()) {
                clsf->classify(&group);
            }

            group.computeAvgBlankArea(eics);
            group.groupStatistics();

            if (clsf->hasModel()&& group.goodPeakCount < minGoodPeakCount) continue;
            if (clsf->hasModel() && group.maxQuality < minQuality) continue;

            // if (group.blankMean*minBlankRatio > group.sampleMean ) continue;
            if (group.maxNoNoiseObs < minNoNoiseObs) continue;
            if (group.maxSignalBaselineRatio < minSignalBaseLineRatio) continue;
            if (group.maxIntensity < minGroupIntensity ) continue;

            if (group.blankMax*minSignalBlankRatio > group.maxIntensity) continue;

            group.chargeState = group.getChargeStateFromMS1(compoundPPMWindow);
            vector<Isotope> isotopes = highestpeak->getScan()->getIsotopicPattern(highestpeak->peakMz,compoundPPMWindow, 6, 10);

            if(isotopes.size() > 0) {
                //group.chargeState = isotopes.front().charge;
                for(Isotope& isotope: isotopes) {
                    if (mzUtils::ppmDist((float) isotope.mass, (float) group.meanMz) < compoundPPMWindow) {
                        group.isotopicIndex=isotope.C13;
                    }
                }
            }

           if ((excludeIsotopicPeaks or compound)) {
                if (group.chargeState > 0 and not group.isMonoisotopic(compoundPPMWindow)) continue;
            }

            //if (getChargeStateFromMS1(&group) < minPrecursorCharge) continue;
                        //build consensus ms2 specta
            //vector<Scan*>ms2events = group.getFragmenationEvents();
            //cerr << "\tFound ms2events" << ms2events.size() << "\n";

            group.computeFragPattern(productPpmTolr);
            // BROKEN group.findHighestPurityMS2Pattern(compoundPPMWindow);

            //TODO: restructuring, expect that compound is always nullptr

            matchFragmentation(&group);
            //If this is successful, then group.compound will not be nullptr.

            if (!isRetainUnmatchedCompounds && !group.compound){
                continue;
            }

            if(mustHaveMS2) {
                if(group.ms2EventCount == 0) continue;

                if (group.compound) {
                    bool isValidCompound = (group.fragMatchScore.mergedScore >= this->minFragmentMatchScore &&
                                            group.fragMatchScore.numMatches >= this->minNumFragments);

                    if (!isValidCompound ){
                        //invalid compound - either skip this group, or replace with unmatched peak.

                        if (isRetainUnmatchedCompounds) {
                            group.compound = nullptr;
                        } else {
                            continue;
                        }
                    }
                }
            }

            if (!slice->srmId.empty()) group.srmId = slice->srmId;

            if (matchRtFlag && compound && compound->expectedRt>0) {
                float rtDiff =  abs(compound->expectedRt - (group.meanRt));
                group.expectedRtDiff = rtDiff;
                group.groupRank = rtDiff*rtDiff*(1.1-group.maxQuality)*(1/log(group.maxIntensity+1));
                if (group.expectedRtDiff > compoundRTWindow ) continue;
            } else {
                group.groupRank = (1.1-group.maxQuality)*(1/log(group.maxIntensity+1));
            }

            groupsToAppend.push_back(&group);
        }

        std::sort(groupsToAppend.begin(), groupsToAppend.end(),PeakGroup::compRankPtr);

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
        PeakGroup::clusterGroups(allgroups,samples,maxRtDiff,minSampleCorrelation,minPeakShapeCorrelation,compoundPPMWindow);
    }

    if (showProgressFlag && pullIsotopesFlag ) {
        emit(updateProgressBar("Calculation Isotopes" ,1, 100));
    }

    //write reports
    CSVReports* csvreports = NULL;
    if (writeCSVFlag) {
        string groupfilename = outputdir + setName + ".csv";
        csvreports = new CSVReports(samples);
        csvreports->setUserQuantType(mainwindow->getUserQuantType());
        csvreports->openGroupReport(groupfilename);
    }

    for(int j=0; j < allgroups.size(); j++ ) {
        PeakGroup& group = allgroups[j];
        Compound* compound = group.compound;

        if(pullIsotopesFlag && !group.isIsotope()) pullIsotopes(&group);
        if(csvreports!=NULL) csvreports->addGroup(&group);

        if (compound) {
            if(!compound->hasGroup() || group.groupRank < compound->getPeakGroup()->groupRank )
                compound->setPeakGroup(group);
        }

        if(keepFoundGroups) {
            //cerr << "[BackgroundPeakUpdate::processSlices] "<< &allgroups[j] << " Id="<< (&allgroups[j])->groupId << ":(" << (&allgroups[j])->meanMz << "," << (&allgroups[j])->meanRt << ")" << endl;
            emit(newPeakGroup(&allgroups[j],false, false));
            //qDebug() << "Emmiting..." << allgroups[j].meanMz;
            QCoreApplication::processEvents();
        }

        if (showProgressFlag && pullIsotopesFlag && j % 10 == 0) {
            emit(updateProgressBar("Calculating Isotopes", j , allgroups.size()));
        }
    }

    if(csvreports!=NULL) { csvreports->closeFiles(); delete(csvreports); csvreports=NULL; }
    emit(updateProgressBar("Done" ,1, 1));

    qDebug() << "processSlices() Slices=" << slices.size();
    qDebug() << "processSlices() EICs="   << eicCount;
    qDebug() << "processSlices() Groups="   << groupCount;
    qDebug() << "processSlices() Peaks="   << peakCount;
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

void BackgroundPeakUpdate::processCompounds(vector<Compound*> set, string setName) { 

    qDebug() << "BackgroundPeakUpdate:processCompounds(vector<Compound*> set, string setName)";

    if (set.size() == 0 ) return;

    vector<mzSlice*>slices;
    MassCalculator massCalc;

    vector<Adduct*> adductList;
    if (ionizationMode > 0) adductList.push_back(MassCalculator::PlusHAdduct);
    else if (ionizationMode < 0) adductList.push_back(MassCalculator::MinusHAdduct);
    else adductList.push_back(MassCalculator::ZeroMassAdduct);

    //search all adducts
    if (searchAdductsFlag) adductList = DB.adductsDB;

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

    if (mustHaveMS2) {
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

            if(mustHaveMS2) {

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

void BackgroundPeakUpdate::setSamples(vector<mzSample*>&set){ 
	samples=set;   
	if ( samples.size() > 0 ) avgScanTime = samples[0]->getAverageFullScanTime();
}

void BackgroundPeakUpdate::processMassSlices() { 

         qDebug() << "BackgroundPeakUpdate:processMassSlices()";

		showProgressFlag=true;
		checkConvergance=true;
		QTime timer; timer.start();
	
        if (samples.size() > 0 ){
            avgScanTime = 0;
            for (auto sample : samples) {
                float singleSampleAvgFullScanTime = sample->getAverageFullScanTime();
                avgScanTime += singleSampleAvgFullScanTime;
            }
            avgScanTime /= samples.size();
        }

		emit (updateProgressBar( "Computing Mass Slices", 2, 10 ));
        ParallelMassSlicer massSlices;
        massSlices.setSamples(samples);

        if(mustHaveMS2) {
            //rsamples must be loaded for this to work
            massSlices.algorithmE(compoundPPMWindow, rtStepSize*avgScanTime);
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
	if(mainwindow == NULL) return;
	vector<mzSlice*>slices = mainwindow->getSrmSlices();
	processSlices(slices,"QQQ Peaks");
	delete_all(slices);
	slices.clear();
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
                                            int baseline_dropTopX) {
	vector<EIC*> eics; 
	vector<mzSample*>vsamples;

	for( unsigned int i=0; i< samples.size(); i++ ) {
        if (samples[i] == NULL) continue;
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
		Compound* c =  slice->compound;

        EIC* e = NULL;

        if ( ! slice->srmId.empty() ) {
            //cout << "computeEIC srm:" << slice->srmId << endl;
            e = sample->getEIC(slice->srmId);
        } else if ( c && c->precursorMz >0 && c->productMz >0 ) {
            //cout << "computeEIC qqq: " << c->precursorMz << "->" << c->productMz << endl;
            e = sample->getEIC(c->precursorMz, c->collisionEnergy, c->productMz, amuQ1, amuQ3);
        } else {
            //cout << "computeEIC mzrange" << setprecision(7) << slice->mzmin  << " " << slice->mzmax << slice->rtmin  << " " << slice->rtmax << endl;
            e = sample->getEIC(slice->mzmin, slice->mzmax, slice->rtmin, slice->rtmax, 1);
        }

        if (e) {

            //TODO: what should be retained here?
//            EIC::SmootherType smootherType = (EIC::SmootherType) smoothingAlgorithm;
//            e->setSmootherType(smootherType);
//            e->setBaselineSmoothingWindow(baseline_smoothingWindow);
//            e->setBaselineDropTopX(baseline_dropTopX);
//            e->getPeakPositions(smoothingWindow);
            eics.push_back(e);
        }
    }
	return eics;
}

void BackgroundPeakUpdate::pullIsotopes(PeakGroup* parentgroup) {

    if(parentgroup == NULL) return;
    if(parentgroup->compound == NULL ) return;
    if(parentgroup->compound->formula.empty() == true) return;
    if ( samples.size() == 0 ) return;

    float ppm = compoundPPMWindow;
    double maxIsotopeScanDiff = 10;
    double maxNaturalAbundanceErr = 100;
    double minIsotopicCorrelation=0;
    bool   C13Labeled=false;
    bool   N15Labeled=false;
    bool   S34Labeled=false;
    bool   D2Labeled=false;
    int eic_smoothingAlgorithm = 0;

    //Issue 130: Increase isotope calculation accuracy by using sample-specific mz values
    map<mzSample*, double> sampleToPeakMz{};
    for (Peak& p : parentgroup->peaks) {
        sampleToPeakMz.insert(make_pair(p.sample, p.peakMz));
    }

    if(mainwindow) {
        QSettings* settings = mainwindow->getSettings();
        if (settings) {
            maxIsotopeScanDiff  = settings->value("maxIsotopeScanDiff").toDouble();
            minIsotopicCorrelation  = settings->value("minIsotopicCorrelation").toDouble();
            maxNaturalAbundanceErr  = settings->value("maxNaturalAbundanceErr").toDouble();
            C13Labeled   =  settings->value("C13Labeled").toBool();
            N15Labeled   =  settings->value("N15Labeled").toBool();
            S34Labeled   =  settings->value("S34Labeled").toBool();
            D2Labeled   =   settings->value("D2Labeled").toBool();
            QSettings* settings = mainwindow->getSettings();
            eic_smoothingAlgorithm = settings->value("eic_smoothingAlgorithm").toInt();

	    //Feng note: assign labeling state to sample
	    samples[0]->_C13Labeled = C13Labeled;
	    samples[0]->_N15Labeled = N15Labeled;
	    samples[0]->_S34Labeled = S34Labeled;
	    samples[0]->_D2Labeled = D2Labeled;
	    //End Feng addition
        }
    }

    string formula = parentgroup->compound->formula;
    vector<Isotope> masslist = mcalc.computeIsotopes(formula,ionizationMode);

    map<string,PeakGroup>isotopes;
    map<string,PeakGroup>::iterator itr2;
    for ( unsigned int s=0; s < samples.size(); s++ ) {
        mzSample* sample = samples[s];
        for (int k=0; k< masslist.size(); k++) {
            if ( stopped() ) break;
            Isotope& x= masslist[k];
            string isotopeName = x.name;
            double isotopeMass = x.mass;
            double expectedAbundance = x.abundance;
            float mzmin = isotopeMass-isotopeMass/1e6*ppm;
            float mzmax = isotopeMass+isotopeMass/1e6*ppm;
            float rt =  parentgroup->medianRt();
            float rtmin = parentgroup->minRt;
            float rtmax = parentgroup->maxRt;

            Peak* parentPeak =  parentgroup->getPeak(sample);
            if ( parentPeak ) rt  =   parentPeak->rt;
            if ( parentPeak ) rtmin = parentPeak->rtmin;
            if ( parentPeak ) rtmax = parentPeak->rtmax;

            float isotopePeakIntensity=0;
            float parentPeakIntensity=0;

            if ( parentPeak ) {
                parentPeakIntensity=parentPeak->peakIntensity;
                int scannum = parentPeak->getScan()->scannum;
                for(int i= scannum-3; i < scannum+3; i++ ) {
                    Scan* s = sample->getScan(i);

                    //look for isotopic mass in the same spectrum
                    vector<int> matches = s->findMatchingMzs(mzmin,mzmax);

                    for(int i=0; i < matches.size(); i++ ) {
                        int pos = matches[i];
                        if ( s->intensity[pos] > isotopePeakIntensity ) {
                            isotopePeakIntensity = s->intensity[pos];
                            rt = s->rt;
                        }
                    }
                }

            }
            //if(isotopePeakIntensity==0) continue;

            //natural abundance check
            if (    (x.C13 > 0    && C13Labeled==false)
                    || (x.N15 > 0 && N15Labeled==false)
                    || (x.S34 > 0 && S34Labeled==false )
                    || (x.H2 > 0  && D2Labeled==false )

                    ) {
                if (expectedAbundance < 1e-8) continue;
                if (expectedAbundance * parentPeakIntensity < 1) continue;
                float observedAbundance = isotopePeakIntensity/(parentPeakIntensity+isotopePeakIntensity);
                float naturalAbundanceError = abs(observedAbundance-expectedAbundance)/expectedAbundance*100;

                //cerr << isotopeName << endl;
                //cerr << "Expected isotopeAbundance=" << expectedAbundance;
                //cerr << " Observed isotopeAbundance=" << observedAbundance;
                //cerr << " Error="     << naturalAbundanceError << endl;

                if (naturalAbundanceError > maxNaturalAbundanceErr )  continue;
            }


            float w = maxIsotopeScanDiff*avgScanTime;
            double c = sample->correlation(isotopeMass,parentgroup->meanMz,ppm, rtmin-w,rtmax+w);
            if (c < minIsotopicCorrelation)  continue;

            //cerr << "pullIsotopes: " << isotopeMass << " " << rtmin-w << " " <<  rtmin+w << " c=" << c << endl;

            //show correlation values
            qDebug() << "ISSUE-130-DEBUGGING" << sample->getSampleName().c_str()
                     << ": mzmin=" << QString::number(mzmin,'f', 6) << ", mzmax=" << QString::number(mzmax,'f',6)
                     << " rtmin=" << QString::number((rtmin-w), 'f', 6) << ", rtmax=" << QString::number((rtmax+w),'f', 6)
                     << ", correlation=" << QString::number(c, 'f', 6);

            EIC* eic=nullptr;
            for( int i=0; i<maxIsotopeScanDiff; i++ ) {
                float window=i*avgScanTime;
                eic = sample->getEIC(mzmin,mzmax,rtmin-window,rtmax+window,1);
                if(!eic) continue;

                eic->setSmootherType((EIC::SmootherType) eic_smoothingAlgorithm);
                eic->getPeakPositions(eic_smoothingWindow);
                if (eic->peaks.size() >= 1 ) break;

                //clean up
                delete(eic); eic=nullptr;
            }

            if (eic) {
                Peak* nearestPeak=nullptr; float d=FLT_MAX;
                for(int i=0; i < eic->peaks.size(); i++ ) {
                    Peak& x = eic->peaks[i];
                    float dist = abs(x.rt - rt);
                    if ( dist > maxIsotopeScanDiff*avgScanTime) continue;
                    if ( dist < d ) { d=dist; nearestPeak = &x; }
                }

                if (nearestPeak) {
                    if (isotopes.count(isotopeName)==0) {
                        PeakGroup g;
                        g.meanMz=isotopeMass;
                        g.tagString=isotopeName;
                        g.expectedAbundance=expectedAbundance;
                        g.isotopeC13count= x.C13;
                        isotopes[isotopeName] = g;
                    }
                    isotopes[isotopeName].addPeak(*nearestPeak);
                }
                delete(eic);
            }
        }
    }

    parentgroup->children.clear();
    for(itr2 = isotopes.begin(); itr2 != isotopes.end(); itr2++ ) {
        string isotopeName = (*itr2).first;
        PeakGroup& child = (*itr2).second;
        child.tagString = isotopeName;
        child.metaGroupId = parentgroup->metaGroupId;
        child.groupId = parentgroup->groupId;
        child.compound = parentgroup->compound;
        child.parent = parentgroup;
        child.setType(PeakGroup::IsotopeType);
        child.groupStatistics();
        if (clsf->hasModel()) { clsf->classify(&child); child.groupStatistics(); }
        parentgroup->addChild(child);
        //cerr << " add: " << isotopeName << " " <<  child.peaks.size() << " " << isotopes.size() << endl;
    }
    //cerr << "Done: " << parentgroup->children.size();
    /*
            //if ((float) isotope.maxIntensity/parentgroup->maxIntensity > 3*ab[isotopeName]/ab["C12 PARENT"]) continue;
        */
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

void BackgroundPeakUpdate::matchFragmentation(PeakGroup* g) {
    if(DB.compoundsDB.size() == 0) return;
    if(g->ms2EventCount == 0) return;

    double searchMz = g->meanMz;

    /*Peak* peak = g->getHighestIntensityPeak();
    if (!peak or !peak->getScan()) return;
    vector<Isotope> isotopes = peak->getScan()->getIsotopicPattern(searchMz,compoundPPMWindow,6,10);
    if(isotopes.size() > 0) searchMz = isotopes.front().mass;
    */

    vector<MassCalculator::Match>matchesX = DB.findMatchingCompounds(searchMz,compoundPPMWindow,ionizationMode);
    //qDebug() << "findMatching" << searchMz << " " << matchesX.size();

  // g->fragmentationPattern.printFragment(productPpmTolr,10);
    MassCalculator::Match bestMatch;
    string scoringAlgorithm = scoringScheme.toStdString();

    for(MassCalculator::Match match: matchesX ) {
        Compound* cpd = match.compoundLink;
        if(!compoundDatabase.isEmpty() and  cpd->db != compoundDatabase.toStdString()) continue;

        if (isRequireMatchingAdduct && cpd->adductString != match.adductLink->name) {
            continue;
        }

        FragmentationMatchScore s = cpd->scoreCompoundHit(&g->fragmentationPattern,productPpmTolr,searchProton);
        if (s.numMatches < minNumFragments ) continue;
        s.mergedScore = s.getScoreByName(scoringAlgorithm);
        s.ppmError = match.diff;
        //qDebug() << "scoring=" << scoringScheme << " " << s.mergedScore;

        //cerr << "\t"; cerr << cpd->name << "\t" << cpd->precursorMz << "\tppmDiff=" << m.diff << "\t num=" << s.numMatches << "\t tic" << s.ticMatched <<  " s=" << s.spearmanRankCorrelation << endl;

        match.fragScore = s;
        if (match.fragScore.mergedScore > bestMatch.fragScore.mergedScore ) {
            bestMatch = match;
            bestMatch.adductLink = match.adductLink;
            bestMatch.compoundLink = match.compoundLink;
        }
    }

    if (bestMatch.fragScore.mergedScore > 0) {
        g->compound = bestMatch.compoundLink;
        g->adduct  =  bestMatch.adductLink;
        g->fragMatchScore = bestMatch.fragScore;

        if (g->adduct) g->tagString = g->adduct->name;
        /*
                cerr << "\t BESTHIT"
                     << setprecision(6)
                     << g->meanMz << "\t"
                     << g->meanRt << "\t"
                     << g->maxIntensity << "\t"
                     << bestHit->name << "\t"
                     << bestScore.numMatches << "\t"
                     << bestScore.fractionMatched << "\t"
                     << bestScore.ticMatched << endl;
                */

    }
}
