#include "filtertagsdialog.h"

FilterTagsDialog::FilterTagsDialog(QWidget *parent) : QDialog(parent) {
    setupUi(this);
    setModal(true);

    tblTags->setColumnWidth(0, 75);     // visible
    tblTags->setColumnWidth(1, 150);    // name
    tblTags->setColumnWidth(2, 50);     // label
    tblTags->setColumnWidth(3, 50);     // hotkey
    tblTags->setColumnWidth(4, 50);     // icon
    tblTags->setColumnWidth(5, 600);    // description

    tblTags->setSortingEnabled(true);
    tblTags->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tblTags->setSelectionMode(QAbstractItemView::NoSelection);

    //start counter for building table
    int counter = 0;

    //Special cases: does not correspond to a PeakGroupTag* object

    //No Tags

    tblTags->insertRow(counter);

    QTableWidgetItem *u_item0 = new QTableWidgetItem();
    u_item0->setCheckState(Qt::Checked);
    tblTags->setItem(counter, 0, u_item0);

    noTags = u_item0;

    QTableWidgetItem *u_item1 = new QTableWidgetItem();
    u_item1->setText("No Tags");
    tblTags->setItem(counter, 1, u_item1);

    QTableWidgetItem *u_item2 = new QTableWidgetItem();
    u_item2->setText("");
    tblTags->setItem(counter, 2, u_item2);

    QTableWidgetItem *u_item3 = new QTableWidgetItem();
    u_item3->setText("u");
    tblTags->setItem(counter, 3, u_item3);

    QTableWidgetItem *u_item4 = new QTableWidgetItem();
    u_item4->setIcon(QIcon());
    tblTags->setItem(counter, 4, u_item4);

    QTableWidgetItem *u_item5 = new QTableWidgetItem();
    u_item5->setText("Peak Group has not been tagged.");
    tblTags->setItem(counter, 5, u_item5);

    counter++;

    // Good Tag

    tblTags->insertRow(counter);

    QTableWidgetItem *g_item0 = new QTableWidgetItem();
    g_item0->setCheckState(Qt::Checked);
    tblTags->setItem(counter, 0, g_item0);

    goodTag = g_item0;

    QTableWidgetItem *g_item1 = new QTableWidgetItem();
    g_item1->setText("Good");
    tblTags->setItem(counter, 1, g_item1);

    QTableWidgetItem *g_item2 = new QTableWidgetItem();
    g_item2->setText(QString(PeakGroup::ReservedLabel::GOOD));
    tblTags->setItem(counter, 2, g_item2);

    QTableWidgetItem *g_item3 = new QTableWidgetItem();
    g_item3->setText("g");
    tblTags->setItem(counter, 3, g_item3);

    QTableWidgetItem *g_item4 = new QTableWidgetItem();
    g_item4->setIcon(QIcon(":/images/good.png"));
    tblTags->setItem(counter, 4, g_item4);

    QTableWidgetItem *g_item5 = new QTableWidgetItem();
    g_item5->setText("Peak Group is good.");
    tblTags->setItem(counter, 5, g_item5);

    counter++;

    // Bad Tag

    tblTags->insertRow(counter);

    QTableWidgetItem *b_item0 = new QTableWidgetItem();
    b_item0->setCheckState(Qt::Checked);
    tblTags->setItem(counter, 0, b_item0);

    badTag = b_item0;

    QTableWidgetItem *b_item1 = new QTableWidgetItem();
    b_item1->setText("Bad");
    tblTags->setItem(counter, 1, b_item1);

    QTableWidgetItem *b_item2 = new QTableWidgetItem();
    b_item2->setText(QString(PeakGroup::ReservedLabel::BAD));
    tblTags->setItem(counter, 2, b_item2);

    QTableWidgetItem *b_item3 = new QTableWidgetItem();
    b_item3->setText("b");
    tblTags->setItem(counter, 3, b_item3);

    QTableWidgetItem *b_item4 = new QTableWidgetItem();
    b_item4->setIcon(QIcon(":/images/bad.png"));
    tblTags->setItem(counter, 4, b_item4);

    QTableWidgetItem *b_item5 = new QTableWidgetItem();
    b_item5->setText("Peak Group is bad.");
    tblTags->setItem(counter, 5, b_item5);

    counter++;

    //Issue 429: Compound manually changed (reassigned)

    tblTags->insertRow(counter);

    QTableWidgetItem *c_item0 = new QTableWidgetItem();
    c_item0->setCheckState(Qt::Checked);
    tblTags->setItem(counter, 0, c_item0);

    reAssignedCompoundTag = c_item0;

    QTableWidgetItem *c_item1 = new QTableWidgetItem();
    c_item1->setText("Reannotated");
    tblTags->setItem(counter, 1, c_item1);

    QTableWidgetItem *c_item2 = new QTableWidgetItem();
    c_item2->setText(QString(PeakGroup::ReservedLabel::COMPOUND_MANUALLY_CHANGED));
    tblTags->setItem(counter, 2, c_item2);

    QTableWidgetItem *c_item3 = new QTableWidgetItem();
    c_item3->setText(""); // no hotkey for this item - added automatically when compounds reannotated.
    tblTags->setItem(counter, 3, c_item3);

    QTableWidgetItem *c_item4 = new QTableWidgetItem();
    c_item4->setIcon(QIcon(":/images/compound_reassigned.png"));
    tblTags->setItem(counter, 4, c_item4);

    QTableWidgetItem *c_item5 = new QTableWidgetItem();
    c_item5->setText("Compound associated with Peak Group re-assigned manually.");
    tblTags->setItem(counter, 5, c_item5);

    counter++;

    //Issue 818: Peak group manual integrated

    tblTags->insertRow(counter);

    QTableWidgetItem *manual_integration_item0 = new QTableWidgetItem();
    manual_integration_item0->setCheckState(Qt::Checked);
    tblTags->setItem(counter, 0, manual_integration_item0);

    manualIntegrationTag = manual_integration_item0;

    QTableWidgetItem *manual_integration_item1 = new QTableWidgetItem();
    manual_integration_item1->setText("Manually Integrated");
    tblTags->setItem(counter, 1, manual_integration_item1);

    QTableWidgetItem *manual_integration_item2 = new QTableWidgetItem();
    manual_integration_item2->setText(QString(PeakGroup::ReservedLabel::MANUALLY_INTEGRATED));
    tblTags->setItem(counter, 2, manual_integration_item2);

    QTableWidgetItem *manual_integration_item3 = new QTableWidgetItem();
    manual_integration_item3->setText(""); // no hotkey for this item - added automatically when peak group geneated via manual annotation
    tblTags->setItem(counter, 3, manual_integration_item3);

    QTableWidgetItem *manual_integration_item4 = new QTableWidgetItem();
    manual_integration_item4->setIcon(QIcon(":/images/manual_integration.png"));
    tblTags->setItem(counter, 4, manual_integration_item4);

    QTableWidgetItem *manual_integration_item5 = new QTableWidgetItem();
    manual_integration_item5->setText("Peak Group generated via manual integration (click and drag) bookmarking.");
    tblTags->setItem(counter, 5, manual_integration_item5);

    counter++;

    //Peak group tags
    vector<PeakGroupTag*> supplementalTags = DB.getSupplementalPeakGroupTags();

    for (auto peakGroupTag : supplementalTags) {

        tblTags->insertRow(counter);

        QTableWidgetItem *item0 = new QTableWidgetItem();
        item0->setCheckState(Qt::Checked);
        tblTags->setItem(counter, 0, item0);

        checkBoxTag.insert(make_pair(item0, peakGroupTag));

        QTableWidgetItem *item1 = new QTableWidgetItem();
        item1->setText(peakGroupTag->tagName.c_str());
        tblTags->setItem(counter, 1, item1);

        QTableWidgetItem *item2 = new QTableWidgetItem();
        item2->setText(QString(peakGroupTag->label));
        tblTags->setItem(counter, 2, item2);

        QTableWidgetItem *item3 = new QTableWidgetItem();
        item3->setText(QString(peakGroupTag->hotkey));
        tblTags->setItem(counter, 3, item3);

        QTableWidgetItem *item4 = new QTableWidgetItem();
        item4->setIcon(peakGroupTag->icon);
        tblTags->setItem(counter, 4, item4);

        QTableWidgetItem *item5 = new QTableWidgetItem();
        item5->setText(peakGroupTag->description.c_str());
        tblTags->setItem(counter, 5, item5);

        counter++;
    }

    connect(btnSelectAll, SIGNAL(clicked()), this, SLOT(selectAll()));
    connect(btnDeselectAll, SIGNAL(clicked()), this, SLOT(deselectAll()));
    connect(btnCancel, SIGNAL(clicked()), this, SLOT(hide()));
    connect(btnApplyFilter, SIGNAL(clicked()), this, SLOT(processNewFilter()));
    connect(btnClear, SIGNAL(clicked()), this, SLOT(clearFilter()));
}

