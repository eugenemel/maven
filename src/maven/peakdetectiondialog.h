#ifndef PEAKDETECTIONDIALOG_H
#define PEAKDETECTIONDIALOG_H

#include "ui_peakdetectiondialog.h"
#include "stable.h"
#include "database.h"
#include "mainwindow.h"
#include "msmsscoringsettings.h"
#include "configurediffisotopesearch.h"

class MainWindow;
class TableDockWidget;
class BackgroundPeakUpdate;
class MSMSScoringSettingsDialog;

extern Database DB; 


class PeakDetectionDialog : public QDialog, public Ui_PeakDetectionDialog
{
		Q_OBJECT

        public:
        enum FeatureDetectionType { PeaksSearch=0, LibrarySearch, QQQ };
				 PeakDetectionDialog(QWidget *parent);
				 ~PeakDetectionDialog();
                 void setSettings(QSettings* settings) { this->settings = settings; setUIValuesFromSettings();}
                 void setMainWindow(MainWindow* w);

		public slots:
				 void findPeaks();
				 void loadModel();
				 void setOutputDir();
				 void setProgressBar(QString text, int progress, int totalSteps);
				 void runBackgroupJob(QString func);
				 void showInfo(QString text);
				 void cancel();
				 void show();
                 void setFeatureDetection(FeatureDetectionType type);
                 void updateLibraryList();
                 void showLibraryDialog();
                 void resetDiffIsotopeSampleList();
                 void populateDiffIsotopeSampleList();

		private:
                void setUIValuesFromSettings();
				QSettings *settings;
				MainWindow *mainwindow;
   				BackgroundPeakUpdate* peakupdater;
				FeatureDetectionType _featureDetectionType;

                shared_ptr<PeaksSearchParameters> getPeaksSearchParameters();
                shared_ptr<LCLipidSearchParameters> getLipidSearchParameters();
                shared_ptr<MzkitchenMetaboliteSearchParameters> getMzkitchenMetaboliteSearchParameters();
                shared_ptr<PeakPickingAndGroupingParameters> getPeakPickingAndGroupingParameters();

                MSMSScoringSettingsDialog *scoringSettingsDialog;
                ConfigureDiffIsotopeSearch *configureDiffIsotopeSearch;
                void displayCompoundOptions(bool isDisplayCompoundOptions);
};

#endif
