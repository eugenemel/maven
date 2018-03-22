#include "classifierNeuralNet.h"
#include <stdio.h>

ClassifierNeuralNet::ClassifierNeuralNet() 
{
	num_features=9;
	hidden_layer=4;
	num_outputs=2;
	trainingSize=0;
	brain = NULL;
	clsf_type = "NeuralNet";
	
	features_names.push_back("peakAreaFractional"); //f1
	features_names.push_back("noNoiseFraction");    //f2
	features_names.push_back("symmetry");           //f3
	features_names.push_back("groupOverlap");		//f4
	features_names.push_back("gaussFitR2");			//f5
	features_names.push_back("S/N");				//f6
	features_names.push_back("peakRank");			//f7
	features_names.push_back("peakIntensity");		//f8
	features_names.push_back("skinnyPeak");			//f9
//	features_names.push_back("widePeak");			//f10

     defaultModel = "nnlib network weights data file version 1.0\n\
Size: 9 4 2\n\
Hidden layer weights:\n\
0.348244	0.402841	0.550701	0.194118	-0.753584	1.74345	-1.77715	0.00668378	-0.534154\n\
-0.889052	-0.114491	-1.11209	-1.11362	1.36742	-2.0695	3.25954	-0.105453	0.230226\n\
-0.189997	0.50211	0.389362	0.0126719	-0.498335	-0.380567	-0.0783285	0.188709	-0.123932\n\
-0.0173413	-0.379919	0.71529	-0.464644	-0.375341	0.845834	-0.113616	1.18479	-0.386247\n\
Output layer weights:\n\
1.74287	-3.31841	-0.520656	0.454478\n\
-1.33858	3.64759	0.439269	-0.761122\n";

}

ClassifierNeuralNet::~ClassifierNeuralNet() { 
    if(brain) delete(brain); brain=NULL;
}

bool ClassifierNeuralNet::hasModel() { 
		return brain != NULL; 
}

void ClassifierNeuralNet::saveModel(string filename) { 
	if ( brain == NULL ) return; 
	brain->save((char*) filename.c_str());
}

void ClassifierNeuralNet::loadDefaultModel(){

    char tmpfilename[L_tmpnam];
    tmpnam(tmpfilename);
    printf ("Tempname #1: %s\n",tmpfilename);

    FILE* tmpFile = fopen(tmpfilename,"w");
    fprintf(tmpFile,"%s",defaultModel.c_str());
    fclose(tmpFile);

    loadModel(tmpfilename);
}

void ClassifierNeuralNet::loadModel(string filename) { 
	if ( ! fileExists(filename) ) {
			cerr << "Can't load " << filename << endl;
			return;
	}
	if ( brain != NULL ) delete(brain);
	brain = new nnwork(num_features,hidden_layer,num_outputs);
	brain->load((char*) filename.c_str());
	cerr << "Read in classification model " << filename << endl;
}


vector<float> ClassifierNeuralNet::getFeatures(Peak& p) { 
		vector<float>set(num_features,0);
		if (p.width>0 ) {
				set[0] =  p.peakAreaFractional;
				set[1] =  p.noNoiseFraction;
				set[2] =  p.symmetry/(p.width+1)*log2(p.width+1);
				set[3] =  p.groupOverlapFrac;
				set[4] =  p.gaussFitR2*100.0;
				set[5] =  p.signalBaselineRatio>0 ? log2(p.signalBaselineRatio)/10.0 : 0;
				set[6] =  p.peakRank/10.0;
				set[7] =  p.peakIntensity>0 ? log10(p.peakIntensity) : 0;
				set[8] =  p.width <= 3 && p.signalBaselineRatio >= 3.0 ? 1 : 0;
				//set[9] =  ((float) (p.width >= 3) + (int) (p.symmetry >= 5) + (int) (p.signalBaselineRatio > 3.0))/3;
				if ( p.peakRank/10.0 > 1 ) set[6]=1;
				//cerr << "tiny=" << set[8] << " " << set[7] << " " << p.symmetry << endl;
				//set[7] =  ((float) (p.baseLineRightCleanCount >= 5) +  (int) (p.baseLineLeftCleanCount >= 5))/2;
		}
		return set;
}

