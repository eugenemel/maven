#ifndef TREEDOCKWIDGET_H
#define TREEDOCKWIDGET_H

#include "stable.h"
#include "mzSample.h"
#include "mainwindow.h"

class MainWindow;

using namespace std;

class TreeDockWidget: public QDockWidget {
      Q_OBJECT

public:
    QWidget 	*dockWidgetContents;
    QHBoxLayout *horizontalLayout;
    QTreeWidget *treeWidget;

    TreeDockWidget(MainWindow*, QString title, int numColms);

    //this->treeWidget will contain items only of this type
    inline void setExclusiveItemType(int exclusiveItemType){this->exclusiveItemType = exclusiveItemType;}

public slots: 
	  //void showInfo(PeakGroup* group);
	  QTreeWidgetItem* addItem(QTreeWidgetItem* parentItem, string key , float value,  int type);
      QTreeWidgetItem* addItem(QTreeWidgetItem* parentItem, string key , string value, int type);


      void showInfo(bool isUpdateMassCalcGUI=false);
      void showInfoAndUpdateMassCalcGUI();

	  void setInfo(Peak* peak);
	  void setInfo(PeakGroup* group);
	  void setInfo(Compound* c);
	  void setInfo(vector<Compound*>&compounds);
      void setInfo(double ms1PrecMz, double ms2PrecMz);
      void setInfo(vector<SRMTransition*>& srmTransitions);
      void setInfo(set<string>& srmIds);

	  void setInfo(vector<mzLink>&links);
      void setupScanListHeader();
      void setupConsensusScanListHeader();
      void setupMs1ScanHeader();
      void setupMs3ScanHeader();
      void setupSRMTransitionListHeader();

	  void addScanItem(Scan* scan);
      void addMs1ScanItem(Scan *scan);
      void addMs3ScanItem(Scan *scan);
      void addMs2ScanVectorItem(vector<Scan*> scans);

      void addMs1TitleBar();
      void addMs3TitleBar();
      void ms3SearchFromSpinBoxes();

	  void clearTree();
	  void filterTree(QString needle);
      void copyToClipbard();

      bool hasPeakGroup(PeakGroup* group);

      void selectMs2Scans(Peak *peak);
      void selectMs2Scans(PeakGroup *group);
      void selectMs2Scans(Scan *scan);

      protected slots:
          void keyPressEvent(QKeyEvent *e );
      void contextMenuEvent ( QContextMenuEvent *e );

      private slots:
      QTreeWidgetItem* addPeakGroup(PeakGroup* group, QTreeWidgetItem* parent);
      QTreeWidgetItem* addPeak(Peak* peak, QTreeWidgetItem* parent);
      QTreeWidgetItem* addCompound(Compound* c, QTreeWidgetItem* parent);
      QTreeWidgetItem* addSlice(mzSlice* s,QTreeWidgetItem* parent);
      QTreeWidgetItem* addLink(mzLink* s,  QTreeWidgetItem* parent);

      void unlinkGroup();

    private:
      void itemToClipboard(QTreeWidgetItem* item, QString& clipboardtext);
      int exclusiveItemType = -1; //this->treeWidget should only contain items of this type. If -1, any type.

      MainWindow* _mw = nullptr;
      QDoubleSpinBox *ms1PrecMzSpn = nullptr;
      QDoubleSpinBox *ms2PrecMzSpn = nullptr;
};

#endif