void FilterTagsDialog::selectAll() {
    qDebug() << "FilterTagsDialog::selectAll()";

    noTags->setCheckState(Qt::Checked);
    goodTag->setCheckState(Qt::Checked);
    badTag->setCheckState(Qt::Checked);
    reAssignedCompoundTag->setCheckState(Qt::Checked);
    manualIntegrationTag->setCheckState(Qt::Checked);

    for (auto it = checkBoxTag.begin(); it != checkBoxTag.end(); ++it) {
        it->first->setCheckState(Qt::Checked);
    }

    tblTags->update();
    tblTags->repaint();
}

void FilterTagsDialog::deselectAll() {
    qDebug() << "FilterTagsDialog::deselectAll()";

    noTags->setCheckState(Qt::Unchecked);
    goodTag->setCheckState(Qt::Unchecked);
    badTag->setCheckState(Qt::Unchecked);
    reAssignedCompoundTag->setCheckState(Qt::Unchecked);
    manualIntegrationTag->setCheckState(Qt::Unchecked);

    for (auto it = checkBoxTag.begin(); it != checkBoxTag.end(); ++it) {
        it->first->setCheckState(Qt::Unchecked);
    }

    tblTags->update();
    tblTags->repaint();
}

void FilterTagsDialog::clearFilter() {
    selectAll();
    hide();
    updateFilter();
}

