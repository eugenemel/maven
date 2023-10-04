#ifndef PROJECTDOCKWIDGET_H
#define PROJECTDOCKWIDGET_H

#include "stable.h"
#include "mzSample.h"
#include "mainwindow.h"
#include "projectDB.h"
#include "mzfileio.h"

class mzFileIO;

class ProjectDockWidget : public QDockWidget
{
    Q_OBJECT
public:
    explicit ProjectDockWidget(QMainWindow *parent = 0);
    QColor getSampleColor(mzSample* sample);
    QIcon getSampleIcon(mzSample* sample);
    ProjectDB* currentProject;

public slots:
    void showInfo();
    void setInfo(vector<mzSample*>&samples);
    void changeSampleOrder();
    void updateSampleList();
    void loadProject();
    void saveProject();
    void saveProjectAs();
    void closeProject();
    void loadAllPeakTables();
    void loadProjectSQLITE(QString filename);
    void saveProjectSQLITE(QString filename);

    void loadProjectXML(QString filename);
    void saveProjectXML(QString filename, TableDockWidget* peakTable = 0);
    void allSamplesVisible();
    void allSamplesInvisible();
    void toggleSelectedSamples();
    void toggleSamplesVisibility(QTreeWidgetItem *item, bool isVisible);
    void toggleSelectedSampleVisibility(QTreeWidgetItem *item);

    void refreshSamplesListInOtherWidgets();

protected slots:
      void keyPressEvent( QKeyEvent *e );
      void contextMenuEvent ( QContextMenuEvent * event );

private slots:
    void showSample(QTreeWidgetItem* item, int col);
    void showSampleInfo(QTreeWidgetItem* item, int col);
    void changeSampleColor(QTreeWidgetItem* item, int col);
    void changeNormalizationConstant(QTreeWidgetItem* item, int col);
    void changeSampleSet(QTreeWidgetItem* item, int col);
    void selectSample(QTreeWidgetItem* item, int col);
    void changeColors();
    void setSampleColor(QTreeWidgetItem* item, QColor color);
    void unloadSample();
    void unloadAllSamples();
    void filterTreeItems(QString filterString);
    void getSampleInfoSQLITE();
    void warnUserEmptySampleFiles();
    void importSampleMetadata();
    void exportSampleMetadata();
    void computeGroupStatistics(PeakGroup& peakGroup);

private:
    QTreeWidgetItem* getParentFolder(QString filename);
    QMap<QString,QTreeWidgetItem*> parentMap;
    MainWindow* _mainwindow;
    QTreeWidget* _treeWidget;

    QString lastOpennedProject;
    QString lastSavedProject;
    QColor  lastUsedSampleColor;

    mzFileIO*  fileLoader;

};

class SampleTreeWidget : public QTreeWidget{
    Q_OBJECT

public:
    SampleTreeWidget(QWidget *parent = nullptr) : QTreeWidget(parent){
        setDragEnabled(true);
        setDragDropMode(QAbstractItemView::InternalMove);
    }

protected:
    void dropEvent(QDropEvent *event) {
        QTreeWidget::dropEvent(event);
        emit(sampleTreeWidgetDropEvent());
    }

    signals:
        void sampleTreeWidgetDropEvent();
};

#endif // PROJECTDOCKWIDGET_H
