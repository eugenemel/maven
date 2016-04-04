#include "BondBreaker.h"

using namespace Smiley;
using namespace std;

void BondBreaker::breakBonds(string smile) {

        MoleculeSmilesCallback callback;
        Molecule &mol = callback.molecule;
        Parser<MoleculeSmilesCallback> parser(callback);
        try {
            parser.parse(smile);
        } catch (Exception &e) {
            if (e.type() == Exception::SyntaxError)
                std::cerr << "Smile Syntax Parse Error" << smile << endl;
            return; 
        }
        mol.addHydrogens();
        mol.calculateRings();

        if (!mol.isOrganic() ) {
            cerr <<  "NOT ORGANIC!  " << smile << endl;
            return;
        }

        //if ( comp.formula != mol.getFormula() ) {
        //    cerr << "HMMM FORMULA MISMATCH: " << comp.formula << " " << mol.getFormula() << " " << comp.id << " " << smile << endl;
        //   return;
        //}

        allfragments[mol.getFormula()] = mol.MW();

        //single bond break
        for(int bi=0; bi < mol.bonds.size(); bi++) {
            if (checkBondBreakable(mol,bi) == false) continue;

            vector<int>breakBonds;
            breakBonds.push_back(bi);
            vector<int> fragments = mol.getFragments(breakBonds);
            map<string,double> fragmentFormulas = mol.getFragmentFormulas(fragments);
            for (auto f : fragmentFormulas )  if(f.second>_minMW) allfragments[f.first] = f.second;
            //two bonds breaks
            for(int bj=0; bj < mol.bonds.size(); bj++) {
                if(bj == bi) continue;

                if (checkBondBreakable(mol, bj) == false) continue;
                breakBonds.push_back(bj);
                vector<int> fragments = mol.getFragments(breakBonds);
                map<string,double> fragmentFormulas = mol.getFragmentFormulas(fragments);
                for (auto f : fragmentFormulas )  if(f.second>_minMW) allfragments[f.first] = f.second;

                //three bonds breaks 
                for(int bk=bj+1; bk < mol.bonds.size(); bk++) {
                    if (checkBondBreakable(mol, bk) == false) continue;
                    breakBonds.push_back(bk);
                    vector<int> fragments = mol.getFragments(breakBonds);
                    map<string,double> fragmentFormulas = mol.getFragmentFormulas(fragments);
                    for (auto f : fragmentFormulas )  if(f.second>_minMW) allfragments[f.first] = f.second;
                }
            }
        }
}




bool BondBreaker::checkBondBreakable(Molecule& mol, int bi) { 
    Bond* bond1 =    mol.getBond(bi);
    Atom* atom1 =    mol.getAtom(bond1->source);
    Atom* atom2 =    mol.getAtom(bond1->target);


    //cerr << bi << " " << atom1->element << " " << atom2->element << " " << atom1->inRing << atom2->inRing << endl;
    if (bond1->order  >=  2 && _breakDoubleBonds == false)  return false; //break double bonds
    if (atom1->element == 1 || atom2->element == 1) return false; //do not break *-H bonds
    if (atom1->inRing != atom2->inRing ) return true; //break ring to non ring connection
    if (atom1->element == 6 && atom2->element == 6 && _breakCBonds == false) return false; //do not break C-C bonds

    if (atom1->element == 8 && atom2->element == 15) return false; //do not break O-P bonds
    if (atom1->element == 15 && atom2->element == 8) return false; //do not break P-O bonds

    return true;
}