void FilterTagsDialog::processNewFilter() {

    TagFilterState tagFilterState = getFilterState();

    if (!tagFilterState.isNoTagsPass && tagFilterState.passingLabels.empty()) {
        QMessageBox msgBox;
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setText("These filter settings will make all Peak Groups invisible, which is disallowed.");
        msgBox.setInformativeText("Did you mean to make all Peak Groups visible?");
        msgBox.setDefaultButton(QMessageBox::Yes);

        int ref = msgBox.exec();

        if (ref == QMessageBox::Yes) {
            selectAll();
        } else {
            return;
        }
    }

    hide();
    updateFilter();
}


TagFilterState FilterTagsDialog::getFilterState() {

    TagFilterState tagFilterState;

    bool isAllPass = true;
    bool isNoTagsPass = true;
    vector<char> passingLabels;

    if (noTags->checkState() == Qt::Checked) {
        isNoTagsPass = true;
    } else {
        isNoTagsPass = false;
        isAllPass = false;
    }

    if (goodTag->checkState() == Qt::Checked) {
        passingLabels.push_back(PeakGroup::ReservedLabel::GOOD);
    } else {
        isAllPass = false;
    }

    if (badTag->checkState() == Qt::Checked) {
        passingLabels.push_back(PeakGroup::ReservedLabel::BAD);
    } else {
        isAllPass = false;
    }

    if (reAssignedCompoundTag->checkState() == Qt::Checked) {
        passingLabels.push_back(PeakGroup::ReservedLabel::COMPOUND_MANUALLY_CHANGED);
    } else {
        isAllPass = false;
    }

    if (manualIntegrationTag->checkState() == Qt::Checked) {
        passingLabels.push_back(PeakGroup::ReservedLabel::MANUALLY_INTEGRATED);
    } else {
        isAllPass = false;
    }

    for (auto &it : checkBoxTag) {
        QTableWidgetItem *item = it.first;
        PeakGroupTag *tag = it.second;

        if (item->checkState() == Qt::Checked) {
            passingLabels.push_back(tag->label);
        } else {
            isAllPass = false;
        }
    }

    tagFilterState.isAllPass = isAllPass;
    tagFilterState.isNoTagsPass = isNoTagsPass;
    tagFilterState.passingLabels = passingLabels;

    return tagFilterState;
}

bool TagFilterState::isPeakGroupPasses(PeakGroup *g){

    if (isAllPass) return true;
    if (!g) return false; //in practice, this would be a QTreeWidgetItem that does not correspond to a PeakGroup
    if (g->labels.empty() && isNoTagsPass) return true;

    //passingLabels acts like an OR filter (if the label is found, will be retained).
    for (auto &x : passingLabels) {
        if(g->isGroupLabeled(x)) return true;
    }

    return false;
}


