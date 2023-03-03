#ifndef CSVREPORT_H
#define CSVREPORT_H

#include "stable.h"
#include "mzUtils.h"
#include "mzSample.h"
#include "tabledockwidget.h"

using namespace std;
using namespace mzUtils;

class mzSample;
class EIC;
class PeakGroup;
class mzLink;

class CSVReports {

	public:
		CSVReports(){};
        CSVReports(vector<mzSample*>& insamples);
		~CSVReports();

        void openGroupReport(string filename);
        void openPeakReport(string filename);
        void openMzLinkReport(string filename);

        void addGroup(PeakGroup* group);
        void writeIsotopeTableMzLink(mzLink* link);

		void closeFiles();
		void setSamples(vector<mzSample*>& insamples) { samples = insamples; }
		void setUserQuantType(PeakGroup::QType t) { qtype=t; }
        void setTabDelimited()   { SEP="\t"; }
        void setCommaDelimited() { SEP=","; }


	private:
		void writeGroupInfo(PeakGroup* group);
        void writePeakInfo(PeakGroup* group);
        string doubleQuoteString(std::string& in);

		int groupId;	//sequential group numbering
        string SEP;

		vector<mzSample*>samples;
		ofstream groupReport;
		ofstream peakReport;
        ofstream mzLinkReport;
		PeakGroup::QType qtype;
        set<string> writtenIds{};

};

#endif
