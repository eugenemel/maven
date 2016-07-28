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
};

#endif
