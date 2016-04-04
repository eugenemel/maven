#ifndef SMILEY_MOLECULE_H
#define SMILEY_MOLECULE_H

#include "mzUtils.h"
#include <vector>
#include <string>
#include <map>
#include <iostream>

namespace Smiley {

struct AtomInfo {
    std::string SYMBOLS[109]={"X","H","He","Li","Be","B","C","N","O","F","Ne","Na","Mg","Al","Si","P","S","Cl","Ar","K",
                              "Ca","Sc","Ti","V","Cr","Mn","Fe","Co","Ni","Cu","Zn","Ga","Ge","As","Se","Br","Kr","Rb","Sr","Y","Zr",
                              "Nb","Mo","Tc","Ru","Rh","Pd","Ag","Cd","In","Sn","Sb","Te","I","Xe","Cs","Ba","La","Ce","Pr","Nd","Pm",
                              "Sm","Eu","Gd","Tb","Dy","Ho","Er","Tm","Yb","Lu","Hf","Ta","W","Re","Os","Ir","Pt","Au","Hg","Tl","Pb",
                              "Bi","Po","At","Rn","Fr","Ra","Ac","Th","Pa","U","Np","Pu","Am","Cm","Bk","Cf","Es","Fm","Md","No","Lr",
                              "Hx","Cx","Nx","Ox","Sx"};

    double ELEMEMNT_MASS[109]={0.000000000000,1.0078250321,3.016030000000,6.015121000000,9.012182000000,10.012937000000,
                               12.000000000000,14.0030740052,15.9949146221,18.998403200000,19.992435000000,22.989767000000,
                               23.985042000000,26.981539000000,27.976927000000,30.97376151,31.97207069,34.968853100000,
                               35.967545000000,38.963707000000,39.962591000000,44.955910000000,45.952629000000,49.947161000000,
                               49.946046000000,54.938047000000,53.939612000000,58.933198000000,57.935346000000,62.939598000000,
                               63.929145000000,68.925580000000,69.924250000000,74.921594000000,73.922475000000,78.918336000000,
                               77.914000000000,84.911794000000,83.913430000000,88.905849000000,89.904703000000,92.906377000000,
                               91.906808000000,98.000000000000,95.907599000000,102.905500000000,101.905634000000,106.905092000000,
                               105.906461000000,112.904061000000,111.904826000000,120.903821000000,119.904048000000,126.904473000000,
                               123.905894000000,132.905429000000,129.906282000000,137.907110000000,135.907140000000,140.907647000000,
                               141.907719000000,145.000000000000,143.911998000000,150.919847000000,151.919786000000,158.925342000000,
                               155.925277000000,164.930319000000,161.928775000000,168.934212000000,167.933894000000,174.940770000000,
                               173.940044000000,179.947462000000,179.946701000000,184.952951000000,183.952488000000,190.960584000000,
                               189.959917000000,196.966543000000,195.965807000000,202.972320000000,203.973020000000,208.980374000000,
                               209.000000000000,210.000000000000,222.000000000000,223.000000000000,226.025000000000,227.028000000000,
                               232.038054000000,231.035900000000,234.040946000000,237.048000000000,244.000000000000,243.000000000000,
                               247.000000000000,247.000000000000,251.000000000000,252.000000000000,257.000000000000,258.000000000000,
                               259.000000000000,260.000000000000,1.007824600000,12.000000000000,14.003073200000,15.994914100000,
                               31.972070000000};
};

  struct Atom
  {
    int element;
    int isotope;
    int hCount;
    int charge;
    int atomClass;
    bool aromatic;
    bool inRing;
    unsigned int atomIdx;
  };

  struct Bond
  {
    int source;
    int target;
    int order;
    bool isUp;
    bool isDown;
    int bondIdx;
  };

  class Molecule
  {
    public: 
    const Bond* bond(int source, int target) const
    {
      for (std::size_t i = 0; i < bonds.size(); ++i)
        if ((source == bonds[i].source && target == bonds[i].target) || (source == bonds[i].target && target == bonds[i].source))
          return &bonds[i];
      return 0;
    }

    Bond* getBond(int idx) { return &bonds[idx]; }
    Atom* getAtom(int idx) { return &atoms[idx]; }

    std::vector<Bond*> getBonds(int source)  { 
      std::vector<Bond*> matches;
      for (std::size_t i = 0; i < bonds.size(); ++i) {
        if (source == bonds[i].source || source == bonds[i].target) matches.push_back(&bonds[i]);
      }
      return matches;
    }

    int countBonds(int source)  { 
      std::vector<Bond*> matches = getBonds(source);
      int count=0;

      for(Bond* b: matches) {
          if (b->order == 5 ) count+=1;
          else count += b->order;
      }

      return count;
    }

     double MW() {
        double mw=0;
        for (auto a: atoms ) mw += atominfo.ELEMEMNT_MASS[a.element];
        return mw;
    }

