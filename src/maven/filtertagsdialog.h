#ifndef FILTERTAGSDIALOG_H
#define FILTERTAGSDIALOG_H

#include "ui_filtertagsdialog.h"
#include "stable.h"
#include "database.h"
#include "mzSample.h"

extern Database DB;

struct TagFilterState;

// Filter behavior when multiple tags are involved in filtering
enum MultipleTagFilterProtocol {
    OR=0, // If any tag is present, item passes.
    AND=1, // If all tags are present, item passes. Otherwise, item does not pass.
    NOR=2, // If even one tag is present, item does not pass. Otherwise, item passes.
    NAND=3 // If all tags are present, item does not pass. Otherwise, item passes.
};

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
        QTableWidgetItem *reAssignedCompoundTag = nullptr;
        QTableWidgetItem *manualIntegrationTag = nullptr;
};

struct TagFilterState {

    MultipleTagFilterProtocol multipleTagFilterProtocol = MultipleTagFilterProtocol::OR;
    bool isAllLabelsUnselected = true;
    bool isNoTagsPass = true;
    vector<char> selectedLabels;

    bool isPeakGroupPasses(PeakGroup *g);

    MultipleTagFilterProtocol static getMultipleTagFilterProtocolFromName(QString multipleTagFilterProtocol);
};

#endif // FILTERTAGSDIALOG_H
