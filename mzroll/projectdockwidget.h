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
    int bookmarkPeakGroup(PeakGroup* group);
    void loadProjectSQLITE(QString filename);
    void saveProjectSQLITE(QString filename);

    void loadProjectXML(QString filename);
    void saveProjectXML(QString filename, TableDockWidget* peakTable = 0);

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
    void dropEvent (QDropEvent*event);
    void unloadSample();
    void unloadAllSamples();
    void filterTreeItems(QString filterString);
    void getSampleInfoSQLITE();

private:
    QTreeWidgetItem* getParentFolder(QString filename);
    QMap<QString,QTreeWidgetItem*> parentMap;
    MainWindow* _mainwindow;
    QTreeWidget* _treeWidget;

    QString lastOpennedProject;
    QString lastSavedProject;
    QColor  lastUsedSampleColor;

    ProjectDB* currentProject;
    mzFileIO*  fileLoader;

};

#endif // PROJECTDOCKWIDGET_H
