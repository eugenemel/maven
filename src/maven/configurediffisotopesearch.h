#ifndef CONFIGUREDIFFISOTOPESEARCH_H
#define CONFIGUREDIFFISOTOPESEARCH_H

#include "stable.h"
#include "ui_configurediffisotopesearch.h"
// #include "mainwindow.h"

class ConfigureDiffIsotopeSearch : public QDialog, public Ui_ConfigureDiffIsotopeSearch
{
		Q_OBJECT

		public:
            ConfigureDiffIsotopeSearch(QWidget *parent);

            vector<mzSample*> unlabeledSamples{};
            vector<mzSample*> labeledSamples{};
            //  MainWindow *mainWindow = nullptr;

            // void setMainWindow(MainWindow *w){this->mainWindow = w;}

        public slots:
            void bringIntoView();
            void resetSamples();
};

#endif
