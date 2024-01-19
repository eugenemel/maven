#ifndef SETTINGS_FORM_H
#define SETTINGS_FORM_H

#include "ui_settingsform.h"
#include "mainwindow.h"

class MainWindow;


class SettingsForm : public QDialog, public Ui_SettingsForm
{
                Q_OBJECT
		public:
                SettingsForm(QSettings* s, MainWindow *w);
                protected:
                    void closeEvent       (QCloseEvent* e) { getFormValues(); QDialog::closeEvent(e);}
                    void keyPressEvent    (QKeyEvent* e) { QDialog::keyPressEvent(e); getFormValues(); }
                    void mouseReleaseEvent(QMouseEvent* e) {QDialog::mouseReleaseEvent(e); getFormValues(); }

		public slots:
				 void setFormValues();
				 void getFormValues();
				 void recomputeEIC();
				 void recomputeIsotopes();
                 void replotMS1Spectrum();
                 void replotMS2Spectrum();
                 void replotMS3Spectrum();
                 void recomputeConsensusSpectrum();
                 void selectFolder(QString key);
                 void selectFile(QString key);
                 void bringIntoView();

                 inline void showInstrumentationTab() { tabWidget->setCurrentIndex(0); }
                 inline void showFileImportTab() { tabWidget->setCurrentIndex(1); }
                 inline void showPeakDetectionTab()   { tabWidget->setCurrentIndex(2); }
                 inline void setIsotopeDetectionTab()  { tabWidget->setCurrentIndex(3); }

                 inline void selectMethodsFolder() {   selectFolder("methodsFolder"); }
                 void updateClassifierFile();



		private:
				QSettings *settings;
				MainWindow *mainwindow;
};

#endif
