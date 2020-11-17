#ifndef TABLEDOCKWIDGET_H
#define TABLEDOCKWIDGET_H

#include "stable.h"
#include "mainwindow.h"
#include "traindialog.h"
#include "clusterdialog.h"
#include "searchparamsdialog.h"
#include "numeric_treewidgetitem.h"
#include "directinfusionprocessor.h"
#include "editpeakgroupingdialog.h"
#include "lipidsummarizationutils.h"
#include "filtertagsdialog.h"

class MainWindow;
class TrainDialog;
class ClusterDialog;
class SearchParamsDialog;
class NumericTreeWidgetItem;
class ProjectDB;

using namespace std;

class TableDockWidget: public QDockWidget {
      Q_OBJECT

public:
    QWidget 	*dockWidgetContents;
    QHBoxLayout *horizontalLayout;
    QTreeWidget *treeWidget;

    TableDockWidget(MainWindow* mw, QString title, int numColms, QString encodedTableInfo="", QString displayTableInfo="");
	~TableDockWidget();

	int  groupCount() { return allgroups.size(); }
	bool hasPeakGroup(PeakGroup* group);
    QList<PeakGroup*> getAllGroups(); //even hidden and deleted groups
    QList<PeakGroup*> getGroups();
    static QString groupTagString(PeakGroup* group);

    inline QString getDisplayTableInfo(){return displayTableInfo;}
    inline QString getEncodedTableInfo(){return encodedTableInfo;}

    inline bool isTargetedMs3Table(){return windowTitle().startsWith("Targeted MS3 Search");}
    inline bool isDirectInfusionTable(){return windowTitle().startsWith("Direct Infusion Analysis");}

public slots: 
	  //void showInfo(PeakGroup* group);
      PeakGroup* addPeakGroup(PeakGroup* group, bool updateTable);
      PeakGroup* addPeakGroup(PeakGroup* group, bool updateTable, bool isDeletePeakGroupPtr);
      void addDirectInfusionAnnotation(DirectInfusionGroupAnnotation *directInfusionGroupAnnotation, int clusterNum);
      void addMs3Annotation(Ms3Annotation* ms3Annotation, int clusterNum);
      void setupPeakTable();
      PeakGroup* getSelectedGroup();
      PeakGroup* getLastBookmarkedGroup();
      QList<PeakGroup*> getSelectedGroups();
          void showFocusedGroups();
          void clearFocusedGroups();
          void unhideFocusedGroups();


      //input from xml
        void loadPeakTable();
        void loadPeakTableXML(QString infile);

      //output to xml
       void savePeakTable();
       void savePeakTable(QString fileName);
       void writePeakTableXML(QXmlStreamWriter& stream);

      //output to csv file
      void exportGroupsToSpreadsheet();
	  void showSelectedGroup();
	  void setGroupLabel(char label);
	  void showLastGroup();
	  void showNextGroup();
	  void Train();
	  float showAccuracy(vector<PeakGroup*>& groups);
	  void saveModel();
	  void printPdfReport();
	  void updateTable();
	  void updateItem(QTreeWidgetItem* item);
	  void updateStatus();
       void runScript();

	  void markGroupBad();
	  void markGroupGood();
      void unmarkSelectedGroups();
      void tagGroup(const QString& tagLabel);
      void showAllGroups();
      void showAllGroupsThenSort();
	  void showHeatMap();
	  void showGallery();
          void showTreeMap();
	  void showScatterPlot();
	  void setClipboard();
      void deleteSelected();
      void sortBy(int);
	  void align();
	  void deleteAll();
          void clusterGroups();
	  void findMatchingCompounds();
	  void filterPeakTable();
          void loadSpreadsheet(QString fileName);
          int loadCSVFile(QString filename, QString sep);
          void switchTableView();
          void clearClusters();
          void filterTree();
          void filterTree(QString needle);
          void rescoreFragmentation();
      void changePeakGroupDisplay();
      void updateSelectedPeakGroup();
      void showEditPeakGroupDialog();
      void hideEditPeakGroupDialog();
      void selectGroup(PeakGroup *group);
      void updateTagFilter();
      void exportAlignmentFile();
      void disconnectCompounds(QString);
      void reconnectCompounds(QString);
      void refreshPeakGroupQuant();

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

protected slots:
	  void keyPressEvent( QKeyEvent *e );
	  void contextMenuEvent ( QContextMenuEvent * event );

private:
         bool traverseNode(QTreeWidgetItem *item, QString needle);
         void traverseAndDeleteGroups(QTreeWidgetItem *item);
         void traverseAndCollectVisibleGroups(QTreeWidgetItem *item, QList<PeakGroup*>& groups);
         void traverseAndSetVisibleState(QTreeWidgetItem *item, bool isVisible);
          void deletePeaks();
          void addRow(PeakGroup* group, QTreeWidgetItem* root);
          void heatmapBackground(QTreeWidgetItem* item);
	  PeakGroup* readGroupXML(QXmlStreamReader& xml,PeakGroup* parent);
          void writeGroupXML(QXmlStreamWriter& stream, PeakGroup* g);
	  void readPeakXML(QXmlStreamReader& xml,PeakGroup* parent);

         MainWindow* _mainwindow;
         std::vector<PeakGroup>allgroups;
         std::multimap<PeakGroup*, QTreeWidgetItem*> groupToItem = {};

          TrainDialog* traindialog;
          ClusterDialog*       clusterDialog;
          SearchParamsDialog*   searchParamsDialog;

          QLineEdit*  filterEditor;

          TagFilterState tagFilterState;
          QToolButton *btnTagsFilter;
          FilterTagsDialog*     filterTagsDialog;

          QDialog* 	 filtersDialog;
          EditPeakGroupDialog* editPeakGroupDialog;

          enum tableViewType{ groupView=0, peakView=1 };
          tableViewType viewType;

          QString displayTableInfo;
          QString encodedTableInfo;

          shared_ptr<DirectInfusionSearchParameters> directInfusionSearchParams = nullptr;
};

typedef std::multimap<PeakGroup*, QTreeWidgetItem*>::iterator rowIterator;

#endif
