#ifndef CONFIGUREDIFFISOTOPESEARCH_H
#define CONFIGUREDIFFISOTOPESEARCH_H

#include "stable.h"
#include "ui_configurediffisotopesearch.h"

class ConfigureDiffIsotopeSearch : public QDialog, public Ui_ConfigureDiffIsotopeSearch
{
		Q_OBJECT

		public:
            ConfigureDiffIsotopeSearch(QWidget *parent);

            vector<mzSample*> unlabeledSamples{};
            vector<mzSample*> labeledSamples{};

        public slots:
            void populateSamples(vector<mzSample*> projectSamples);
            void bringIntoView();
            void resetSamples();
};

#endif