    void calculateRings() {
        for(int bi=0; bi < bonds.size(); bi++ ) {
            Bond* bond1 = &bonds[bi];
            Atom* atom1 = getAtom(bond1->source);
            Atom* atom2 = getAtom(bond1->target);
            if (atom1->element == 1 || atom2->element == 1 ) continue;
            //std::cerr << "\tCheck:" << atom1->element  << " " << atom2->element << "\n"; 
            
            std::vector<int> breakBondIdxList;  breakBondIdxList.push_back(bond1->bondIdx);
            std::vector<int>fragments = getFragments(breakBondIdxList);

            int nComp=0;
            for (int f: fragments) {
                if (f > nComp) nComp=f;
            }

            if (nComp==1) {
                atoms[bond1->source].inRing = true;
                atoms[bond1->target].inRing = true;
            }
        }
    }


    std::vector<int> getFragments(std::vector<int> breakBondIdxList) {
        std::vector<int>components(atoms.size(),0);
        int componentIdx=0;

        //walk along the molecule
        for(int ai=0; ai<atoms.size(); ai++ ) {
            if(atoms[ai].element == 1 ) continue; //skip hydrogens
            if(components[ai] != 0) continue;   //already processed
            if(components[ai] == 0) components[ai] = ++componentIdx; //new component number
            walkBonds(ai,components,breakBondIdxList);
        }
        return components;
    }

    std::vector<Atom*> getComponentAtoms(int compNum, std::vector<int>& components) { 
        std::vector<Atom*> subset;
        for(int ai=0; ai<atoms.size(); ai++ ) {
            if (components[ai] != compNum ) continue;
            subset.push_back(&atoms[ai]);
        }
        return subset;
    }

    std::map<std::string,double> getFragmentFormulas(std::vector<int>& components) {
        int nComp=0;
        for (int f: components) if (f > nComp) nComp=f;
        int componentsCount = nComp;

        //print components
        //for(int c: components) std::cerr << c; std::cerr << "\n";

        std::map<std::string,double> formulas;
        for(int ci=1; ci <= componentsCount; ci++ ) {
            std::vector<Atom*> atomSubset = getComponentAtoms(ci, components);

            //get atomic composition
            std::map<int,int>atomCounts;
            for(Atom* a : atomSubset )  atomCounts[a->element]++;
            
            //get number of bonds broken
            int stubCount = 0;
            for(Atom* a : atomSubset ) {
                std::vector<Bond*> abonds = getBonds(a->atomIdx);
                //std::cerr << a->atomIdx << " " << abonds.size() << std::endl;
                for(Bond* ab: abonds ) { 
                    //std::cerr << components[ab->source] << " " << components[ab->target] << std::endl;
                    if (components[ ab->source ] != components[ ab->target ] ) {
                        stubCount++;
                    }
                }
            }
            if (stubCount >=1) atomCounts[1] += stubCount; //ADD HYDROGENS to stubs

            //calc MW
            std::string formula = prettyFormula(atomCounts);
            double compMW=0;
            for (auto p: atomCounts) { 
                compMW  += atominfo.ELEMEMNT_MASS[p.first] * p.second;
            }

            //std::cerr << ci << " stubs=" << stubCount << " formula=" << formula << "\n";
            formulas[formula] = compMW;
        }
       return formulas;
    }

    void walkBonds(int ai, std::vector<int>& components, std::vector<int>& breakBondIdxList ) { 
            std::vector<Bond*> bonds = getBonds(ai);
            int componentId = components[ai]; // get atoms component

            std::vector<int> children;
            for(Bond* b: bonds) { 
                //check if this bond is broken.
                bool broken=false;
                for(int brokenBondIndex: breakBondIdxList ) {
                    if( b->bondIdx == brokenBondIndex) { 
                       // std::cerr << "broken bond=" << b->bondIdx << " " << b->source << " " << b->target << "\n";
                        broken = true; break;
                    }
                }
                if (! broken ) {
                    //add atoms to list of children to visit
                    if (b->source != ai) children.push_back(b->source);
                    if (b->target != ai) children.push_back(b->target);
                }
            }
            /*
           std::cerr << " walkBonds: ai=" << ai;
           std::cerr << " children: "; for(int aj: children) std::cerr << aj << "\t"; 
           std::cerr <<  "\n";
           */

           for(int aj: children) { 
               if (components[aj] > 0) continue;
               components[aj] = componentId;
               if (atoms[aj].element == 1) continue; //skip hydrogens
               walkBonds(aj,components, breakBondIdxList);
           }
    }

    bool isOrganic() { 
        //count atoms of each type
        int atomOrder[10] =  { 6, 1, 9, 17, 35, 53, 7, 8, 15, 16 };

        for(auto a: atoms) { 
            bool isOk = false;
            for( auto x: atomOrder ) if (a.element == x) { isOk = true; break; }
            if (!isOk) return false;
        }
        return true;
    }


    std::string getFormula() {
        //count atoms of each type
        std::map<int,int>atomCounts;
        for (auto a: atoms ) atomCounts[a.element]++;
        return prettyFormula(atomCounts);
    }

