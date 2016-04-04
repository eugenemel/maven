#ifndef MASSCALCGUI_H
#define MASSCALCGUI_H

#include "stable.h"
#include "mainwindow.h"
#include "ui_masscalcwidget.h"
#include "mzMassCalculator.h"

class QAction;
class QTextEdit;
class MainWindow;
class MassCalculator;
class Database;

extern Database DB; 

class MassCalcWidget: public QDockWidget,  public Ui_MassCalcWidget {
      Q_OBJECT

public:
      MassCalcWidget(MainWindow* mw);
      ~MassCalcWidget();

protected:
        

public slots:
      void setPeakGroup(PeakGroup* grp);
      void setFragmentationScan(Scan* scan);
 	  void setMass(float mz);
	  void setCharge(float charge);
	  void setPPM(float ppm);
      void compute();
      void updateDatabaseList();

private slots:
      void showInfo();
	  void getMatches();
      void showTable();
   
private:
      MainWindow* _mw;
      MassCalculator mcalc;
      std::vector<MassCalculator::Match> matches;

	  double _mz;
	  double _charge;
	  double _ppm;

      void pubChemLink(QString formula);
      void keggLink(QString formula);
      
};

#endif
