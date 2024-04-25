//dump basic scan information
#include "mzSample.h"
#include "mzUtils.h"
#include <iostream>
#include <sys/stat.h>

struct QCStandard;
struct SearchParams;
struct SearchDataset;

void writePeaks(EIC* eic, QCStandard qcStandard, shared_ptr<SearchDataset> searchDataset, string tag="eic");
void writeEIC(EIC* eic, QCStandard qcStandard, shared_ptr<SearchDataset> searchDataset, string tag="eic");

shared_ptr<SearchDataset> processArguments(int argc, char* argv[]);
vector<QCStandard> importStandards(string qcStandardsFile);

EIC* pullEIC(mzSample* sample, QCStandard qcStandard, shared_ptr<SearchDataset> searchDataset);

void writeHeaders(shared_ptr<SearchDataset> searchDataset);

int file_unique_id(string filename);
void processSample(string fileIn, shared_ptr<SearchDataset> searchDataset);

struct QCStandard {

    string compoundName;
    string standardSet;
    string msMethod;
    string standardType;
    string formula;
    string adductName;

    double precMz;
    double rt;
};

struct SearchParams {

    float precMzTol = 10;
    float rtTol = -1; //if rtTol <0, span full sample range
    float minIntensity = 100;

    int msLevel = 1;

    EIC::SmootherType smootherType = EIC::SmootherType::GAUSSIAN;
    int smootherWindow = 10;
    int baseline_smoothingWindow = 10;
    int baseline_dropTopX = 60;

    string qcStandardsFilePath;

    string outputDir;
    string eicFilePath;
    string peakFilePath;

    //match standards to sample types according to various criteria
    bool isMatchMethod = false;
    bool isMatchSampleType = false;
};

struct SearchDataset {
    vector<string> sampleFiles;
    vector<QCStandard> standards;
    SearchParams searchParams;
    ofstream eicFile;
    ofstream peakFile;
};

int main(int argc, char *argv[]) {

    shared_ptr<SearchDataset> searchDataset = processArguments(argc, argv);

    if (searchDataset->standards.empty()) {
        cout << "Nothing to do - no standards found in QC standards file." << endl;
        exit(0);
    }

    if (searchDataset->sampleFiles.empty()) {
        cout << "Nothing to do - no mzML samples discovered." << endl;
        exit(0);
    }

    searchDataset->eicFile.open(searchDataset->searchParams.eicFilePath);
    searchDataset->peakFile.open(searchDataset->searchParams.peakFilePath);

    if (!searchDataset->eicFile.is_open() || !searchDataset->peakFile.is_open()) {
        cout << "EIC and peaks output files could not be found or accessed." << endl;
        exit(1);
    }

    writeHeaders(searchDataset);

    //one sample at a time
    for (auto sampleFile : searchDataset->sampleFiles) {
        processSample(sampleFile, searchDataset);
    }

    searchDataset->eicFile.close();
    searchDataset->peakFile.close();

    cout << "All Processes Successfully Completed!" << endl;
}