    std::string prettyFormula(std::map<int,int>atomCounts) { 

        //order  CHNOPS 
        int atomOrder[10] =  { 6, 1, 9, 17, 35, 53, 7, 8, 15, 16 };

        std::string formula;
        for (int e: atomOrder ) {
           if (atomCounts.count(e) ) {
               formula += atominfo.SYMBOLS[e];
               if (atomCounts[e]>1) formula += mzUtils::integer2string(atomCounts[e]);
           }
        }
        return formula;
         /*
        //all other atom types
        for (auto p: atomCounts) {
            if ( p.first == 1 || p.first == 6 || p.first == 7 || p.first == 8 || p.first == 15 || p.first == 16 ) continue;
            formula += SYMBOLS[p.first]+ std::to_string(p.second);
        }
        */
    }

    void addHydrogens() {

      int atomCount = atoms.size();
      int hCount=0;

      for (std::size_t i = 0; i < atomCount; i++) { 
          Atom* a = getAtom(i);
          int bcount = countBonds(i);
          int toAdd  = maxBonds(a) - bcount-1;

          //std::cerr << "atom=" << a->element << " aromatic=" << a->aromatic << " bcount=" << bcount << " add=" << toAdd << "\t\t";

          for(int j=0; j<toAdd;j++) {
            addAtom(1,false,0,1,0,0); 
            int newAtomId=atoms.size()-1;
            addBond(i,newAtomId,1,0,0);
            hCount++;
            //std::cerr << " H ";
          }
          //std::cerr << "\t" << hCount << "\n";
      }
    }

    void printGraph() {
      for (std::size_t i = 0; i < bonds.size(); i++) { 
          Bond* b =&bonds[i];
          std::cout << b->source << " " << b->target << " " << b->order << " " << atoms[b->source].element << " " << atoms[b->target].element << std::endl;
      }
    }
   void printAtoms() {
      for (std::size_t i = 0; i < atoms.size(); i++) { 
          Atom* a =  getAtom(i);
          std::cout << i << " " << a->element << " " << a->aromatic  << "\n";
      }
    }

  
    int  maxBonds(Atom* a ) { 
        int atomNum = a->element;
        int charge =  a->charge;
        int aromatic = a->aromatic;

        switch(atomNum) {
            case 1: return 1+charge;   //H
            case 6: return 4+charge-aromatic;   //C
            case 7: return 3+charge-aromatic;   //N
            case 8: return 2+charge-aromatic;   //O
            case 16: return 2+charge;  //S
            case 15: return 5+charge;  //P
            case 9:  return 1+charge;  //F,Cl,Br,I
            case 17: return 1+charge;
            case 35: return 1+charge;
            case 53: return 1+charge;



            default: return 1+charge;
        }
    }

    void addAtom(int atomicNumber, bool aromatic, int isotope, int hCount, int charge, int atomClass)
    {
      //std::cout << "addAtom(atomicNumber = " << atomicNumber << ", hCount = " << hCount << ")" <<  " arom=" << aromatic << std::endl;
      Atom a;
      a.element = atomicNumber;
      a.aromatic = aromatic;
      a.isotope = isotope;
      a.hCount = hCount;
      a.charge = charge;
      a.inRing = false;
      a.atomClass = atomClass;
      a.atomIdx =  atoms.size();
      atoms.push_back(a);
    }

    void addBond(int source, int target, int order, bool isUp, bool isDown)
    {
      bonds.resize(bonds.size() + 1);
      bonds.back().source = source;
      bonds.back().target = target;
      bonds.back().order = order;
      bonds.back().isUp = isUp;
      bonds.back().isDown = isDown;
      bonds.back().bondIdx = bonds.size()-1;
    }


    std::vector<Atom> atoms;
    std::vector<Bond> bonds;
    std::map<int, std::pair<Chirality, std::vector<int> > > chirality;
    AtomInfo atominfo;
  };

  struct MoleculeSmilesCallback : public CallbackBase
  {
    void clear()
    {
      molecule.atoms.clear();
      molecule.bonds.clear();
      molecule.chirality.clear();
    }

    void addAtom(int atomicNumber, bool aromatic, int isotope, int hCount, int charge, int atomClass)
    {
      Atom a;
      a.element = atomicNumber;
      a.aromatic = aromatic;
      a.isotope = isotope;
      a.hCount = hCount;
      a.charge = charge;
      a.inRing = false;
      a.atomClass = atomClass;
      a.atomIdx =  molecule.atoms.size();
      molecule.atoms.push_back(a);
    }

    void addBond(int source, int target, int order, bool isUp, bool isDown)
    {
      molecule.bonds.resize(molecule.bonds.size() + 1);
      molecule.bonds.back().source = source;
      molecule.bonds.back().target = target;
      molecule.bonds.back().order = order;
      molecule.bonds.back().isUp = isUp;
      molecule.bonds.back().isDown = isDown;
      molecule.bonds.back().bondIdx = molecule.bonds.size()-1;
    }

    void setChiral(int index, Chirality chirality, const std::vector<int> &chiralRefs)
    {
      molecule.chirality[index] = std::make_pair(chirality, chiralRefs);
    }

    Molecule molecule;
  };

}

#endif
