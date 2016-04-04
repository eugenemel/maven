#ifndef BONDBREAKER_H
#define BONDBREAKER_H

#include <iostream>
#include <string>
#include <set>
#include <map>
#include <iomanip>

#include "smiley.h"
#include "molecule.h"

class BondBreaker {

public:
    BondBreaker() {
        _breakCBonds=false;
        _breakDoubleBonds=false;
        _minMW=60;
    }

    void breakCarbonBonds(bool flag) { _breakCBonds=flag; }
    void breakDoubleBonds(bool flag) { _breakDoubleBonds=flag; }
    void setMinMW(double mw) { _minMW=mw; }

    void setSmile(std::string smile) {
        _smile = smile;
        allfragments.clear();
        breakBonds(smile);
    }

    inline map<string,double> getFragments() {
        return allfragments; 
    }

private:
        std::string _smile;
        std::map<string,double> allfragments;
        void breakBonds(std::string smile);
        bool checkBondBreakable(Smiley::Molecule& mol, int bi);

        bool _breakCBonds;
        bool _breakDoubleBonds;
        double _minMW;


};


#endif