shared_ptr<SearchDataset> processArguments(int argc, char* argv[]) {

    shared_ptr<SearchDataset> searchDataset = shared_ptr<SearchDataset>(new SearchDataset());

    cout << "Reading in CLI arguments..." << endl;

    SearchParams searchParams;

    vector<string> sampleFiles{};

    for (unsigned int i = 1; i < static_cast<unsigned int>(argc); i++) {

        string argString(argv[i]);

        if (strcmp(argv[i],"-ppm") == 0) {
            searchParams.precMzTol = stoi(argv[i+1]);
        } else if (strcmp(argv[i], "-rttol") == 0) {
            searchParams.rtTol = stof(argv[i+1]);
        } else if (strcmp(argv[i], "-minions") == 0) {
            searchParams.minIntensity = stof(argv[i+1]);
        } else if (strcmp(argv[i], "-qcfile") == 0) {
            searchParams.qcStandardsFilePath = argv[i+1];
        } else if (strcmp(argv[i], "-o") == 0) {
            searchParams.outputDir = argv[i+1];
        } else if (strcmp(argv[i], "-peakFile") == 0) {
            searchParams.peakFilePath = argv[i+1];
        } else if (strcmp(argv[i], "-eicFile") == 0) {
            searchParams.eicFilePath = argv[i+1];
        } else if (strcmp(argv[i], "-bwindow") == 0) {
            searchParams.baseline_smoothingWindow = stoi(argv[i+1]);
        } else if (strcmp(argv[i], "-bdrop") == 0) {
            searchParams.baseline_dropTopX = stoi(argv[i+1]);
        } else if (strcmp(argv[i], "-smooth") == 0) {
            searchParams.smootherWindow = stoi(argv[i+1]);
        } else if (strcmp(argv[i], "-match-method") == 0) {
            searchParams.isMatchMethod = true;
        } else if (strcmp(argv[i], "-match-type") ==0) {
            searchParams.isMatchSampleType = true;
        }

        if (mzUtils::ends_with(argString, ".mzML")) {
            sampleFiles.push_back(argString);
        }

        if (mzUtils::ends_with(argString, "/")) {
            vector<string> dirFileNames = mzUtils::getMzSampleFilesFromDirectory(argString.c_str());
            for (auto dirFileName : dirFileNames) {
                sampleFiles.push_back(dirFileName);
            }
        }
    }

    if (searchParams.peakFilePath.empty()) {
        searchParams.peakFilePath = searchParams.outputDir + "/peaks.tsv";
    }
    if (searchParams.eicFilePath.empty()) {
        searchParams.eicFilePath = searchParams.outputDir + "/eics.tsv";
    }

    cout << "===============================\n";
    cout << "Search Parameters:\n"
         << "mz tol = " << searchParams.precMzTol << " ppm\n"
         << "rt tol = " << searchParams.rtTol << " min\n"
         << "QC standards file: " << searchParams.qcStandardsFilePath << "\n"
         << "smoothing window = " << searchParams.smootherWindow << "\n"
         << "baseline smoothing window = " << searchParams.baseline_smoothingWindow << "\n"
         << "baseline drop top X = " << searchParams.baseline_dropTopX << "\n"
         << "Output EIC file: " << searchParams.eicFilePath << "\n"
         << "Output Peaks file: " << searchParams.peakFilePath << "\n";
    cout << "===============================" << endl;

    searchDataset->searchParams = searchParams;

    cout << "Reading in QC Standards..." << endl;

    searchDataset->standards = importStandards(searchParams.qcStandardsFilePath);

    cout << "Imported "
         << searchDataset->standards.size()
         << " QC Standards.\n"
         << "===============================" << endl;

    searchDataset->sampleFiles = sampleFiles;

    cout << "Identified "
         << sampleFiles.size()
         << " sample files.\n"
         << "===============================" << endl;

    return searchDataset;
}

vector<QCStandard> importStandards(string qcStandardsFile) {
    vector<QCStandard> standards{};

    ifstream qcStandardsFileStream(qcStandardsFile.c_str());

    if (! qcStandardsFileStream.is_open()) return standards;

    vector<string> headers{};
    map<string, unsigned int> headerPosMap{};

    string line;
    while ( getline(qcStandardsFileStream, line) ) {
        if (line.empty()) continue;
        if (line[0] == '#') continue;

        if (headers.empty()) {
            mzUtils::split(line,'\t', headers);
            for (unsigned int i = 0; i < headers.size(); i++) {
                string key = headers[i];

                key.erase(std::remove(key.begin(), key.end(), '\r'), key.end());

                headers[i] = key;

                std::transform(key.begin(), key.end(),key.begin(), ::toupper);
                headerPosMap.insert(make_pair(key, i));
            }
            continue;
        }

        vector<string>fields;
        mzUtils::split(line,'\t', fields);

        for (unsigned int i = 0; i < fields.size(); i++) {
            string key = fields[i];

            key.erase(std::remove(key.begin(), key.end(), '\r'), key.end());

            fields[i] = key;
        }

        QCStandard standard;

        try {
            standard.compoundName = fields.at(headerPosMap.at("COMPOUND NAME"));
            standard.standardSet = fields.at(headerPosMap.at("STANDARD SET"));
            standard.msMethod = fields.at(headerPosMap.at("MS METHOD"));
            standard.standardType = fields.at(headerPosMap.at("TYPE"));
            standard.formula = fields.at(headerPosMap.at("MOLECULAR FORMULA"));
            standard.precMz = stod(fields.at(headerPosMap.at("PRECURSOR MZ")));
            standard.adductName = fields.at(headerPosMap.at("ADDUCT"));
            standard.rt = stod(fields.at(headerPosMap.at("RETENTION TIME")));

            standards.push_back(standard);
        } catch (std::invalid_argument) {
            cout << "skipping entry: " << line << endl;
        }

    }
    qcStandardsFileStream.close();

    return standards;
}

