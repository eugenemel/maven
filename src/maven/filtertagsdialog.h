#ifndef FILTERTAGSDIALOG_H
#define FILTERTAGSDIALOG_H

#include "ui_filtertagsdialog.h"
#include "stable.h"
#include "database.h"
#include "mzSample.h"

extern Database DB;

struct TagFilterState;

class FilterTagsDialog : public QDialog, public Ui_filterTagsDialog {

    Q_OBJECT

    public:
        FilterTagsDialog(QWidget *parent);
        TagFilterState getFilterState();

    signals:
        void updateFilter();

    public slots:
        void selectAll();
        void deselectAll();
        void processNewFilter();
        void clearFilter();

    private:
        map<QTableWidgetItem*, PeakGroupTag*> checkBoxTag = {};

        QTableWidgetItem *noTags = nullptr;
        QTableWidgetItem *goodTag = nullptr;
        QTableWidgetItem *badTag = nullptr;
};

struct TagFilterState {
    bool isAllPass = true;
    bool isNoTagsPass = true;
    vector<char> passingLabels;

    bool isPeakGroupPasses(PeakGroup *g);
};

#endif // FILTERTAGSDIALOG_H