void ClassifierNeuralNet::classify(PeakGroup* grp) {
	if ( brain == NULL ) return;

    float result[2] = {0.1,0.1};
	for (unsigned int j=0; j < grp->peaks.size(); j++ ) {
		float fts[1000]; 
		vector<float> features = getFeatures(grp->peaks[j]);
		for(int k=0;k<num_features;k++) fts[k]=features[k];
		brain->run(fts, result);
		grp->peaks[j].quality = result[0];
	}
}

void ClassifierNeuralNet::refineModel(PeakGroup* grp) {
		if (grp == NULL ) return;
		if (brain == NULL ) brain = new nnwork(num_features,hidden_layer,num_outputs);

		if (grp->label == 'g' || grp->label == 'b' ) {
				for (unsigned int j=0; j < grp->peaks.size(); j++ ) {
					Peak& p = grp->peaks[j];
					p.label = grp->label;
					if ( p.width < 2 )  p.label='b';
					if ( p.signalBaselineRatio <= 1 )  p.label='b'; 

       				float result[2] = {0.1,0.1};
					p.label == 'g' ? result[0]=0.9 : result[1]=0.9;

					if (p.label == 'g' || p.label == 'b' ) {
						vector<float> features = getFeatures(grp->peaks[j]);
						FEATURES.push_back(features);
						labels.push_back(p.label);

						float fts[1000]; 
						for(int k=0;k<num_features;k++) fts[k]=features[k];
						trainingSize++;
						brain->train(fts, result, 0.0000001/trainingSize, 0.001/trainingSize);
			}
		}
	}
}


void ClassifierNeuralNet::train(vector<PeakGroup*>& groups) {
	FEATURES.clear();
	labels.clear();
	trainingSize=0;
	for(unsigned int i=0; i < groups.size(); i++ ) { PeakGroup* grp = groups[i];
		refineModel(grp);
	}
}

/*
			if (grp == NULL )continue;
			if (grp->label == 'g' || grp->label == 'b' ) {
                    for (unsigned int j=0; j < grp->peaks.size(); j++ ) {
							Peak& p = grp->peaks[j];
                            p.label = grp->label;
                           	if ( p.width < 2 )  p.label='b';
							if ( p.signalBaselineRatio <= 1 )  p.label='b'; 
						    if ( p.label == 'b' ) {  badpeaks.push_back(&p); }
                            if ( p.label == 'g' ) {  goodpeaks.push_back(&p); }

							allpeaks.push_back(&p); 
							labels.push_back(p.label);
					}
			}
		}
		if (goodpeaks.size() == 0 || badpeaks.size() == 0) return;

		goodpeaks = removeRedundacy(goodpeaks);
		badpeaks =  removeRedundacy(badpeaks);

		int totalPeaks = goodpeaks.size() + badpeaks.size(); 
		int g=0; 
		int b=0; 

		for(int i=0; i < totalPeaks; i++ ) {
				Peak* p= NULL;
				if (i % 2 == 0 ) {
					if (g < goodpeaks.size() ) p = goodpeaks[g]; 
					g++;
				} else  {
					if (b < badpeaks.size() ) p = badpeaks[b]; 
					b++;
				} 

				if (p) {
        				float result[2] = {0.1, 0.1};
						p->label == 'g' ? result[0]=0.9 : result[1]=0.9;
						//cerr << p->label << endl;
						//
						float fts[1000]; 
						vector<float> features = getFeatures(*p);
						for(int k=0;k<num_features;k++) fts[k]=features[k];

						brain->train(fts, result, 0.0000001/(i+1), 0.2/(i+1));
				}
				if (g >= goodpeaks.size() && b >= badpeaks.size()) break;
		}
		cerr << "Done training. " << endl;

*/