void writePeaks(EIC* eic, QCStandard qcStandard, shared_ptr<SearchDataset> searchDataset, string tag) {

        eic->setSmootherType(searchDataset->searchParams.smootherType);
        eic->setBaselineDropTopX(searchDataset->searchParams.baseline_dropTopX);
        eic->setBaselineSmoothingWindow(searchDataset->searchParams.baseline_smoothingWindow);
        eic->getPeakPositionsC(searchDataset->searchParams.smootherWindow, false, true, 0.5f);

        if ( eic->peaks.size() == 0 ) return;

        Peak maxIntensityPeak;
        float maxIntensity = -1;
        for (auto peak : eic->peaks) {
            float intensity = eic->intensity[peak.pos];
            if (intensity > maxIntensity) {
                maxIntensityPeak = peak;
                maxIntensity = intensity;
            }
        }

        Peak& x = maxIntensityPeak;

        float rtDiff = abs(x.rt - static_cast<float>(qcStandard.rt));

        searchDataset->peakFile

            //peak ID info
            << eic->sample->sampleId << "\t"
                << eic->sampleName << "\t"
                << tag << "\t"

                //peak bounds
                << x.pos << "\t"
                << x.peakMz << "\t"
                << x.rt << "\t"
                << x.minpos << "\t"
                << x.maxpos << "\t"
                << x.width << "\t"
                << x.mzmin << "\t"
                << x.mzmax << "\t"
                << x.rtmin << "\t"
                << x.rtmax << "\t"

                //peak quantitation types
                << x.peakIntensity << "\t"
                << x.peakAreaTop << "\t"
                << x.peakArea << "\t"
                << x.peakAreaCorrected << "\t"
                << x.peakBaseLineLevel << "\t"
                << x.signalBaselineRatio << "\t"

                //quality scores
                << x.gaussFitR2 << "\t"
                << x.gaussFitSigma << "\t"
                << rtDiff
                << endl;
}

int file_unique_id(string filename) {

        struct stat file_info;
#ifdef _WIN32
        _stat(filename.c_str(), &file_info);
#else
        lstat(filename.c_str(), &file_info);
#endif
        return static_cast<int>(file_info.st_ino);
}

void writeEIC(EIC* eic, QCStandard qcStandard, shared_ptr<SearchDataset> searchDataset, string tag) {
		if(!eic) return;
		if(eic->intensity.size()==0) return;

        for(unsigned int i=0; i < eic->mz.size(); i++) {
                if (eic->intensity[i] <= searchDataset->searchParams.minIntensity) continue;
                searchDataset->eicFile
				<< eic->sample->sampleId << "\t"
				<< eic->sample->sampleName << "\t"
				<< tag << "\t" 
				<< setprecision(6)
				<< eic->rt[i] << "\t"
				<< setprecision(8)
				<< eic->mz[i] << "\t" 
                << static_cast<int>(eic->intensity[i])
                << endl;
		}

        writePeaks(eic, qcStandard, searchDataset, tag);
}


