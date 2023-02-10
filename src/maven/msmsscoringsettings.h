#ifndef MSMSSCORINGSETTINGS_H
#define MSMSSCORINGSETTINGS_H

#include "stable.h"
#include "ui_msmsscoringsettings.h"

class MSMSScoringSettingsDialog : public QDialog, public Ui_MSMSScoringSettingsDialog
{
		Q_OBJECT

		public:
             MSMSScoringSettingsDialog(QWidget *parent);

        public slots:
             void setLipidClassAdductFile();
};

#endif
