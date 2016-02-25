#include "Fragment.h"
#include "mzSample.h"

Compound::Compound(string id, string name, string formula, int charge ) {
		this->id = id;
		this->name = name;
		this->formula = formula;
		this->charge = charge;
        this->mass =  mcalc->computeNeutralMass(formula);
		this->expectedRt = -1;
        this->logP = 0;

		precursorMz=0;
		productMz=0;
		collisionEnergy=0;

}

MassCalculator* Compound::mcalc = new MassCalculator();

float Compound::ajustedMass(int charge) { 
	return Compound::mcalc->computeMass(formula,charge); 
}


FragmentationMatchScore Compound::scoreCompoundHit(Fragment* f, float productAmuToll=0.01) {
        FragmentationMatchScore s;
        Compound* cpd = this;

        if (!cpd or cpd->fragment_mzs.size() == 0) return s;

        Fragment t;
        t.precursorMz = cpd->precursorMz;
        t.mzs = cpd->fragment_mzs;
        t.intensity_array = cpd->fragment_intensity;

        int N = t.mzs.size();
        for(int i=0; i<N;i++) {
            t.mzs.push_back( t.mzs[i] + PROTON);
            t.intensity_array.push_back( t.intensity_array[i] );
            t.mzs.push_back( t.mzs[i] - PROTON );
            t.intensity_array.push_back( t.intensity_array[i] );
        }

        t.sortByIntensity();
        s = t.scoreMatch(f,productAmuToll);

        //if(s.numMatches>0)  {
        //   vector<int>ranks = Fragment::compareRanks(&t,f,productAmuToll);
        //    for(int i=0; i< ranks.size(); i++ ) if(ranks[i]>0) cerr << t.mzs[i] << " "; cerr << endl;
        //}
        return s;
}
