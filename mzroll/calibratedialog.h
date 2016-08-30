#ifndef CALIBRATEDIALOG_H
#define CALIBRATEDIALOG_H

#include "stable.h"
#include "ui_calibrateform.h"
#include "database.h"
#include "mainwindow.h"

class MainWindow;
class BackgroundPeakUpdate;
extern Database DB; 


class CalibrateDialog : public QDialog, public Ui_CalibrateDialog {
		Q_OBJECT

		public:
			 CalibrateDialog(QWidget *parent);

                public slots:
                    void show();
                    void runCalibration();
                    void undoCalibration();
                    void doCorrection();
                    void addPeakGroup(PeakGroup* grp,bool);

        private:
                    MainWindow* mw;
                    BackgroundPeakUpdate* workerThread;
                    vector<PeakGroup>allgroups;
                    bool evaluateFit(vector<double>obs, vector<double>exp, vector<double> coef);
                    void applyFit(mzSample* sample, vector<double> coef);
                    void reverseFit(mzSample* sample);
                    vector<double> leastSqrFit(vector<double> &x, vector<double> &y);

};

#endif
