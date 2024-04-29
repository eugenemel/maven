#ifndef CONFIGUREDIFFISOTOPESEARCH_H
#define CONFIGUREDIFFISOTOPESEARCH_H

#include "stable.h"
#include "ui_configurediffisotopesearch.h"

class ConfigureDiffIsotopeSearch : public QDialog, public Ui_ConfigureDiffIsotopeSearch
{
		Q_OBJECT

		public:
            ConfigureDiffIsotopeSearch(QWidget *parent);

        public slots:
            void populateSamples(vector<mzSample*> loadedSamples);
            void bringIntoView();
            void resetSamples();

        private:
            bool isSampleInListWidget(mzSample* sample, QListWidget *widget);
};

#endif