EIC* pullEIC(mzSample* sample, QCStandard qcStandard, shared_ptr<SearchDataset> searchDataset) {

        float preMz = static_cast<float>(qcStandard.precMz);
        float amuTolr = static_cast<float>(preMz/1.0e6f*searchDataset->searchParams.precMzTol);

        float rtmin = 0;
        float rtmax = 1e9;
        if (searchDataset->searchParams.rtTol > 0) {
            rtmin = static_cast<float>(qcStandard.rt) - searchDataset->searchParams.rtTol;
            rtmax = static_cast<float>(qcStandard.rt) + searchDataset->searchParams.rtTol;
        }
        EIC* eic = sample->getEIC(
                    preMz-amuTolr,
                    preMz + amuTolr,
                    rtmin,
                    rtmax,
                    searchDataset->searchParams.msLevel);

        return(eic);
}


void processSample(string fileIn, shared_ptr<SearchDataset> searchDataset) {
	
	mzSample* sample = new mzSample();
    sample->loadSample(fileIn.c_str());    //load file
	sample->sampleId = file_unique_id(fileIn.c_str());

	//remove extenstion from the samplename
	mzUtils::replace( sample->sampleName, ".mzML", "");
	mzUtils::replace( sample->sampleName, ".mzXML", "");

    if (sample->scans.size() == 0 ) {
        cerr << "Can't load scan data from file " << fileIn << endl;
        return;
    }

    cout << "Processing sample '"
         << sample->sampleName
         << "'..."
         << endl;

	//dump tic 
    cout << "Computing TIC for sample '"
         << sample->sampleName
         << "'..."
         << endl;

    EIC* tic1 = sample->getTIC(0, 1e9,1);
    writeEIC(tic1, QCStandard(), searchDataset, "tic1");
	delete(tic1);

    EIC* tic2 = sample->getTIC(0, 1e9, 2);
    writeEIC(tic2, QCStandard(), searchDataset, "tic2");
	delete(tic2);

    EIC* tic3 = sample->getTIC(0, 1e9,3);
    writeEIC(tic3, QCStandard(), searchDataset, "tic3");
	delete(tic3);

    EIC* bic1 = sample->getBIC(0, 1e9,1);
    writeEIC(bic1, QCStandard(), searchDataset, "bic1");
	delete(bic1);

    // Samples with the string "std" should be searched against external standards
    // Otherwise, should be searched against internal standards
    bool isExternalStandard = sample->sampleName.find("std") != string::npos;

    for(auto c: searchDataset->standards) {

        // Only search for standards of the correct type according to the searched sample
        if (searchDataset->searchParams.isMatchSampleType) {
            if ((isExternalStandard && c.standardType == "Internal") || (!isExternalStandard && c.standardType == "External")) continue;
        }

        // Only search for standards that match the sample type
        if (searchDataset->searchParams.isMatchMethod) {
            if (sample->sampleName.find(c.msMethod) == string::npos) continue;
        }

        string tag = c.compoundName + " " + c.adductName;

        cout << "Pulling EIC for QC Standard '"
             << tag
             << "'..."
             << endl;

        EIC* eic = pullEIC(sample, c, searchDataset);

        writeEIC(eic, c, searchDataset, tag);
		delete(eic);
	}

    cout << "finished processing sample '"
         << sample->sampleName
         << "'"
         << endl;

    delete(sample);
}

void writeHeaders(shared_ptr<SearchDataset> searchDataset){

    searchDataset->eicFile
            << "sampleid" << "\t"
            << "samplename" << "\t"
            << "tag" << "\t"
            << "rt" << "\t"
            << "mz" << "\t"
            << "intensity" << endl;

    searchDataset->peakFile
            << "sampleid" << "\t"
            << "samplename" << "\t"
            << "tag" << "\t"
            << "pos" << "\t"
            << "peakMz" << "\t"
            << "rt" << "\t"
            << "minpos" << "\t"
            << "maxpos" << "\t"
            << "width" << "\t"
            << "mzmin" << "\t"
            << "mzmax" << "\t"
            << "rtmin" << "\t"
            << "rtmax" << "\t"
            << "peakIntensity" << "\t"
            << "peakAreaTop" << "\t"
            << "peakArea" << "\t"
            << "peakAreaCorrected" << "\t"
            << "peakBaseLineLevel" << "\t"
            << "signalBaselineRatio" << "\t"
            << "gaussFitR2" << "\t"
            << "gaussFitSigma" << "\t"
            << "rtDiff" << endl;
}
